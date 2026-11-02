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

// Package handsum implements the Handsum image file format.
//
// This is a very lossy format for very small thumbnails. Very small in terms
// of image dimensions, up to 16×16 pixels, but also in terms of file size.
//
// The file format has three color settings (1=Gray, 3=RGB and 4=RGBA) and four
// quality settings. For any given color-and-quality combination, every Handsum
// image file with those settings is a fixed number of bytes.
//
// For color=1, also known as color=Gray:
//
//   - A quality=1 file is  33 bytes long.
//   - A quality=2 file is  51 bytes long.
//   - A quality=3 file is  83 bytes long.
//   - A quality=4 file is  99 bytes long.
//
// For color=3, also known as color=RGB:
//
//   - A quality=1 file is  48 bytes long.
//   - A quality=2 file is  75 bytes long.
//   - A quality=3 file is 123 bytes long.
//   - A quality=4 file is 147 bytes long.
//
// For color=4, also known as color=RGBA:
//
//   - A quality=1 file is  72 bytes long.
//   - A quality=2 file is  99 bytes long.
//   - A quality=3 file is 147 bytes long.
//   - A quality=4 file is 171 bytes long.
//
// Every Gray/q1 image file is exactly 33 bytes (264 bits) long. For a 16×16
// pixel image, this uses 1.03125 bits (0.12890625 bytes) per pixel.
//
// Every RGB/q1 image file is exactly 48 bytes (384 bits) long. For a 16×16
// pixel image, this uses 1.5 bits (0.1875 bytes) per pixel.
//
// Every RGB/q4 image file is exactly 147 bytes (1176 bits) long. This uses
// 4.59375 bits per pixel for a 16×16 pixel image (a 1:1 aspect ratio), or
// 6.125 bits per pixel for a 16×12 pixel image (a 4:3 aspect ratio).
//
// Handsum files start with a 3 byte header: a 15-bit magic signature, a 2-bit
// color (Gray, RGB or RGBA), a 2-bit quality and a 5-bit aspect ratio. An
// image's longest dimension (width or height) is 16 pixels and the aspect
// ratio gives the shorter dimension.
//
// The color=RGB payload, after the header, holds a scaled 16×16 pixel YCbCr
// 4:2:0 JPEG MCU (Minimum Coded Unit), 4 Luma and 2 Chroma blocks. Each block
// is 8×8 pixels.
//
// The color=RGB payload ends with 2 Chroma blocks in 15, 24, 40 or 48 bytes
// (depending on the quality setting). For a grayscale image, the encoded
// Chroma blocks' bytes are all 0x88, and the color=Gray payload is just the 4
// Luma blocks without explicitly recording the 2 Chroma blocks.
//
// The color=RGBA payload extends the color=RGB payload with one more 8×8 Alpha
// block. That seventh block is always encoded in 24 bytes (as if quality=4).
//
// Each 8×8 block is sub-divided into 8×8 (for q1), 4×4 (for q2 and q3) or 2×2
// (for q4) tiles. DCT (Discrete Cosine Transform) is applied to each tile,
// producing 64, 16, 16 or 4 DCT coefficients (depending on the quality). Only
// the 15 (out of 64), 6 (out of 16), 10 (out of 16) or 3 (out of 4) lowest
// frequency DCT coefficients are kept for each tile. Lowest frequency means
// the top-left corner in the usual visualization of JPEG's zig-zag ordering.
//
// In percentage terms, the quality setting keeps 23%, 38%, 63% or 75% of the
// 64 DCT coefficients in each 8×8 block.
//
// Each DCT coefficient is encoded as one nibble (4 bits) with fixed bias and
// quantization factors.
//
// All Handsum images use the sRGB color profile and non-premultiplied alpha.
//
// The "Handsum" name was inspired by the "Thumbhash" image file format (which
// was in turn inspired by "Blurhash"), which is also designed for very small
// thumbnails (or very compact representations of image placeholders). Handsum
// files are bigger (but better quality) than Thumbhash (and Blurhash).
// "Handsum" also sounds like "handsome", meaning "good looking".
//
// Other techniques and image formats, similar to Thumbhash, can be found by
// search for "LQIP" or "Low Quality Image Placeholders".
package handsum

// When decoding, each nibble produces a DCT coefficient according to a simple
// linear formula, with different parameters (the linear slope and intercept)
// depending on DC vs AC coefficients and on the quality setting (whether the
// tiles are 8×8, 4×4 or 2×2):
//
// ======    ======== DC ========    ======== AC ========
// Bucket      q1   q2,q3      q4      q1   q2,q3      q4
//
// 0x0      -1024    -512    -256    -512    -256    -128
// 0x1       -888    -444    -222    -448    -224    -112
// 0x2       -752    -376    -188    -384    -192     -96
// 0x3       -616    -308    -154    -320    -160     -80
// ...        ...     ...     ...     ...     ...     ...
// 0x7        -72     -36     -18     -64     -32     -16
// 0x8        +64     +32     +16       0       0       0
// 0x9       +200    +100     +50     +64     +32     +16
// ...        ...     ...     ...     ...     ...     ...
// 0xE       +880    +440    +220    +384    +192     +96
// 0xF      +1016    +508    +254    +448    +224    +112
//
// Width     0x88    0x44    0x22    0x40    0x20    0x10
// TileSize     8       4       2       8       4       2
// ======    ======== DC ========    ======== AC ========
//
// DC bucket widths are multiples of 0x11, not 0x10 with bucket=0x8 mapping to
// zero, so that an input tile that is pure black (luma=0x00) or pure white
// (luma=0xFF) can round-trip losslessly (as bucket 0x0 or bucket 0xF). AC maps
// bucket=0x8 to zero, so that a uniformly-valued tile (all-0x00, all-0x11,
// all-0x22, ..., all-0xFF) can again round-trip losslessly.
//
// This encoder uses smaller bucket widths for the AC coefficients, compared to
// those used when decoding (42, 18, 8 instead of 64, 32, 16). This adjustment
// is arbitrary but the results seem a little more vibrant. It also compensates
// somewhat for the mean-reversion (gray-reversion) from the decoder's seam
// smoothing, a localized blur that mitigates artifacts at tile boundaries.
//
// Luma values can span the full range [0x00, 0xFF] but decoded Chroma values
// are restricted to a narrower range [0x3C, 0xBB], as extreme Chroma values
// are rare in practice. The latter range has a slight bias (not centred on the
// neutral 0x80) so that the Chroma DC components of a neutral gray image can
// round-trip losslessly (in the 0x8 bucket). Chroma is narrowed (after Inverse
// DCT) on decode and so is widened (before Forward DCT) on encode. Luma,
// wide-Chroma and Alpha then use the same DCT buckets.
//
// For Alpha values only, to encourage producing 0x00 (fully transparent) or
// 0xFF (fully opaque) values, we transform (on decode) the values towards the
// extreme ends via a cube root mapping.
//
// There are 16 possible landscape aspect ratios (and likewise for portrait):
//
// 16 :  1   ≈  16.000 : 1
// 16 :  2   ≈   8.000 : 1
// 16 :  3   ≈   5.333 : 1
// 16 :  4   ≈   4.000 : 1
// 16 :  5   ≈   3.200 : 1
// 16 :  6   ≈   2.667 : 1
// 16 :  7   ≈   2.286 : 1
// 16 :  8   ≈   2.000 : 1
// 16 :  9   ≈   1.778 : 1
// 16 : 10   ≈   1.600 : 1
// 16 : 11   ≈   1.455 : 1
// 16 : 12   ≈   1.333 : 1
// 16 : 13   ≈   1.231 : 1
// 16 : 14   ≈   1.143 : 1
// 16 : 15   ≈   1.067 : 1
// 16 : 16   ≈   1.000 : 1

