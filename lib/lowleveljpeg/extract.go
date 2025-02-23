// Copyright 2025 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

package lowleveljpeg

import (
	"image"
	"image/color"
)

// ExtractFrom sets dst to a single channel (Gray) 8×8 MCU (Minimum Coded
// Unit), with the given top-left corner, from the image m.
func (dst *Array1BlockU8) ExtractFrom(m image.Image, topLeftX int, topLeftY int) {
	if dst == nil {
		return
	}
	dst.SetToNeutral()
	if m == nil {
		return
	}
	bounds := m.Bounds()
	mGray, _ := m.(*image.Gray)
	mRGBA64Image, _ := m.(image.RGBA64Image)

	for dy := 0; dy < 8; dy++ {
		for dx := 0; dx < 8; dx++ {
			p := image.Point{topLeftX + dx, topLeftY + dy}
			if !p.In(bounds) {
				continue
			}
			i := (8 * dy) + dx

			if mGray != nil {
				dst[0][i] = mGray.GrayAt(p.X, p.Y).Y

			} else if mRGBA64Image != nil {
				pix := mRGBA64Image.RGBA64At(p.X, p.Y)
				r, g, b := uint32(pix.R), uint32(pix.G), uint32(pix.B)
				y := ((19595 * r) + (38470 * g) + (7471 * b) + (1 << 15)) >> 24
				dst[0][i] = uint8(y)

			} else {
				r, g, b, _ := m.At(p.X, p.Y).RGBA()
				y := ((19595 * r) + (38470 * g) + (7471 * b) + (1 << 15)) >> 24
				dst[0][i] = uint8(y)
			}
		}
	}

	fillRightAndDown(&dst[0], topLeftX, topLeftY, bounds.Max.X, bounds.Max.Y)
}

// ExtractFrom sets dst to a three channel (Luma, Chroma-blue, Chroma-red) 8×8
// 4:4:4 MCU (Minimum Coded Unit), with the given top-left corner, from the
// image m.
func (dst *Array3BlockU8) ExtractFrom(m image.Image, topLeftX int, topLeftY int) {
	if dst == nil {
		return
	}
	dst.SetToNeutral()
	if m == nil {
		return
	}
	bounds := m.Bounds()
	mYCbCr, _ := m.(*image.YCbCr)
	mRGBA64Image, _ := m.(image.RGBA64Image)

	for dy := 0; dy < 8; dy++ {
		for dx := 0; dx < 8; dx++ {
			p := image.Point{topLeftX + dx, topLeftY + dy}
			if !p.In(bounds) {
				continue
			}
			i := (8 * dy) + dx

			if mYCbCr != nil {
				pix := mYCbCr.YCbCrAt(p.X, p.Y)
				dst[0][i] = pix.Y
				dst[1][i] = pix.Cb
				dst[2][i] = pix.Cr

			} else if mRGBA64Image != nil {
				pix := mRGBA64Image.RGBA64At(p.X, p.Y)
				y, cb, cr := color.RGBToYCbCr(
					uint8(pix.R>>8),
					uint8(pix.G>>8),
					uint8(pix.B>>8),
				)
				dst[0][i] = y
				dst[1][i] = cb
				dst[2][i] = cr

			} else {
				r, g, b, _ := m.At(p.X, p.Y).RGBA()
				y, cb, cr := color.RGBToYCbCr(
					uint8(r>>8),
					uint8(g>>8),
					uint8(b>>8),
				)
				dst[0][i] = y
				dst[1][i] = cb
				dst[2][i] = cr
			}
		}
	}

	fillRightAndDown(&dst[0], topLeftX, topLeftY, bounds.Max.X, bounds.Max.Y)
	fillRightAndDown(&dst[1], topLeftX, topLeftY, bounds.Max.X, bounds.Max.Y)
	fillRightAndDown(&dst[2], topLeftX, topLeftY, bounds.Max.X, bounds.Max.Y)
}

