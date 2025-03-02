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
// of image dimensions, up to 32×32 pixels, but also in terms of file size.
// Every Handsum image file is exactly 48 bytes (384 bits) long. This can imply
// using as little as 0.046875 bytes (0.375 bits) per pixel.
//
// Each Handsum image is essentially a scaled 16×16 pixel YCbCr 4:2:0 JPEG MCU
// (Minimum Coded Unit; 4 Luma and 2 Chroma blocks), keeping only the 15 lowest
// frequency DCT (Discrete Cosine Transform) coefficients. Each of the (6 × 15)
// = 90 coefficients are encoded as one nibble (4 bits) with fixed quantization
// factors, totalling 45 bytes. The initial 3 bytes holds a 16-bit magic
// signature, 2-bit version number and 6-bit aspect ratio.
//
// As of February 2025, the latest version is Version 0. All Version 0 files
// use the sRGB color profile.
package handsum

import (
	"errors"
	"image"
	"image/color"
	"io"

	"github.com/google/wuffs/lib/lowleveljpeg"

	"golang.org/x/image/draw"
)

// FileSize is the size (in bytes) of every Handsum image file.
const FileSize = 48

// MaxDimension is the maximum (inclusive) width or height of every Handsum
// image file.
//
// Every image is either (W × 32) or (32 × H) or both, for some positive W or H
// that is no greater than 32.
const MaxDimension = 32

// Magic is the byte string prefix of every Handsum image file.
//
// It's like how every JPEG image file starts with "\xFF\xD8".
const Magic = "\xFE\xD7"

func init() {
	image.RegisterFormat("handsum", Magic, Decode, DecodeConfig)
}

var (
	ErrBadArgument            = errors.New("handsum: bad argument")
	ErrNotAHandsumFile        = errors.New("handsum: not a handsum file")
	ErrUnsupportedFileVersion = errors.New("handsum: unsupported file version")
)

// EncodeOptions are optional arguments to Encode. The zero value is valid and
// means to use the default configuration.
//
// There are no fields for now, but there may be some in the future.
type EncodeOptions struct {
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

	aspectRatio := byte(0x1F)
	if srcW > srcH { // Landscape.
		a := ((int64(srcH) * 64) + int64(srcW)) / (2 * int64(srcW))
		if a <= 0 {
			a = 1
		}
		aspectRatio = byte(a-1) | 0x00
	} else if srcW < srcH { // Portrait.
		a := ((int64(srcW) * 64) + int64(srcH)) / (2 * int64(srcH))
		if a <= 0 {
			a = 1
		}
		aspectRatio = byte(a-1) | 0x20
	}

	dst := image.NewRGBA(image.Rectangle{Max: image.Point{X: 16, Y: 16}})
	draw.BiLinear.Scale(dst, dst.Bounds(), src, src.Bounds(), draw.Src, nil)

	dstU8s := lowleveljpeg.Array6BlockU8{}
	dstU8s.ExtractFrom(dst, 0, 0)

	dstI16s := lowleveljpeg.Array6BlockI16{}
	dstI16s.ForwardDCTFrom(&dstU8s)

	buf := [FileSize]byte{}
	buf[0] = Magic[0]
	buf[1] = Magic[1]
	buf[2] = aspectRatio

	bitOffset := 3 * 8
	for i := range dstI16s {
		bitOffset = encodeBlock(&buf, bitOffset, &dstI16s[i])
	}

	_, err := w.Write(buf[:])
	return err
}

func encodeBlock(buf *[FileSize]byte, bitOffset int, b *lowleveljpeg.BlockI16) int {
	for i := 0; i < nCoeffs; i++ {
		e := uint8(0)
		if i == 0 {
			e = encodeDC(b[0])
		} else {
			e = encodeAC(b[zigzag[i]])
		}
		buf[bitOffset>>3] |= e << (bitOffset & 4)
		bitOffset += 4
	}

	return bitOffset
}

func encodeDC(value int16) uint8 {
	const w = dcBucketWidth

	v := int32(value) + ((w * 8) + (w / 2))
	v /= w
	if v < 0x0 {
		return 0x0
	} else if v > 0xF {
		return 0xF
	}
	return uint8(v)
}

func encodeAC(value int16) uint8 {
	// Dividing by 2 is an arbitrary adjustment that's not mirrored on the
	// decode side, but the results seem a little more vibrant.
	const w = acBucketWidth / 2

	v := int32(value) + ((w * 8) + (w / 2))
	v /= w
	if v < 0x0 {
		return 0x0
	} else if v > 0xF {
		return 0xF
	}
	return uint8(v)
}

// Both DC and AC coefficients are quantized into 16 buckets (4 bits), but they
// use different bucket widths:
//
//	Bucket    DC      AC
//	0x0    -1024    -128
//	0x1     -896    -112
//	0x2     -768     -96
//	0x3     -640     -80
//	...       ...     ...
//	0x7     -128     -16
//	0x8        0       0
//	0x9     +128     +16
//	...       ...     ...
//	0xE     +768     +96
//	0xF     +896    +112
const (
	dcBucketWidth = 128
	acBucketWidth = 16
)