import (
	"errors"
	"image"
	"image/color"
	"io"

	"github.com/google/wuffs/lib/lowleveldct4x4"
	"github.com/google/wuffs/lib/lowleveljpeg"

	"golang.org/x/image/draw"
)

// MaxDimension is the maximum (inclusive) width or height of every Handsum
// image file.
//
// Every image is either (W × 16) or (16 × H) or both, for some positive W or H
// that is no greater than 16.
const MaxDimension = 16

// MagicEtc is the byte string prefix of every Handsum image file. A Gray image
// starts with Magic0. An RGB or RGBA image starts with Magic1. The two 16-bit
// strings only differ in their final bit.
//
// It's like how every JPEG image file starts with "\xFF\xD8".
const (
	Magic0 = "\xFE\xD6"
	Magic1 = "\xFE\xD7"
)

func init() {
	image.RegisterFormat("handsum", Magic0, Decode, DecodeConfig)
	image.RegisterFormat("handsum", Magic1, Decode, DecodeConfig)
}

var (
	ErrBadArgument     = errors.New("handsum: bad argument")
	ErrNotAHandsumFile = errors.New("handsum: not a handsum file")
)

const (
	fileSizeHeader = 3
	fileSizeMax    = 171
)

func fileSize(c Color, q Quality) int {
	// 33, 51,  83,  99.
	//  0,  0,   0,   0.
	// 48, 75, 123, 147.
	// 72, 99, 147, 171.
	return int(("" +
		"\x21\x33\x53\x63" +
		"\x00\x00\x00\x00" +
		"\x30\x4B\x7B\x93" +
		"\x48\x63\x93\xAB")[(int((c-1)&3)<<2)|int((q-1)&3)])
}

// Color is a Handsum image's color setting, either 1 (Gray), 3 (RGB) or 4
// (RGBA).
type Color uint8

const (
	ColorGray = Color(1)
	ColorRGB  = Color(3)
	ColorRGBA = Color(4)
)

func (c Color) numberOfBlocks() int {
	if c == ColorGray {
		return 4
	}
	return 6
}

// Quality is a Handsum image's quality setting, from 1 (worst) to 4 (best).
//
// "Best" is relative to the other settings. In absolute terms, Handsum's image
// quality ranges from "potato" (best) to "extremely potato" (worst).
type Quality uint8

const (
	QualityWorst      = Quality(1)
	QualityMediumLow  = Quality(2)
	QualityMediumHigh = Quality(3)
	QualityBest       = Quality(4)
)

func (q Quality) numberOfCoefficients() int {
	// 15 (out of 64), 6 (out of 16), 10 (out of 16), 3 (out of 4).
	return int("\x0F\x06\x0A\x03"[(q-1)&3])
}

// EncodeOptions are optional arguments to Encode. The zero value is valid and
// means to use the default configuration, encoding to 147 bytes.
type EncodeOptions struct {
	// Color is the color setting. The zero value means to use the default,
	// ColorRGB.
	Color Color

	// Quality is the quality-versus-file-size setting. The zero value means to
	// use the default, QualityBest (which is also the largest file size).
	Quality Quality
}

func (o *EncodeOptions) color() Color {
	if o != nil {
		switch o.Color {
		case ColorGray, ColorRGB, ColorRGBA:
			return o.Color
		}
	}
	return ColorRGB
}

func (o *EncodeOptions) quality() Quality {
	if o != nil {
		switch o.Quality {
		case QualityWorst, QualityMediumLow, QualityMediumHigh, QualityBest:
			return o.Quality
		}
	}
	return QualityBest
}

