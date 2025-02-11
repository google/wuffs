// Copyright 2025 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

// ----------------

//go:generate go run gen.go

// Package lowleveljpeg provides a JPEG encoder that takes in-DCT-space
// coefficients instead of in-XY-space pixels.
//
// It can be useful for experimenting with or visualizing in-DCT-space
// transformations, such as quantization, extrapolation and progressive-like
// reduction. Or for learning about how the JPEG file format works. But most
// programmers should use the Go standard library's image/jpeg package instead.
package lowleveljpeg

// See "the spec", also known as "the JPEG specification" or "ITU/CCITT T.81":
// https://www.w3.org/Graphics/JPEG/itu-t81.pdf

import (
	"errors"
	"io"
)

var (
	ErrBadAddNForColorType     = errors.New("lowleveljpeg: bad AddN for ColorType")
	ErrBadArgument             = errors.New("lowleveljpeg: bad argument")
	ErrInvalidBlockI16         = errors.New("lowleveljpeg: invalid BlockI16")
	ErrNilReceiver             = errors.New("lowleveljpeg: nil receiver")
	ErrPreviouslyReturnedError = errors.New("lowleveljpeg: previously returned error")
	ErrTooManyAddNCalls        = errors.New("lowleveljpeg: too many AddN calls")
)

// ColorType is a JPEG image's color type.
type ColorType byte

const (
	// ColorTypeInvalid, the zero value, is invalid.
	ColorTypeInvalid = ColorType(0)

	// ColorTypeGray means a single channel (Gray).
	ColorTypeGray = ColorType(1)

	// ColorTypeYCbCr444 means three channels (Luma, Chroma-blue, Chroma-red),
	// using 4:4:4 Chroma subsampling.
	ColorTypeYCbCr444 = ColorType(3)

	// ColorTypeYCbCr420 means three channels (Luma, Chroma-blue, Chroma-red),
	// using 4:2:0 Chroma subsampling.
	//
	// This is the most popular color type for colorful (not gray) JPEG images.
	ColorTypeYCbCr420 = ColorType(6)

	// As an implementation detail, the constants' numerical value is the
	// number of blocks in a JPEG MCU (Mininum Coded Unit).
)

// MCUDimensions returns the width and height of a Mininum Coded Unit.
func (c ColorType) MCUDimensions() (width int, height int) {
	switch c {
	case ColorTypeGray, ColorTypeYCbCr444:
		return 8, 8
	case ColorTypeYCbCr420:
		return 16, 16
	}
	return 0, 0
}

func (c ColorType) isValid() bool {
	switch c {
	case ColorTypeGray, ColorTypeYCbCr444, ColorTypeYCbCr420:
		return true
	}
	return false
}

// EncoderOptions are optional arguments to Encoder.Reset. The zero value is
// valid and means to use the default configuration.
type EncoderOptions struct {
	// QuantizationFactors are the BlockI16 quantization factors. A nil pointer
	// is equivalent to using DefaultQuality.
	QuantizationFactors *Array2QuantizationFactors

	// There may be other fields added in the future.
}

// Encoder writes JPEG-formatted bytes to an io.Writer.
//
// Call Reset and then AddN multiple times (N depends on the ColorType). Each
// AddN call encodes a Minimum Coded Unit (an 8×8 or 16×16 group of pixels,
// depending on the ColorType). Add MCUs in left-to-right top-to-bottom order.
// The final AddN call also writes the JPEG footer.
//
// AddN takes ArrayNBlockI16 arguments, which are post-FDCT (when encoding) or
// pre-IDCT (when decoding) coefficients.
//
// To get ArrayNBlockI16 values from pixels, use ExtractFrom on an image.Image
// (to get an array of BlockU8 values) and then FowardDCTFrom (to get an array
// of BlockI16 values). See Example_Basic for an example.
//
// An Encoder makes no allocations, other than the Encoder struct itself and
// any allocations that the passed io.Writer makes.
//
// An Encoder can be re-used, by calling Reset.
type Encoder struct {
	hasReturnedError bool

	colorType ColorType

	prevDC [3]int16

	numAddsRemaining uint32

	bitsV uint32
	bitsN uint32

	quants Array2QuantizationFactors

	// The fields above occupy 148 bytes. Each Reset or AddN call, in the worst
	// case, emits not much more than 2688 bytes. We round sizeof(Encoder) up
	// to be 3072, which is 1.5 times a power of 2.

	buf [3072 - 148]byte
}

