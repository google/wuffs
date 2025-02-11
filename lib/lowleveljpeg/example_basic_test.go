// Copyright 2025 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

package lowleveljpeg_test

import (
	"bytes"
	"fmt"
	"image"
	"image/color"
	"log"
	"os"

	"github.com/google/wuffs/lib/lowleveljpeg"
)

// Example_Basic demonstrates creating an opaque RGBA image and saving it as a
// JPEG. It uses 4:2:0 chroma subsampling, the most popular choice for colorful
// (not gray) JPEG images.
func Example_Basic() {
	const width, height = 40, 30
	src := image.NewRGBA(image.Rect(0, 0, width, height))

	// Fill src with a red/blue gradient.
	for y := 0; y < height; y++ {
		blue := uint8((float64(y) * 0xFF) / (height - 1))
		for x := 0; x < width; x++ {
			red := uint8((float64(x) * 0xFF) / (width - 1))
			src.SetRGBA(x, y, color.RGBA{red, 0x00, blue, 0xFF})
		}
	}

	// Initialize the encoder.
	bounds := src.Bounds()
	colorType := lowleveljpeg.ColorTypeYCbCr420
	buf := &bytes.Buffer{}
	enc := lowleveljpeg.Encoder{}
	if err := enc.Reset(buf, colorType, bounds.Dx(), bounds.Dy(), nil); err != nil {
		log.Fatalf("enc.Reset: %v", err)
	}

	// Extract, DCT-transform and add each MCU (Mininum Coded Unit). For 4:2:0
	// Chroma subsampling, each MCU is 16×16 and contains 6 blocks: 4 Y, 1 Cb,
	// 1 Cr.
	dx, dy := colorType.MCUDimensions()
	fmt.Printf("MCU Dimensions: %d, %d.\n", dx, dy)
	srcU8s := lowleveljpeg.Array6BlockU8{}
	srcI16s := lowleveljpeg.Array6BlockI16{}
	for y := bounds.Min.Y; y < bounds.Max.Y; y += dy {
		for x := bounds.Min.X; x < bounds.Max.X; x += dx {
			srcU8s.ExtractFrom(src, x, y)
			srcI16s.ForwardDCTFrom(&srcU8s)
			if err := enc.Add6(buf, &srcI16s); err != nil {
				log.Fatalf("enc.Add6: %v", err)
			}
		}
	}

	// Change writeTmpFiles to true if you want to visually inspect the output.
	const writeTmpFiles = false
	const dstFilename = "/tmp/lowleveljpeg_example_basic.jpeg"
	if writeTmpFiles {
		if err := os.WriteFile(dstFilename, buf.Bytes(), 0644); err != nil {
			log.Fatalf("os.WriteFile: %v", err)
		}
		fmt.Println("Wrote  ", dstFilename)
	} else {
		fmt.Println("Skipped", dstFilename)
	}

	// Output:
	// MCU Dimensions: 16, 16.
	// Skipped /tmp/lowleveljpeg_example_basic.jpeg
}