// Encode writes src to w in the Handsum format.
//
// options may be nil, which means to use the default configuration.
func Encode(w io.Writer, src image.Image, options *EncodeOptions) error {
	if (w == nil) || (src == nil) {
		return ErrBadArgument
	}
	srcB := src.Bounds()
	srcW, srcH := srcB.Dx(), srcB.Dy()
	if (srcW <= 0) || (srcH <= 0) {
		return ErrBadArgument
	}

	aspectRatio := byte(0)
	if srcW >= srcH { // Landscape.
		a := ((int64(srcH) * 32) + int64(srcW)) / (2 * int64(srcW))
		if a <= 0 {
			a = 1
		}
		aspectRatio = byte(a-1) | 0x00
	} else { // Portrait.
		a := ((int64(srcW) * 32) + int64(srcH)) / (2 * int64(srcH))
		if a <= 0 {
			a = 1
		}
		aspectRatio = byte(a-1) | 0x10
		if aspectRatio == 0x1F { // Reserved for future expansion.
			aspectRatio = 0x0F
		}
	}

	alphasQuadBlock := lowleveljpeg.QuadBlockU8{}
	dst := scaleSrc(src, &alphasQuadBlock)
	dstU8s := lowleveljpeg.Array6BlockU8{}
	dstU8s.ExtractYCbCrFrom(dst, 0, 0)

	// Biasing the Chroma blocks by +8 shifts the neutral (gray) Chroma values
	// from 0x80 to 0x88. DC coefficient bias-and-quantization can losslessly
	// encode a block whose elements are a uniform multiple of 0x11.
	scaleAndBiasChromaUp(&dstU8s[4])
	scaleAndBiasChromaUp(&dstU8s[5])

	c := options.color()
	q := options.quality()
	buf := [fileSizeMax]byte{}
	buf[0] = Magic0[0]
	buf[1] = Magic0[1] | uint8((c-1)>>1)
	buf[2] = ((uint8(c - 1)) << 7) | ((uint8(q - 1)) << 5) | aspectRatio

	bitOffset := 3 * 8
	encodeBlock := encodeBlockFuncs[(q-1)&3]
	for i := range c.numberOfBlocks() {
		bitOffset = encodeBlock(&buf, bitOffset, &dstU8s[i], q.numberOfCoefficients())
	}

	if c >= ColorRGBA {
		alphasBlock := lowleveljpeg.BlockU8{}
		alphasBlock.DownsampleFrom(&alphasQuadBlock)
		bitOffset = encodeBlockQ4(&buf, bitOffset, &alphasBlock, QualityBest.numberOfCoefficients())
	}

	_, err := w.Write(buf[:bitOffset/8])
	return err
}

func scaleSrc(src image.Image, alphasQuadBlock *lowleveljpeg.QuadBlockU8) image.Image {
	if o, ok := src.(interface{ Opaque() bool }); ok && o.Opaque() {
		for i := range alphasQuadBlock {
			alphasQuadBlock[i] = 0xFF
		}
		dst := image.NewRGBA(image.Rectangle{Max: image.Point{X: 16, Y: 16}})
		draw.BiLinear.Scale(dst, dst.Bounds(), src, src.Bounds(), draw.Src, nil)
		return dst
	}

	dst := image.NewNRGBA(image.Rectangle{Max: image.Point{X: 16, Y: 16}})
	draw.BiLinear.Scale(dst, dst.Bounds(), src, src.Bounds(), draw.Src, nil)

	// Extract alphasQuadBlock (as a side effect of calling this function) and
	// set dst's alpha values to 0xFF.
	for i := range alphasQuadBlock {
		v := dst.Pix[(4*i)+3]
		dst.Pix[(4*i)+3] = 0xFF
		alphasQuadBlock[i] = v
	}

	return &image.RGBA{ // Re-interpret NRGBA as RGBA.
		Pix:    dst.Pix,
		Stride: dst.Stride,
		Rect:   dst.Rect,
	}
}

type encodeBlockFunc func(buf *[fileSizeMax]byte, bitOffset int, src *lowleveljpeg.BlockU8, nCoeffs int) int

var encodeBlockFuncs = [4]encodeBlockFunc{
	encodeBlockQ1,
	encodeBlockQ2,
	encodeBlockQ2,
	encodeBlockQ4,
}

func encodeBlockQ1(buf *[fileSizeMax]byte, bitOffset int, src *lowleveljpeg.BlockU8, nCoeffs int) int {
	f8 := src.ForwardDCT()

	for _, z := range zigzag8 {
		v := int32(f8[z])
		if z == 0 {
			v = (v + 1024 + 0x44) / 0x88
		} else {
			v = (v + (8.5 * 42)) / 42
		}
		buf[bitOffset>>3] |= nibblify(v) << (bitOffset & 4)
		bitOffset += 4
	}

	return bitOffset
}

func encodeBlockQ2(buf *[fileSizeMax]byte, bitOffset int, src *lowleveljpeg.BlockU8, nCoeffs int) int {
	for i := range 4 {
		x4 := (i & 1) << 2  // The sequence 0, 4, 0, 4.
		y4 := (i & 2) << 1  // The sequence 0, 0, 4, 4.
		o4 := (8 * y4) + x4 // The sequence 0, 4, 32, 36.

		b4 := extract4x4Tile(src, o4)
		f4 := b4.ForwardDCT()

		for _, z := range zigzag4[:nCoeffs] {
			v := int32(f4[z])
			if z == 0 {
				v = (v + 512 + 0x22) / 0x44
			} else {
				v = (v + (8.5 * 18)) / 18
			}
			buf[bitOffset>>3] |= nibblify(v) << (bitOffset & 4)
			bitOffset += 4
		}
	}

	return bitOffset
}

func encodeBlockQ4(buf *[fileSizeMax]byte, bitOffset int, src *lowleveljpeg.BlockU8, nCoeffs int) int {
	// We could import the lowleveldct2x2 package and call its ForwardDCT
	// method (and InverseDCTFrom on decode), but it's easy to just inline it.

	for i := range 16 {
		x := 2 * (i & 3)
		y := 2 * (i >> 2)
		j := (8 * y) + x

		s0 := int32(src[j+0]) - 0x80
		s1 := int32(src[j+1]) - 0x80
		s2 := int32(src[j+8]) - 0x80
		s3 := int32(src[j+9]) - 0x80

		dct0 := (+s0 + s1 + s2 + s3 + 1) >> 1
		dct1 := (+s0 - s1 + s2 - s3 + 1) >> 1
		dct2 := (+s0 + s1 - s2 - s3 + 1) >> 1

		v0 := (dct0 + 256 + 0x11) / 0x22
		buf[bitOffset>>3] |= nibblify(v0) << (bitOffset & 4)
		bitOffset += 4

		v1 := (dct1 + (8.5 * 8)) / 8
		buf[bitOffset>>3] |= nibblify(v1) << (bitOffset & 4)
		bitOffset += 4

		v2 := (dct2 + (8.5 * 8)) / 8
		buf[bitOffset>>3] |= nibblify(v2) << (bitOffset & 4)
		bitOffset += 4
	}

	return bitOffset
}