// Reset makes an Encoder ready to use (ready to make AddN calls), for encoding
// a JPEG with the given colorType, width and height.
//
// colorType should be ColorTypeGray, ColorTypeYCbCr444 or ColorTypeYCbCr420.
//
// width and height should both be positive but no larger than 0xFFFF, since a
// JPEG image cannot represent anything larger.
//
// options may be nil, which means to use the default configuration.
func (e *Encoder) Reset(w io.Writer, colorType ColorType, width int, height int, options *EncoderOptions) error {
	if e == nil {
		return ErrNilReceiver
	} else if (width <= 0) || (0xFFFF < width) || (height <= 0) || (0xFFFF < height) || !colorType.isValid() {
		e.hasReturnedError = true
		return ErrBadArgument
	}

	if (options == nil) || (options.QuantizationFactors == nil) {
		e.quants.SetToStandardValues(DefaultQuality)
	} else {
		q := options.QuantizationFactors
		if !q[0].IsValid() || !q[1].IsValid() {
			e.hasReturnedError = true
			return ErrBadArgument
		}
		e.quants = *q
	}

	e.hasReturnedError = false
	e.colorType = colorType
	e.prevDC[0] = 0
	e.prevDC[1] = 0
	e.prevDC[2] = 0
	if colorType != ColorTypeYCbCr420 {
		e.numAddsRemaining = uint32((width+7)/8) * uint32((height+7)/8)
	} else {
		e.numAddsRemaining = uint32((width+15)/16) * uint32((height+15)/16)
	}
	e.bitsV = 0
	e.bitsN = 0

	e.buf[0] = 0xFF // SOI marker.
	e.buf[1] = 0xD8
	bufIndex := 2
	bufIndex = e.encodeDQT(bufIndex)
	bufIndex = e.encodeSOF0(bufIndex, width, height)
	bufIndex = e.encodeDHT(bufIndex)
	bufIndex = e.encodeSOSHeader(bufIndex)
	if _, err := w.Write(e.buf[:bufIndex]); err != nil {
		e.hasReturnedError = true
		return err
	}
	return nil
}

// Add1 adds an 8×8 group of pixels to a ColorTypeGray image.
func (e *Encoder) Add1(w io.Writer, b *Array1BlockI16) error {
	if e == nil {
		return ErrNilReceiver
	} else if e.hasReturnedError {
		return ErrPreviouslyReturnedError
	} else if e.colorType != ColorTypeGray {
		e.hasReturnedError = true
		return ErrBadAddNForColorType
	} else if b == nil {
		e.hasReturnedError = true
		return ErrBadArgument
	}
	return e.addN(w, b[:])
}

// Add3 adds an 8×8 group of pixels to a ColorTypeYCbCr444 image.
func (e *Encoder) Add3(w io.Writer, b *Array3BlockI16) error {
	if e == nil {
		return ErrNilReceiver
	} else if e.hasReturnedError {
		return ErrPreviouslyReturnedError
	} else if e.colorType != ColorTypeYCbCr444 {
		e.hasReturnedError = true
		return ErrBadAddNForColorType
	} else if b == nil {
		e.hasReturnedError = true
		return ErrBadArgument
	}
	return e.addN(w, b[:])
}

// Add6 adds a 16×16 group of pixels to a ColorTypeYCbCr420 image.
func (e *Encoder) Add6(w io.Writer, b *Array6BlockI16) error {
	if e == nil {
		return ErrNilReceiver
	} else if e.hasReturnedError {
		return ErrPreviouslyReturnedError
	} else if e.colorType != ColorTypeYCbCr420 {
		e.hasReturnedError = true
		return ErrBadAddNForColorType
	} else if b == nil {
		e.hasReturnedError = true
		return ErrBadArgument
	}
	return e.addN(w, b[:])
}

func (e *Encoder) addN(w io.Writer, blocks []BlockI16) error {
	for i := range blocks {
		if !blocks[i].IsValid() {
			e.hasReturnedError = true
			return ErrInvalidBlockI16
		}
	}
	if e.numAddsRemaining <= 0 {
		e.hasReturnedError = true
		return ErrTooManyAddNCalls
	}
	e.numAddsRemaining--

	// In terms of worst case number of bytes emitted, the up-to-six
	// encodeBlock calls can emit 2688 bytes (6 * 448 bytes).

	whichComponents := ""
	switch ColorType(len(blocks)) {
	case ColorTypeGray:
		whichComponents = "\x00"
	case ColorTypeYCbCr444:
		whichComponents = "\x00\x01\x02"
	case ColorTypeYCbCr420:
		whichComponents = "\x00\x00\x00\x00\x01\x02"
	}

	bufIndex := 0
	for i := range blocks {
		bufIndex = e.encodeBlock(bufIndex, whichComponents[i], &blocks[i])
	}

	if e.numAddsRemaining == 0 {
		bufIndex = e.emitBits(bufIndex, 0x7F, 7)
		e.buf[bufIndex+0] = 0xFF // EOI marker.
		e.buf[bufIndex+1] = 0xD9
		bufIndex += 2
	}

	if _, err := w.Write(e.buf[:bufIndex]); err != nil {
		e.hasReturnedError = true
		return err
	}
	return nil
}