// ExtractFrom sets dst to a three channel (Luma, Chroma-blue, Chroma-red)
// 16×16 4:2:0 MCU (Minimum Coded Unit, with the given top-left corner, from
// the image m.
//
// The 6 elements are:
//   - Luma top left
//   - Luma top right
//   - Luma bottom left
//   - Luma bottom right
//   - Chroma-blue
//   - Chroma-red
func (dst *Array6BlockU8) ExtractFrom(m image.Image, topLeftX int, topLeftY int) {
	if dst == nil {
		return
	}
	if m == nil {
		dst.SetToNeutral()
		return
	}

	tmp := [4]Array3BlockU8{}
	tmp[0].ExtractFrom(m, topLeftX+0, topLeftY+0)
	tmp[1].ExtractFrom(m, topLeftX+8, topLeftY+0)
	tmp[2].ExtractFrom(m, topLeftX+0, topLeftY+8)
	tmp[3].ExtractFrom(m, topLeftX+8, topLeftY+8)

	dst[0] = tmp[0][0]
	dst[1] = tmp[1][0]
	dst[2] = tmp[2][0]
	dst[3] = tmp[3][0]

	downsample4x4(dst[4][0x00:], &tmp[0][1])
	downsample4x4(dst[4][0x04:], &tmp[1][1])
	downsample4x4(dst[4][0x20:], &tmp[2][1])
	downsample4x4(dst[4][0x24:], &tmp[3][1])

	downsample4x4(dst[5][0x00:], &tmp[0][2])
	downsample4x4(dst[5][0x04:], &tmp[1][2])
	downsample4x4(dst[5][0x20:], &tmp[2][2])
	downsample4x4(dst[5][0x24:], &tmp[3][2])
}

// fillRightAndDown copies the right and bottom edge values to the full 8×8 block.
//
// For 4:2:0, we call this before chroma downsampling (from 8×8 to 4×4) to
// avoid artifacts from averaging with the neutral 0x80.
//
// For example, if mx = 3 and my = 6 then it updates b from this:
//
//	AC A7 A3 80 80 80 80 80
//	B6 B1 AD 80 80 80 80 80
//	C0 BB B7 80 80 80 80 80
//	C9 C5 C0 80 80 80 80 80
//	D3 CF CA 80 80 80 80 80
//	DD D9 D4 80 80 80 80 80
//	80 80 80 80 80 80 80 80
//	80 80 80 80 80 80 80 80
//
// to this:
//
//	AC A7 A3 A3 A3 A3 A3 A3
//	B6 B1 AD AD AD AD AD AD
//	C0 BB B7 B7 B7 B7 B7 B7
//	C9 C5 C0 C0 C0 C0 C0 C0
//	D3 CF CA CA CA CA CA CA
//	DD D9 D4 D4 D4 D4 D4 D4
//	DD D9 D4 D4 D4 D4 D4 D4
//	DD D9 D4 D4 D4 D4 D4 D4
func fillRightAndDown(b *BlockU8, topLeftX int, topLeftY int, bottomRightX int, bottomRightY int) {
	if bottomRightX <= topLeftX {
		return
	}
	mx := bottomRightX - topLeftX
	if mx >= 8 {
		mx = 8
	}

	if bottomRightY <= topLeftY {
		return
	}
	my := bottomRightY - topLeftY
	if my >= 8 {
		my = 8
	}

	if mx < 8 {
		for y := 0; y < my; y++ {
			value := b[(8*y)|(mx-1)]
			for x := mx; x < 8; x++ {
				b[(8*y)|x] = value
			}
		}
	}

	if my < 8 {
		src := b[(8 * (my - 1)):(8 * (my - 0))]
		for y := my; y < 8; y++ {
			dst := b[(8 * (y + 0)):(8 * (y + 1))]
			copy(dst, src)
		}
	}
}

// downsample4x4 reduces one 8×8 block to one 4×4 sub-block.
func downsample4x4(dst []uint8, src *BlockU8) {
	for y := 0; y < 4; y++ {
		for x := 0; x < 4; x++ {
			d := (y << 3) | (x << 0)
			s := (y << 4) | (x << 1)
			dst[d] = uint8((2 +
				uint32(src[s+0x00]) +
				uint32(src[s+0x01]) +
				uint32(src[s+0x08]) +
				uint32(src[s+0x09])) / 4)
		}
	}
}

// DownsampleFrom reduces one 16×16 quad-block to one 8×8 block.
func (dst *BlockU8) DownsampleFrom(src *QuadBlockU8) {
	if dst == nil {
		return
	} else if src == nil {
		dst.SetToNeutral()
		return
	}
	for y := 0; y < 8; y++ {
		for x := 0; x < 8; x++ {
			d := (y << 3) | (x << 0)
			s := (y << 5) | (x << 1)
			dst[d] = uint8((2 +
				uint32(src[s+0x00]) +
				uint32(src[s+0x01]) +
				uint32(src[s+0x10]) +
				uint32(src[s+0x11])) / 4)
		}
	}
}