// DecodeConfig reads a Handsum image configuration from r.
func DecodeConfig(r io.Reader) (image.Config, error) {
	buf := [fileSizeHeader]byte{}
	if _, err := io.ReadFull(r, buf[:]); err != nil {
		return image.Config{}, err
	} else if (buf[0] != Magic0[0]) || ((0xFE & buf[1]) != Magic0[1]) {
		return image.Config{}, ErrNotAHandsumFile
	}
	c, ok := decodeColorSetting(buf[1], buf[2])
	if !ok {
		return image.Config{}, ErrNotAHandsumFile
	}
	w, h, ok := decodeWidthAndHeight(buf[2])
	if !ok {
		return image.Config{}, ErrNotAHandsumFile
	}

	cm := color.GrayModel
	if c == ColorRGB {
		cm = color.RGBAModel
	} else if c == ColorRGBA {
		cm = color.NRGBAModel
	}

	return image.Config{
		ColorModel: cm,
		Width:      w,
		Height:     h,
	}, nil
}

func decodeColorSetting(buf1 byte, buf2 byte) (Color, bool) {
	switch ((buf1 & 1) << 1) | (buf2 >> 7) {
	case 0:
		return ColorGray, true
	case 2:
		return ColorRGB, true
	case 3:
		return ColorRGBA, true
	}
	return 0, false
}

// Decode reads a Handsum image from r.
func Decode(r io.Reader) (image.Image, error) {
	buf := [fileSizeMax]byte{}
	if _, err := io.ReadFull(r, buf[:fileSizeHeader]); err != nil {
		return nil, err
	} else if (buf[0] != Magic0[0]) || ((0xFE & buf[1]) != Magic0[1]) {
		return nil, ErrNotAHandsumFile
	}
	c, ok := decodeColorSetting(buf[1], buf[2])
	if !ok {
		return nil, ErrNotAHandsumFile
	}
	q := Quality((buf[2]>>5)&0x03) + 1
	if _, err := io.ReadFull(r, buf[fileSizeHeader:fileSize(c, q)]); err != nil {
		return nil, err
	}
	dstW, dstH, ok := decodeWidthAndHeight(buf[2])
	if !ok {
		return nil, ErrNotAHandsumFile
	}

	bitOffset := 3 * 8
	decodeBlock := decodeBlockFuncs[(q-1)&3]
	lumaQuadBlockU8 := lowleveljpeg.QuadBlockU8{}
	bitOffset = decodeBlock(lumaQuadBlockU8[0x00:], 16, &buf, bitOffset, q.numberOfCoefficients())
	bitOffset = decodeBlock(lumaQuadBlockU8[0x08:], 16, &buf, bitOffset, q.numberOfCoefficients())
	bitOffset = decodeBlock(lumaQuadBlockU8[0x80:], 16, &buf, bitOffset, q.numberOfCoefficients())
	bitOffset = decodeBlock(lumaQuadBlockU8[0x88:], 16, &buf, bitOffset, q.numberOfCoefficients())
	smoothBlockSeams16x16(&lumaQuadBlockU8)

	cbQuadBlockU8 := lowleveljpeg.QuadBlockU8{}
	crQuadBlockU8 := lowleveljpeg.QuadBlockU8{}
	aaQuadBlockU8 := lowleveljpeg.QuadBlockU8{}
	for i := range aaQuadBlockU8 {
		aaQuadBlockU8[i] = 0xFF
	}

	if c >= ColorRGB {
		cbBlockU8 := lowleveljpeg.BlockU8{}
		crBlockU8 := lowleveljpeg.BlockU8{}

		bitOffset = decodeBlock(cbBlockU8[:], 8, &buf, bitOffset, q.numberOfCoefficients())
		bitOffset = decodeBlock(crBlockU8[:], 8, &buf, bitOffset, q.numberOfCoefficients())
		scaleAndBiasChromaDown(&cbBlockU8)
		scaleAndBiasChromaDown(&crBlockU8)

		cbQuadBlockU8.UpsampleFrom(&cbBlockU8)
		crQuadBlockU8.UpsampleFrom(&crBlockU8)

		if c >= ColorRGBA {
			aaBlockU8 := lowleveljpeg.BlockU8{}
			bitOffset = decodeBlockQ4(aaBlockU8[:], 8, &buf, bitOffset, QualityBest.numberOfCoefficients())
			aaQuadBlockU8.UpsampleFrom(&aaBlockU8)
			for i, v := range aaQuadBlockU8 {
				aaQuadBlockU8[i] = cubeRootLUT[v]
			}
		}
	}

	return finishDecode(dstW, dstH, c, &lumaQuadBlockU8, &cbQuadBlockU8, &crQuadBlockU8, &aaQuadBlockU8), nil
}

func decodeWidthAndHeight(buf2 byte) (w int, h int, ok bool) {
	if (buf2 & 0x1F) == 0x1F {
		return 0, 0, false
	} else if (buf2 & 0x10) == 0x00 { // Landscape.
		w = 16
		h = 1 + int(buf2&0x0F)
	} else { // Portrait.
		w = 1 + int(buf2&0x0F)
		h = 16
	}
	return w, h, true
}

type decodeBlockFunc func(dst []byte, stride int, buf *[fileSizeMax]byte, bitOffset int, nCoeffs int) int

var decodeBlockFuncs = [4]decodeBlockFunc{
	decodeBlockQ1,
	decodeBlockQ2,
	decodeBlockQ2,
	decodeBlockQ4,
}