func (e *Encoder) encodeDQT(bufIndex int) int {
	e.buf[bufIndex+0] = 0xFF
	e.buf[bufIndex+1] = 0xDB
	e.buf[bufIndex+2] = 0x00
	if e.colorType == ColorTypeGray {
		e.buf[bufIndex+3] = 0x43
	} else {
		e.buf[bufIndex+3] = 0x84
	}
	bufIndex += 4

	for i := range e.quants {
		e.buf[bufIndex+0] = uint8(i)
		bufIndex += 1
		q := &e.quants[i]
		for z := 0; z < 64; z++ {
			e.buf[bufIndex+z] = q[zigzag[z]]
		}
		bufIndex += 64

		if e.colorType == ColorTypeGray {
			break
		}
	}

	return bufIndex
}

func (e *Encoder) encodeSOF0(bufIndex int, width int, height int) int {
	numComponents := byte(1)
	if e.colorType != ColorTypeGray {
		numComponents = byte(3)
	}

	e.buf[bufIndex+0] = 0xFF
	e.buf[bufIndex+1] = 0xC0
	e.buf[bufIndex+2] = 0x00
	e.buf[bufIndex+3] = 0x08 + (3 * numComponents) // Payload length.
	e.buf[bufIndex+4] = 0x08                       // 8-bit depth.
	e.buf[bufIndex+5] = byte(height >> 8)
	e.buf[bufIndex+6] = byte(height >> 0)
	e.buf[bufIndex+7] = byte(width >> 8)
	e.buf[bufIndex+8] = byte(width >> 0)
	e.buf[bufIndex+9] = numComponents

	if e.colorType == ColorTypeGray {
		e.buf[bufIndex+10] = 0x01 // Component ID 1.
		e.buf[bufIndex+11] = 0x11 // Subsampling.
		e.buf[bufIndex+12] = 0x00 // Quant selector 0.
		return bufIndex + 13
	}

	lumaComponentSubsampling := byte(0x11)
	if e.colorType == ColorTypeYCbCr420 {
		lumaComponentSubsampling = byte(0x22)
	}

	// 4:4:4 subsampling. Luma and Chroma select Quant tables 0 and 1.
	e.buf[bufIndex+10] = 0x01 // Component ID 1.
	e.buf[bufIndex+11] = lumaComponentSubsampling
	e.buf[bufIndex+12] = 0x00 // Quant selector 0.
	e.buf[bufIndex+13] = 0x02 // Component ID 2.
	e.buf[bufIndex+14] = 0x11 // Subsampling.
	e.buf[bufIndex+15] = 0x01 // Quant selector 1.
	e.buf[bufIndex+16] = 0x03 // Component ID 3.
	e.buf[bufIndex+17] = 0x11 // Subsampling.
	e.buf[bufIndex+18] = 0x01 // Quant selector 1.
	return bufIndex + 19
}
func (e *Encoder) encodeDHT(bufIndex int) int {
	s := ""
	if e.colorType == ColorTypeGray {
		s = hardCodedDHTSegments[:len(hardCodedDHTSegments)/2]
	} else {
		s = hardCodedDHTSegments
	}

	n := copy(e.buf[bufIndex:], s)
	return bufIndex + n
}

func (e *Encoder) encodeSOSHeader(bufIndex int) int {
	s := ""
	if e.colorType == ColorTypeGray {
		s = "\xFF\xDA" + // SOS marker.
			"\x00\x08" + // Payload length.
			"\x01" + // Number of components.
			"\x01\x00" + // Component ID 1, Huffman selectors 0 (DC) and 0 (AC).
			"\x00\x3F\x00" // Ss=0, Se=63, Ah=0, Al=0.
	} else {
		s = "\xFF\xDA" + // SOS marker.
			"\x00\x0C" + // Payload length.
			"\x03" + // Number of components.
			"\x01\x00" + // Component ID 1, Huffman selectors 0 (DC) and 0 (AC).
			"\x02\x11" + // Component ID 2, Huffman selectors 1 (DC) and 1 (AC).
			"\x03\x11" + // Component ID 3, Huffman selectors 1 (DC) and 1 (AC).
			"\x00\x3F\x00" // Ss=0, Se=63, Ah=0, Al=0.
	}

	n := copy(e.buf[bufIndex:], s)
	return bufIndex + n
}

