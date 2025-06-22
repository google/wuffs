// Copyright 2024 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

package uncompng

import (
	"bytes"
	"image"
	"image/draw"
	"image/png"
	"io"
	"os"
	"testing"
)

func encodeImage(w io.Writer, src image.Image) error {
	e := Encoder{}
	b := src.Bounds()

	switch src := src.(type) {
	case *image.Gray:
		return e.Encode(w, src.Pix, b.Dx(), b.Dy(), src.Stride, Depth8, ColorTypeGray)

	case *image.Gray16:
		return e.Encode(w, src.Pix, b.Dx(), b.Dy(), src.Stride, Depth16, ColorTypeGray)

	case *image.RGBA:
		if src.Opaque() {
			return e.Encode(w, src.Pix, b.Dx(), b.Dy(), src.Stride, Depth8, ColorTypeRGBX)
		}

	case *image.RGBA64:
		if src.Opaque() {
			return e.Encode(w, src.Pix, b.Dx(), b.Dy(), src.Stride, Depth16, ColorTypeRGBX)
		}

	case *image.NRGBA:
		if src.Opaque() {
			return e.Encode(w, src.Pix, b.Dx(), b.Dy(), src.Stride, Depth8, ColorTypeRGBX)
		} else {
			return e.Encode(w, src.Pix, b.Dx(), b.Dy(), src.Stride, Depth8, ColorTypeNRGBA)
		}

	case *image.NRGBA64:
		if src.Opaque() {
			return e.Encode(w, src.Pix, b.Dx(), b.Dy(), src.Stride, Depth16, ColorTypeRGBX)
		} else {
			return e.Encode(w, src.Pix, b.Dx(), b.Dy(), src.Stride, Depth16, ColorTypeNRGBA)
		}
	}

	tmp := image.NewNRGBA(b)
	draw.Draw(tmp, b, src, b.Min, draw.Src)
	return e.Encode(w, tmp.Pix, b.Dx(), b.Dy(), tmp.Stride, Depth8, ColorTypeNRGBA)
}

func getPix(m image.Image) []byte {
	switch m := m.(type) {
	case *image.Gray:
		return m.Pix
	case *image.Gray16:
		return m.Pix
	case *image.RGBA:
		return m.Pix
	case *image.RGBA64:
		return m.Pix
	case *image.NRGBA:
		return m.Pix
	case *image.NRGBA64:
		return m.Pix
	}
	return nil
}

func TestRoundTrip(tt *testing.T) {
	testCases := []string{
		"36.png",
		"49.png",
		"bricks-color.png",
		"bricks-gray.png",
		"harvesters.png",
		"hibiscus.primitive.png",
		"hibiscus.regular.png",
		"hippopotamus.masked-with-muybridge.png",
		"hippopotamus.regular.png",
	}

	for _, tc := range testCases {
		testRoundTrip(tt, tc)
	}
}

func testRoundTrip(tt *testing.T, basename string) {
	src, err := os.Open("../../test/data/" + basename)
	if err != nil {
		tt.Errorf("%q: os.Open: %v", basename, err)
		return
	}
	defer src.Close()

	img0, err := png.Decode(src)
	if err != nil {
		tt.Errorf("%q: png.Decode #0: %v", basename, err)
		return
	}

	buf := &bytes.Buffer{}
	err = encodeImage(buf, img0)
	if err != nil {
		tt.Errorf("%q: encodeImage: %v", basename, err)
		return
	}

	img1, err := png.Decode(buf)
	if err != nil {
		tt.Errorf("%q: png.Decode #1: %v", basename, err)
		return
	}

	rect1, rect0 := img1.Bounds(), img0.Bounds()
	if rect1 != rect0 {
		tt.Errorf("%q: rect1: got %v, want %v", basename, rect1, rect0)
		return
	}

	pix1, pix0 := getPix(img1), getPix(img0)
	if pix1 == nil {
		tt.Errorf("%q: pix1 was nil", basename)
		return
	} else if !bytes.Equal(pix1, pix0) {
		tt.Errorf("%q: pix1 differed from pix0", basename)
		return
	}
}

func TestNoAllocation(tt *testing.T) {
	enc := Encoder{}
	pix := make([]byte, 4*3*5)
	got := testing.AllocsPerRun(100, func() {
		enc.Encode(io.Discard, pix, 3, 5, 4*3, Depth8, ColorTypeNRGBA)
	})
	if got != 0 {
		tt.Errorf("AllocsPerRun: got %v, want 0", got)
		return
	}
}

type lookingForSeparateIENDChunkBuffer struct {
	buf  bytes.Buffer
	seen bool
}

func (z *lookingForSeparateIENDChunkBuffer) Write(b []byte) (int, error) {
	const iendChunk = "\x00\x00\x00\x00IEND\xAE\x42\x60\x82"
	if (len(b) == len(iendChunk)) && (string(b) == iendChunk) {
		z.seen = true
	}
	return z.buf.Write(b)
}

func TestWriteSeparateIENDChunk(tt *testing.T) {
	lfsicb := lookingForSeparateIENDChunkBuffer{}
	rect0 := image.Rect(0, 0, 65470, 1)
	img0 := image.NewGray(rect0)
	if err := encodeImage(&lfsicb, img0); err != nil {
		tt.Fatalf("encodeImage: %v", err)
	}
	img1, err := png.Decode(&lfsicb.buf)
	if err != nil {
		tt.Fatalf("png.Decode: %v", err)
	}
	rect1 := img1.Bounds()
	if rect1 != rect0 {
		tt.Fatalf("rect1: got %v, want %v", rect1, rect0)
	}
	if !lfsicb.seen {
		tt.Fatalf("have not seen a separate IEND chunk")
	}
}