func decodeBlockQ1(dst []byte, stride int, buf *[fileSizeMax]byte, bitOffset int, nCoeffs int) int {
	a := lowleveljpeg.BlockI16{}

	for _, z := range zigzag8 {
		nibble := (buf[bitOffset>>3] >> (bitOffset & 4)) & 15
		if z == 0 {
			a[z] = int16((int32(nibble) * 0x88) - 1024)
		} else {
			a[z] = int16((int32(nibble) * 0x40) - 512)
		}
		bitOffset += 4
	}

	b := lowleveljpeg.BlockU8{}
	b.InverseDCTFrom(&a)

	for i := range 8 {
		di := i * stride
		bi := i * 8
		copy(dst[di:di+8], b[bi:bi+8])
	}

	return bitOffset
}

func decodeBlockQ2(dst []byte, stride int, buf *[fileSizeMax]byte, bitOffset int, nCoeffs int) int {
	tmp := lowleveljpeg.BlockU8{}

	for i := range 4 {
		a := lowleveldct4x4.BlockI16{}

		for _, z := range zigzag4[:nCoeffs] {
			nibble := (buf[bitOffset>>3] >> (bitOffset & 4)) & 15
			if z == 0 {
				a[z] = int16((int32(nibble) * 0x44) - 512)
			} else {
				a[z] = int16((int32(nibble) * 0x20) - 256)
			}
			bitOffset += 4
		}

		b := lowleveldct4x4.BlockU8{}
		b.InverseDCTFrom(&a)

		x4 := (i & 1) << 2 // The sequence 0, 4, 0, 4.
		y4 := (i & 2) << 1 // The sequence 0, 0, 4, 4.

		for i := range 4 {
			ti := ((i + y4) * 8) + x4
			bi := i * 4
			copy(tmp[ti:ti+4], b[bi:bi+4])
		}
	}

	smoothBlockSeams8x8Q2(&tmp)

	for i := range 8 {
		di := i * stride
		ti := i * 8
		copy(dst[di:di+8], tmp[ti:ti+8])
	}

	return bitOffset
}

func decodeBlockQ4(dst []byte, stride int, buf *[fileSizeMax]byte, bitOffset int, nCoeffs int) int {
	tmp := lowleveljpeg.BlockU8{}

	for i := range 16 {
		nibble0 := (buf[bitOffset>>3] >> (bitOffset & 4)) & 15
		dct0 := (int32(nibble0) * 0x22) - 256
		bitOffset += 4

		nibble1 := (buf[bitOffset>>3] >> (bitOffset & 4)) & 15
		dct1 := (int32(nibble1) * 0x10) - 128
		bitOffset += 4

		nibble2 := (buf[bitOffset>>3] >> (bitOffset & 4)) & 15
		dct2 := (int32(nibble2) * 0x10) - 128
		bitOffset += 4

		x2 := 2 * (i & 3)   // The sequence 0, 2, 4, 6,  0,  2,  4, ...,  6.
		y2 := 2 * (i >> 2)  // The sequence 0, 0, 0, 0,  2,  2,  2, ...,  6.
		o2 := (y2 * 8) + x2 // The sequence 0, 2, 4, 6, 16, 18, 20, ..., 54.

		tmp[o2+0] = biasAndClamp[((+dct0+dct1+dct2+1)>>1)&1023]
		tmp[o2+1] = biasAndClamp[((+dct0-dct1+dct2+1)>>1)&1023]
		tmp[o2+8] = biasAndClamp[((+dct0+dct1-dct2+1)>>1)&1023]
		tmp[o2+9] = biasAndClamp[((+dct0-dct1-dct2+1)>>1)&1023]
	}

	smoothBlockSeams8x8Q4(&tmp)

	for i := range 8 {
		di := i * stride
		ti := i * 8
		copy(dst[di:di+8], tmp[ti:ti+8])
	}

	return bitOffset
}

func finishDecode(w int,
	h int,
	c Color,
	yy *lowleveljpeg.QuadBlockU8,
	cb *lowleveljpeg.QuadBlockU8,
	cr *lowleveljpeg.QuadBlockU8,
	aa *lowleveljpeg.QuadBlockU8) image.Image {

	tmp := [1024]byte{}

	if c == ColorGray {
		for i := range 256 {
			tmp[(4 * i)] = yy[i]
		}
	} else {
		for i := range 256 {
			tmp[(4*i)+0], tmp[(4*i)+1], tmp[(4*i)+2] =
				color.YCbCrToRGB(yy[i], cb[i], cr[i])
			tmp[(4*i)+3] = aa[i]
		}
	}

	pix := tmp[:]
	if w < 16 {
		pix = scaleHorizontal(&tmp, uint32(w))
	} else if h < 16 {
		pix = scaleVertical(&tmp, uint32(h))
	}

	if c == ColorGray {
		return &image.Gray{
			Pix:    convertYxxxToY(pix),
			Stride: w,
			Rect:   image.Rect(0, 0, w, h),
		}

	} else if c == ColorRGB {
		return &image.RGBA{
			Pix:    pix,
			Stride: 4 * w,
			Rect:   image.Rect(0, 0, w, h),
		}
	}

	return &image.NRGBA{
		Pix:    pix,
		Stride: 4 * w,
		Rect:   image.Rect(0, 0, w, h),
	}
}

func scaleHorizontal(src *[1024]byte, w uint32) []byte {
	dst := make([]byte, 64*w)

	for y := range 16 {
		dstx := 0
		acc0 := uint32(0)
		acc1 := uint32(0)
		acc2 := uint32(0)
		acc3 := uint32(0)
		remainder := uint32(16)
		for srcx := range 16 {
			si := (64 * y) + (4 * srcx)
			s0 := uint32(src[si+0])
			s1 := uint32(src[si+1])
			s2 := uint32(src[si+2])
			s3 := uint32(src[si+3])

			if remainder > w {
				remainder -= w
				acc0 += w * s0
				acc1 += w * s1
				acc2 += w * s2
				acc3 += w * s3

			} else {
				acc0 += remainder * s0
				acc1 += remainder * s1
				acc2 += remainder * s2
				acc3 += remainder * s3

				di := (4 * int(w) * y) + (4 * dstx)
				dst[di+0] = uint8((acc0 + 8) / 16)
				dst[di+1] = uint8((acc1 + 8) / 16)
				dst[di+2] = uint8((acc2 + 8) / 16)
				dst[di+3] = uint8((acc3 + 8) / 16)
				dstx++

				partial := w - remainder

				acc0 = partial * s0
				acc1 = partial * s1
				acc2 = partial * s2
				acc3 = partial * s3

				remainder = 16 - partial
			}
		}
	}

	return dst
}