// In terms of worst case number of bytes emitted, encodeBlock can emit 448
// bytes (64 * 7 bytes), since there are 64 elements and each element could
// emit 7 bytes.
func (e *Encoder) encodeBlock(bufIndex int, whichComponent byte, b *BlockI16) int {
	whichHuffmanBase := 0
	if whichComponent > 0 {
		whichHuffmanBase = 2
	}

	dc := div(b[0], int16(e.quants[whichHuffmanBase>>1][0]))
	deltaDC := dc - e.prevDC[whichComponent]
	e.prevDC[whichComponent] = dc
	bufIndex = e.emitHuffmanRun(bufIndex, whichHuffmanBase+0, 0, int32(deltaDC))

	zeroesRunLength := uint32(0)
	for z := 1; z < 64; z++ {
		zz := zigzag[z]
		ac := div(b[zz], int16(e.quants[whichHuffmanBase>>1][zz]))
		if ac == 0 {
			zeroesRunLength++
			continue
		}
		for ; zeroesRunLength >= 16; zeroesRunLength -= 16 {
			bufIndex = e.emitHuffman(bufIndex, whichHuffmanBase+1, 0xF0)
		}
		bufIndex = e.emitHuffmanRun(bufIndex, whichHuffmanBase+1, zeroesRunLength, int32(ac))
		zeroesRunLength = 0
	}
	if zeroesRunLength > 0 {
		bufIndex = e.emitHuffman(bufIndex, whichHuffmanBase+1, 0x00)
	}

	return bufIndex
}

// emitHuffmanRun emits (zeroesRunLength, value) as two bitstrings. The first
// bitstring combines zeroesRunLength (also called RRRR) and the "category"
// part of value (also called SSSS). The second bitstring holds the "adjusted
// diff" part of value.
//
// The breakdown from value (a signed integer) into category and adjusted diff
// is a little complicated. Category is an unsigned integer less than 12 and
// smaller category mean values closer to zero. Adjusted diff is an unsigned
// integer less than (1 << category).
//
// Value                             Category   Adjusted Diff
// 0                                        0   0
// -1, +1                                   1   0, 1
// -3, -2, +2, +3                           2   0, 1, 2, 3
// -7, -6, -5, -4, +4, +5, +6, +7           3   0, 1, 2, 3, 4, 5, 6, 7
// -15, -14, ..., -8, +8, ..., +14, +15     4   0, 1, ..., 7, 8, ..., 14, 15
// etc                                    etc
// -2047, ..., -1024, +1024, ..., +2047    11   0, ..., 1023, 1024, ..., 2047
//
// Compare with the JPEG spec, Tables F.1 and F.2, for the distinction between
// "diff" and "adjusted diff".
//
// For example, if value is -5 then category is 3 and adjDiff is 2 (0b010).
//
// For example, if value is -13 then category is 4 and adjDiff is 2 (0b0010).
//
// For example, if value is +15 then category is 4 and adjDiff is 15 (0b1111).
//
// At the bit level, the combined number ((zeroesRunLength << 4) | category),
// also known as ((RRRR << 4) | SSSS) in the spec, is Huffman-encoded. After
// that comes the adjDiff's bits, a bitstring whose length equals category.
//
// For DC values, zeroesRunLength is always zero and what's passed to this
// method is the delta from the previous DC value.
//
// For AC values, zeroesRunLength ranges from 0 up to 15 (inclusive). Category
// 11 is unused. Category 0 is also unused for AC values (as zeroes are run-
// length encoded), other than ((RRRR << 4) | SSSS) values of 0x00 and 0xF0
// being special cases:
//
//   - 0x00 means that all remaining AC coefficients are zero.
//   - 0xF0 means that the next 16 AC coefficients are zero.
//
// Pre-condition: value is in the range [-2047, +2047]. BlockI16.IsValid checks
// for [-1024, +1023] for DC and [-1023, +1023] for AC values. For DC values,
// what's then passed to this method is the delta from the previous DC value.
//
// In terms of worst case number of bytes emitted, emitHuffmanRun can emit 54
// bits, which rounds up to 7 bytes. To calculate that: category can range up
// to 11, so that emitHuffman + emitBits can emit 16 + 11 = 27 bits. Doubling
// that, because of 0xFF 0x00 byte stuffing, produces a worst case of 54.
func (e *Encoder) emitHuffmanRun(bufIndex int, whichHuffman int, zeroesRunLength uint32, value int32) int {
	absValue, adjDiffBeforeMasking := uint32(value), uint32(value)
	if value < 0 {
		absValue, adjDiffBeforeMasking = uint32(-value), uint32(value-1)
	}

	category := uint32(0)
	if absValue < 0x100 {
		category = uint32(bitCount[absValue])
	} else {
		category = uint32(bitCount[absValue>>8]) + 8
	}

	combined := (zeroesRunLength << 4) | category
	bufIndex = e.emitHuffman(bufIndex, whichHuffman, uint8(combined))

	// At this point, we could say
	//
	// adjDiff := adjDiffBeforeMasking & ((1 << category) - 1)
	//
	// but it's equivalent to pass adjDiffBeforeMasking (instead of adjDiff) to
	// emitBits, which does "v &= (1 << n) - 1" early in its function body.
	bufIndex = e.emitBits(bufIndex, adjDiffBeforeMasking, category)

	return bufIndex
}