// DecodeConfig reads a Handsum image configuration from r.
func DecodeConfig(r io.Reader) (image.Config, error) {
	buf := [3]byte{}
	if _, err := io.ReadFull(r, buf[:]); err != nil {
		return image.Config{}, err
	} else if (buf[0] != Magic[0]) || (buf[1] != Magic[1]) {
		return image.Config{}, ErrNotAHandsumFile
	} else if (buf[2] & 0xC0) != 0 {
		return image.Config{}, ErrUnsupportedFileVersion
	}

	w, h := decodeWidthAndHeight(buf[2])
	return image.Config{
		ColorModel: color.RGBAModel,
		Width:      w,
		Height:     h,
	}, nil
}

// Decode reads a Handsum image from r.
func Decode(r io.Reader) (image.Image, error) {
	buf := [FileSize]byte{}
	if _, err := io.ReadFull(r, buf[:]); err != nil {
		return nil, err
	} else if (buf[0] != Magic[0]) || (buf[1] != Magic[1]) {
		return nil, ErrNotAHandsumFile
	} else if (buf[2] & 0xC0) != 0 {
		return nil, ErrUnsupportedFileVersion
	}

	bitOffset := 3 * 8
	lumaQuadBlockU8 := lowleveljpeg.QuadBlockU8{}
	bitOffset = decodeBlock(lumaQuadBlockU8[0x00:], 16, &buf, bitOffset)
	bitOffset = decodeBlock(lumaQuadBlockU8[0x08:], 16, &buf, bitOffset)
	bitOffset = decodeBlock(lumaQuadBlockU8[0x80:], 16, &buf, bitOffset)
	bitOffset = decodeBlock(lumaQuadBlockU8[0x88:], 16, &buf, bitOffset)
	smoothLumaBlockSeams(&lumaQuadBlockU8)

	cbBlockU8 := lowleveljpeg.BlockU8{}
	bitOffset = decodeBlock(cbBlockU8[:], 8, &buf, bitOffset)
	cbQuadBlockU8 := lowleveljpeg.QuadBlockU8{}
	cbQuadBlockU8.UpsampleFrom(&cbBlockU8)

	crBlockU8 := lowleveljpeg.BlockU8{}
	bitOffset = decodeBlock(crBlockU8[:], 8, &buf, bitOffset)
	crQuadBlockU8 := lowleveljpeg.QuadBlockU8{}
	crQuadBlockU8.UpsampleFrom(&crBlockU8)

	src := &image.YCbCr{
		Y:              lumaQuadBlockU8[:],
		Cb:             cbQuadBlockU8[:],
		Cr:             crQuadBlockU8[:],
		YStride:        16,
		CStride:        16,
		SubsampleRatio: image.YCbCrSubsampleRatio444,
		Rect:           image.Rectangle{Max: image.Point{X: 16, Y: 16}},
	}

	dstW, dstH := decodeWidthAndHeight(buf[2])
	dst := image.NewRGBA(image.Rectangle{Max: image.Point{X: dstW, Y: dstH}})
	draw.BiLinear.Scale(dst, dst.Bounds(), src, src.Bounds(), draw.Src, nil)
	return dst, nil
}

func decodeWidthAndHeight(buf2 byte) (w int, h int) {
	if (buf2 & 0x20) == 0 { // Landscape.
		w = 32
		h = 1 + int(buf2&0x1F)
	} else { // Portrait.
		w = 1 + int(buf2&0x1F)
		h = 32
	}
	return w, h
}

func decodeBlock(dst []byte, stride int, src *[FileSize]byte, bitOffset int) int {
	a := lowleveljpeg.BlockI16{}

	{
		nibble := (src[bitOffset>>3] >> (bitOffset & 4)) & 15
		a[0] = (int16(nibble) - 8) * dcBucketWidth
		bitOffset += 4
	}

	for i := 1; i < nCoeffs; i++ {
		nibble := (src[bitOffset>>3] >> (bitOffset & 4)) & 15
		a[zigzag[i]] = (int16(nibble) - 8) * acBucketWidth
		bitOffset += 4
	}

	b := lowleveljpeg.BlockU8{}
	b.InverseDCTFrom(&a)

	for i := 0; i < 8; i++ {
		di := i * stride
		bi := i * 8
		copy(dst[di:di+8], b[bi:bi+8])
	}

	return bitOffset
}

const nCoeffs = 15

// zigzag represents JPEG's zig-zag order for visiting coefficients. Handsum
// only uses the first (1 + 2 + 3 + 4 + 5) = 15 of JPEG's 64 coefficients.
//
// https://en.wikipedia.org/wiki/File:JPEG_ZigZag.svg
var zigzag = [nCoeffs]uint8{
	0o00, 0o01, 0o10, 0o20, 0o11, 0o02, 0o03, 0o12, //  0,  1,  8, 16,  9,  2,  3, 10,
	0o21, 0o30, 0o40, 0o31, 0o22, 0o13, 0o04, //       17, 24, 32, 25, 18, 11,  4,
}

func smoothLumaBlockSeams(b *lowleveljpeg.QuadBlockU8) {
	for _, pair := range smoothingPairs {
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

// smoothingPairs are the seams of the four 8×8 Luma blocks in a 16×16 MCU. The
// central 4 pixels are handled separately.
var smoothingPairs = [28][2]uint8{
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