func scaleVertical(src *[1024]byte, h uint32) []byte {
	dst := make([]byte, 64*h)

	for x := range 16 {
		dsty := 0
		acc0 := uint32(0)
		acc1 := uint32(0)
		acc2 := uint32(0)
		acc3 := uint32(0)
		remainder := uint32(16)
		for srcy := range 16 {
			si := (64 * srcy) + (4 * x)
			s0 := uint32(src[si+0])
			s1 := uint32(src[si+1])
			s2 := uint32(src[si+2])
			s3 := uint32(src[si+3])

			if remainder > h {
				remainder -= h
				acc0 += h * s0
				acc1 += h * s1
				acc2 += h * s2
				acc3 += h * s3

			} else {
				acc0 += remainder * s0
				acc1 += remainder * s1
				acc2 += remainder * s2
				acc3 += remainder * s3

				di := (64 * dsty) + (4 * x)
				dst[di+0] = uint8((acc0 + 8) / 16)
				dst[di+1] = uint8((acc1 + 8) / 16)
				dst[di+2] = uint8((acc2 + 8) / 16)
				dst[di+3] = uint8((acc3 + 8) / 16)
				dsty++

				partial := h - remainder

				acc0 = partial * s0
				acc1 = partial * s1
				acc2 = partial * s2
				acc3 = partial * s3

				remainder = 16 - partial
			}
		}
	}

	return dst
}

func convertYxxxToY(pix []byte) []byte {
	ret := make([]byte, len(pix)/4)
	for i := range ret {
		ret[i] = pix[4*i]
	}
	return ret
}

// zigzag8 represents JPEG's zig-zag order for visiting DCT coefficients.
// QualityWorst only uses the first (1 + 2 + 3 + 4 + 5) = 15 of JPEG's 64 DCT
// coefficients.
//
// https://en.wikipedia.org/wiki/File:JPEG_ZigZag.svg
var zigzag8 = [15]uint8{
	0o00, 0o01, 0o10, 0o20, 0o11, 0o02, 0o03, 0o12, //  0,  1,  8, 16,  9,  2,  3, 10,
	0o21, 0o30, 0o40, 0o31, 0o22, 0o13, 0o04, //       17, 24, 32, 25, 18, 11,  4,
}

// zigzag4 is like zigzag8 but using the first (1 + 2 + 3) = 6 or (1 + 2 + 3 +
// 4) = 10 (instead of 15) DCT coefficients of a 4×4 (instead of 8×8) block.
var zigzag4 = [10]uint8{
	0o00, 0o01, 0o04, 0o10, 0o05, 0o02, 0o03, 0o06, //  0,  1,  4,  8,  5,  2,  3,  6,
	0o11, 0o14, //                                      9, 12,
}

// nibblify clamps v to the range [0x0, 0xF].
func nibblify(v int32) uint8 {
	return uint8(max(0x0, min(0xF, v)))
}

// extract4x4Tile extracts a 4×4 tile out of an 8×8 block.
func extract4x4Tile(b8 *lowleveljpeg.BlockU8, offset int) lowleveldct4x4.BlockU8 {
	return lowleveldct4x4.BlockU8{
		b8[offset+0x00],
		b8[offset+0x01],
		b8[offset+0x02],
		b8[offset+0x03],
		b8[offset+0x08],
		b8[offset+0x09],
		b8[offset+0x0A],
		b8[offset+0x0B],
		b8[offset+0x10],
		b8[offset+0x11],
		b8[offset+0x12],
		b8[offset+0x13],
		b8[offset+0x18],
		b8[offset+0x19],
		b8[offset+0x1A],
		b8[offset+0x1B],
	}
}

func smoothBlockSeams16x16(b *lowleveljpeg.QuadBlockU8) {
	for _, pair := range smoothingPairs16x16 {
		v0 := uint32(b[pair[0]])
		v1 := uint32(b[pair[1]])
		b[pair[0]] = uint8(((3 * v0) + v1 + 2) / 4)
		b[pair[1]] = uint8(((3 * v1) + v0 + 2) / 4)
	}

	v77 := uint32(b[0x77])
	v78 := uint32(b[0x78])
	v88 := uint32(b[0x88])
	v87 := uint32(b[0x87])

	b[0x77] = uint8(((9 * v77) + (3 * v78) + v88 + (3 * v87) + 8) / 16)
	b[0x78] = uint8(((9 * v78) + (3 * v88) + v87 + (3 * v77) + 8) / 16)
	b[0x88] = uint8(((9 * v88) + (3 * v87) + v77 + (3 * v78) + 8) / 16)
	b[0x87] = uint8(((9 * v87) + (3 * v77) + v78 + (3 * v88) + 8) / 16)
}

func smoothBlockSeams8x8Q2(b *lowleveljpeg.BlockU8) {
	for _, pair := range smoothingPairs8x8 {
		v0 := uint32(b[pair[0]])
		v1 := uint32(b[pair[1]])
		b[pair[0]] = uint8(((3 * v0) + v1 + 2) / 4)
		b[pair[1]] = uint8(((3 * v1) + v0 + 2) / 4)
	}

	v33 := uint32(b[0o33])
	v34 := uint32(b[0o34])
	v44 := uint32(b[0o44])
	v43 := uint32(b[0o43])

	b[0o33] = uint8(((9 * v33) + (3 * v34) + v44 + (3 * v43) + 8) / 16)
	b[0o34] = uint8(((9 * v34) + (3 * v44) + v43 + (3 * v33) + 8) / 16)
	b[0o44] = uint8(((9 * v44) + (3 * v43) + v33 + (3 * v34) + 8) / 16)
	b[0o43] = uint8(((9 * v43) + (3 * v33) + v34 + (3 * v44) + 8) / 16)
}