// emitHuffman emits the Huffman-coded bits for value.
func (e *Encoder) emitHuffman(bufIndex int, whichHuffman int, value uint8) int {
	x := huffmanBitWriters[whichHuffman][value]
	return e.emitBits(bufIndex, x&0xFFFF, x>>16)
}

// emitBits emits the low n bits of v.
func (e *Encoder) emitBits(bufIndex int, v uint32, n uint32) int {
	if n <= 0 {
		return bufIndex
	}

	v &= (1 << n) - 1
	n += e.bitsN
	v <<= 32 - n
	v |= e.bitsV

	for ; n >= 8; n -= 8 {
		vByte := byte(v >> 24)
		e.buf[bufIndex] = vByte
		bufIndex++
		if vByte == 0xFF {
			e.buf[bufIndex] = 0x00
			bufIndex++
		}
		v <<= 8
	}

	e.bitsV = v
	e.bitsN = n
	return bufIndex
}

// div returns a/b rounded to the nearest integer, instead of rounded to zero.
//
// Pre-condition: b > 0.
func div(a, b int16) int16 {
	if a >= 0 {
		return (a + (b >> 1)) / b
	}
	return -((-a + (b >> 1)) / b)
}

// zigzag represents JPEG's zig-zag order for visiting coefficients. If b is a
// BlockI16 then b[zigzag[1]], b[zigzag[2]], b[zigzag[3]], ..., b[zigzag[63]]
// are its AC coefficients ordered from lowest to highest frequency, roughly
// speaking. b[0], which is also b[zigzag[0]], is the DC coefficient.
//
// https://en.wikipedia.org/wiki/File:JPEG_ZigZag.svg
var zigzag = [64]uint8{
	0o00, 0o01, 0o10, 0o20, 0o11, 0o02, 0o03, 0o12, //  0,  1,  8, 16,  9,  2,  3, 10,
	0o21, 0o30, 0o40, 0o31, 0o22, 0o13, 0o04, 0o05, // 17, 24, 32, 25, 18, 11,  4,  5,
	0o14, 0o23, 0o32, 0o41, 0o50, 0o60, 0o51, 0o42, // 12, 19, 26, 33, 40, 48, 41, 34,
	0o33, 0o24, 0o15, 0o06, 0o07, 0o16, 0o25, 0o34, // 27, 20, 13,  6,  7, 14, 21, 28,
	0o43, 0o52, 0o61, 0o70, 0o71, 0o62, 0o53, 0o44, // 35, 42, 49, 56, 57, 50, 43, 36,
	0o35, 0o26, 0o17, 0o27, 0o36, 0o45, 0o54, 0o63, // 29, 22, 15, 23, 30, 37, 44, 51,
	0o72, 0o73, 0o64, 0o55, 0o46, 0o37, 0o47, 0o56, // 58, 59, 52, 45, 38, 31, 39, 46,
	0o65, 0o74, 0o75, 0o66, 0o57, 0o67, 0o76, 0o77, // 53, 60, 61, 54, 47, 55, 62, 63,
}

// bitCount[i] is the smallest n such that i < (1 << n).
var bitCount = [256]byte{
	0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
}