func smoothBlockSeams8x8Q4(b *lowleveljpeg.BlockU8) {
	for y := 1; y < 7; y += 2 {
		for x := 1; x < 7; x += 2 {
			o := (y * 8) + x

			v0 := uint32(b[o+0])
			v1 := uint32(b[o+1])
			v9 := uint32(b[o+9])
			v8 := uint32(b[o+8])

			b[o+0] = uint8(((9 * v0) + (3 * v1) + v9 + (3 * v8) + 8) / 16)
			b[o+1] = uint8(((9 * v1) + (3 * v9) + v8 + (3 * v0) + 8) / 16)
			b[o+9] = uint8(((9 * v9) + (3 * v8) + v0 + (3 * v1) + 8) / 16)
			b[o+8] = uint8(((9 * v8) + (3 * v0) + v1 + (3 * v9) + 8) / 16)
		}

		{
			v0 := uint32(b[0o00+y])
			v1 := uint32(b[0o01+y])
			b[0o00+y] = uint8(((3 * v0) + v1 + 2) / 4)
			b[0o01+y] = uint8(((3 * v1) + v0 + 2) / 4)
		}

		{
			v0 := uint32(b[0o70+y])
			v1 := uint32(b[0o71+y])
			b[0o70+y] = uint8(((3 * v0) + v1 + 2) / 4)
			b[0o71+y] = uint8(((3 * v1) + v0 + 2) / 4)
		}

		y8 := y * 8

		{
			v0 := uint32(b[0o00+y8])
			v1 := uint32(b[0o10+y8])
			b[0o00+y8] = uint8(((3 * v0) + v1 + 2) / 4)
			b[0o10+y8] = uint8(((3 * v1) + v0 + 2) / 4)
		}

		{
			v0 := uint32(b[0o07+y8])
			v1 := uint32(b[0o17+y8])
			b[0o07+y8] = uint8(((3 * v0) + v1 + 2) / 4)
			b[0o17+y8] = uint8(((3 * v1) + v0 + 2) / 4)
		}
	}
}

// smoothingPairs16x16 are the seams of the four 8×8 Luma blocks in a 16×16
// MCU. The central 4 pixels are handled separately.
var smoothingPairs16x16 = [28][2]uint8{
	{0x07, 0x08},
	{0x17, 0x18},
	{0x27, 0x28},
	{0x37, 0x38},
	{0x47, 0x48},
	{0x57, 0x58},
	{0x67, 0x68},

	{0x70, 0x80},
	{0x71, 0x81},
	{0x72, 0x82},
	{0x73, 0x83},
	{0x74, 0x84},
	{0x75, 0x85},
	{0x76, 0x86},

	{0x79, 0x89},
	{0x7A, 0x8A},
	{0x7B, 0x8B},
	{0x7C, 0x8C},
	{0x7D, 0x8D},
	{0x7E, 0x8E},
	{0x7F, 0x8F},

	{0x97, 0x98},
	{0xA7, 0xA8},
	{0xB7, 0xB8},
	{0xC7, 0xC8},
	{0xD7, 0xD8},
	{0xE7, 0xE8},
	{0xF7, 0xF8},
}

// smoothingPairs8x8 is like smoothingPairs16x16 but for the seams of the four
// 4×4 quadrants of an 8×8.
var smoothingPairs8x8 = [12][2]uint8{
	{0o03, 0o04},
	{0o13, 0o14},
	{0o23, 0o24},

	{0o30, 0o40},
	{0o31, 0o41},
	{0o32, 0o42},

	{0o35, 0o45},
	{0o36, 0o46},
	{0o37, 0o47},

	{0o53, 0o54},
	{0o63, 0o64},
	{0o73, 0o74},
}

// scaleAndBiasChromaUp is equivalent to this [256]uint8 look-up table.
//
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x06,
// 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x14, 0x16,   0x18, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x24, 0x26,
// 0x28, 0x2A, 0x2C, 0x2E, 0x30, 0x32, 0x34, 0x36,   0x38, 0x3A, 0x3C, 0x3E, 0x40, 0x42, 0x44, 0x46,
// 0x48, 0x4A, 0x4C, 0x4E, 0x50, 0x52, 0x54, 0x56,   0x58, 0x5A, 0x5C, 0x5E, 0x60, 0x62, 0x64, 0x66,
// 0x68, 0x6A, 0x6C, 0x6E, 0x70, 0x72, 0x74, 0x76,   0x78, 0x7A, 0x7C, 0x7E, 0x80, 0x82, 0x84, 0x86,
//
// 0x88, 0x8A, 0x8C, 0x8E, 0x90, 0x92, 0x94, 0x96,   0x98, 0x9A, 0x9C, 0x9E, 0xA0, 0xA2, 0xA4, 0xA6,
// 0xA8, 0xAA, 0xAC, 0xAE, 0xB0, 0xB2, 0xB4, 0xB6,   0xB8, 0xBA, 0xBC, 0xBE, 0xC0, 0xC2, 0xC4, 0xC6,
// 0xC8, 0xCA, 0xCC, 0xCE, 0xD0, 0xD2, 0xD4, 0xD6,   0xD8, 0xDA, 0xDC, 0xDE, 0xE0, 0xE2, 0xE4, 0xE6,
// 0xE8, 0xEA, 0xEC, 0xEE, 0xF0, 0xF2, 0xF4, 0xF6,   0xF8, 0xFA, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
// 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
// 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
// 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
// 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
func scaleAndBiasChromaUp(b *lowleveljpeg.BlockU8) {
	for i, v := range b {
		b[i] = uint8(max(0x00, min(0xFF, ((int(v)*2)-0x78))))
	}
}

// scaleAndBiasChromaDown is equivalent to this [256]uint8 look-up table.
//
// 0x3C, 0x3C, 0x3D, 0x3D, 0x3E, 0x3E, 0x3F, 0x3F,   0x40, 0x40, 0x41, 0x41, 0x42, 0x42, 0x43, 0x43,
// 0x44, 0x44, 0x45, 0x45, 0x46, 0x46, 0x47, 0x47,   0x48, 0x48, 0x49, 0x49, 0x4A, 0x4A, 0x4B, 0x4B,
// 0x4C, 0x4C, 0x4D, 0x4D, 0x4E, 0x4E, 0x4F, 0x4F,   0x50, 0x50, 0x51, 0x51, 0x52, 0x52, 0x53, 0x53,
// 0x54, 0x54, 0x55, 0x55, 0x56, 0x56, 0x57, 0x57,   0x58, 0x58, 0x59, 0x59, 0x5A, 0x5A, 0x5B, 0x5B,
// 0x5C, 0x5C, 0x5D, 0x5D, 0x5E, 0x5E, 0x5F, 0x5F,   0x60, 0x60, 0x61, 0x61, 0x62, 0x62, 0x63, 0x63,
// 0x64, 0x64, 0x65, 0x65, 0x66, 0x66, 0x67, 0x67,   0x68, 0x68, 0x69, 0x69, 0x6A, 0x6A, 0x6B, 0x6B,
// 0x6C, 0x6C, 0x6D, 0x6D, 0x6E, 0x6E, 0x6F, 0x6F,   0x70, 0x70, 0x71, 0x71, 0x72, 0x72, 0x73, 0x73,
// 0x74, 0x74, 0x75, 0x75, 0x76, 0x76, 0x77, 0x77,   0x78, 0x78, 0x79, 0x79, 0x7A, 0x7A, 0x7B, 0x7B,
//
// 0x7C, 0x7C, 0x7D, 0x7D, 0x7E, 0x7E, 0x7F, 0x7F,   0x80, 0x80, 0x81, 0x81, 0x82, 0x82, 0x83, 0x83,
// 0x84, 0x84, 0x85, 0x85, 0x86, 0x86, 0x87, 0x87,   0x88, 0x88, 0x89, 0x89, 0x8A, 0x8A, 0x8B, 0x8B,
// 0x8C, 0x8C, 0x8D, 0x8D, 0x8E, 0x8E, 0x8F, 0x8F,   0x90, 0x90, 0x91, 0x91, 0x92, 0x92, 0x93, 0x93,
// 0x94, 0x94, 0x95, 0x95, 0x96, 0x96, 0x97, 0x97,   0x98, 0x98, 0x99, 0x99, 0x9A, 0x9A, 0x9B, 0x9B,
// 0x9C, 0x9C, 0x9D, 0x9D, 0x9E, 0x9E, 0x9F, 0x9F,   0xA0, 0xA0, 0xA1, 0xA1, 0xA2, 0xA2, 0xA3, 0xA3,
// 0xA4, 0xA4, 0xA5, 0xA5, 0xA6, 0xA6, 0xA7, 0xA7,   0xA8, 0xA8, 0xA9, 0xA9, 0xAA, 0xAA, 0xAB, 0xAB,
// 0xAC, 0xAC, 0xAD, 0xAD, 0xAE, 0xAE, 0xAF, 0xAF,   0xB0, 0xB0, 0xB1, 0xB1, 0xB2, 0xB2, 0xB3, 0xB3,
// 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB6, 0xB7, 0xB7,   0xB8, 0xB8, 0xB9, 0xB9, 0xBA, 0xBA, 0xBB, 0xBB,
func scaleAndBiasChromaDown(b *lowleveljpeg.BlockU8) {
	for i, v := range b {
		b[i] = (v >> 1) + 0x3C
	}
}

// cubeRootLUT[x] is the cube root of x, when mapping the uint8 range [0x00,
// 0xFF] to float64 range [-128/128, +127/128].
var cubeRootLUT = [256]uint8{
	0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05,
	0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B,
	0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12,
	0x13, 0x13, 0x13, 0x14, 0x14, 0x15, 0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x18, 0x19, 0x19, 0x1A,
	0x1A, 0x1B, 0x1B, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23,
	0x24, 0x24, 0x25, 0x26, 0x26, 0x27, 0x28, 0x28, 0x29, 0x2A, 0x2B, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3F,
	0x40, 0x41, 0x43, 0x44, 0x46, 0x48, 0x49, 0x4B, 0x4D, 0x4F, 0x52, 0x55, 0x58, 0x5B, 0x60, 0x67,
	0x80, 0x99, 0xA0, 0xA5, 0xA8, 0xAB, 0xAE, 0xB1, 0xB3, 0xB5, 0xB7, 0xB8, 0xBA, 0xBC, 0xBD, 0xBF,
	0xC0, 0xC1, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
	0xD1, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD5, 0xD6, 0xD7, 0xD8, 0xD8, 0xD9, 0xDA, 0xDA, 0xDB, 0xDC,
	0xDC, 0xDD, 0xDE, 0xDE, 0xDF, 0xDF, 0xE0, 0xE1, 0xE1, 0xE2, 0xE2, 0xE3, 0xE3, 0xE4, 0xE5, 0xE5,
	0xE6, 0xE6, 0xE7, 0xE7, 0xE8, 0xE8, 0xE9, 0xE9, 0xEA, 0xEA, 0xEB, 0xEB, 0xEC, 0xEC, 0xED, 0xED,
	0xED, 0xEE, 0xEE, 0xEF, 0xEF, 0xF0, 0xF0, 0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 0xF3, 0xF3, 0xF3, 0xF4,
	0xF4, 0xF5, 0xF5, 0xF5, 0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA,
	0xFA, 0xFB, 0xFB, 0xFC, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
}

// biasAndClamp[x & 1023] is (x + 0x80), clamped to the range [0x00, 0xFF], for
// a signed integer x in the range [-512, +511].
var biasAndClamp = [1024]uint8{
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,

	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
}
