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
	"image/jpeg"
	"image/png"
	"log"
	"os"

	"github.com/google/wuffs/lib/lowleveljpeg"
)

// Example_yCbCr444 demonstrates loading an RGB PNG, converting to YCbCr and to
// DCT space and saving as a 4:4:4 JPEG. Multiple JPEG outputs from the one PNG
// input demonstrate making further transforms during conversion.
func Example_yCbCr444() {
	// levels holds a rough, ad hoc measure of a coefficient's frequency, in
	// DCT space. Low (or high) levels[i] mean low (or high) frequency.
	//
	// levels[0] == 0 is the DC coefficient. The others are AC coefficients.
	levels := [64]uint8{
		0, 1, 2, 3, 4, 5, 6, 7,
		1, 1, 2, 3, 4, 5, 6, 7,
		2, 2, 2, 3, 4, 5, 6, 7,
		3, 3, 3, 3, 4, 5, 6, 7,
		4, 4, 4, 4, 4, 5, 6, 7,
		5, 5, 5, 5, 5, 5, 6, 7,
		6, 6, 6, 6, 6, 6, 6, 7,
		7, 7, 7, 7, 7, 7, 7, 7,
	}

	// Load the source PNG. We will make numerous JPEG encodings of transformed
	// versions of this PNG image.
	f, err := os.Open("../../test/data/hippopotamus.regular.png")
	if err != nil {
		log.Fatalf("os.Open: %v", err)
	}
	defer f.Close()
	src, err := png.Decode(f)
	if err != nil {
		log.Fatalf("png.Decode: %v", err)
	}

	// Each example (each output JPEG) has a name fragment and a transform from
	// source BlockU8 values to destination BlockI16 values.
	examples := []struct {
		nameFragment string
		quality      int
		transform    func(dst *lowleveljpeg.Array3BlockI16, src *lowleveljpeg.Array3BlockU8)
	}{{

		// Apply the ForwardDCT with no further changes. The resultant JPEG is
		// a low quality (but small in file size) version of the source PNG.
		nameFragment: "0_quality25",
		quality:      25,
		transform: func(dst *lowleveljpeg.Array3BlockI16, src *lowleveljpeg.Array3BlockU8) {
			dst[0].ForwardDCTFrom(&src[0])
			dst[1].ForwardDCTFrom(&src[1])
			dst[2].ForwardDCTFrom(&src[2])
		},
	}, {

		// Ditto, with default quality.
		nameFragment: "1_quality75",
		quality:      75,
		transform: func(dst *lowleveljpeg.Array3BlockI16, src *lowleveljpeg.Array3BlockU8) {
			dst[0].ForwardDCTFrom(&src[0])
			dst[1].ForwardDCTFrom(&src[1])
			dst[2].ForwardDCTFrom(&src[2])
		},
	}, {

		// Ditto, with high quality.
		nameFragment: "2_quality100",
		quality:      100,
		transform: func(dst *lowleveljpeg.Array3BlockI16, src *lowleveljpeg.Array3BlockU8) {
			dst[0].ForwardDCTFrom(&src[0])
			dst[1].ForwardDCTFrom(&src[1])
			dst[2].ForwardDCTFrom(&src[2])
		},
	}, {

		// Like 2_quality100 but swap the Chroma-blue and Chroma-red channels.
		nameFragment: "3_swap_cb_cr",
		quality:      100,
		transform: func(dst *lowleveljpeg.Array3BlockI16, src *lowleveljpeg.Array3BlockU8) {
			dst[0].ForwardDCTFrom(&src[0])
			dst[1].ForwardDCTFrom(&src[2]) // Note the "1" and "2".
			dst[2].ForwardDCTFrom(&src[1]) // Note the "2" and "1".
		},
	}, {

		// Set the 8×8 blocks of the Luma channel to neutral (0x80) except for
		// each top-left corner, which is black (0x00). Chromas are unchanged.
		nameFragment: "4_chroma_only",
		quality:      100,
		transform: func(dst *lowleveljpeg.Array3BlockI16, src *lowleveljpeg.Array3BlockU8) {
			s0 := lowleveljpeg.BlockU8{}
			s0[0] = 0x00
			for i := 1; i < 64; i++ {
				s0[i] = lowleveljpeg.BlockU8NeutralValue
			}
			dst[0].ForwardDCTFrom(&s0)
			dst[1].ForwardDCTFrom(&src[1])
			dst[2].ForwardDCTFrom(&src[2])
		},
	}, {

		// Set the Chroma channels to neutral, as well as any Luma coefficients
		// that have any horizontal variation (within an 8×8 block). What
		// remains are the DC and vertical-only Luma coefficients.
		nameFragment: "5_vertical_luma",
		quality:      100,
		transform: func(dst *lowleveljpeg.Array3BlockI16, src *lowleveljpeg.Array3BlockU8) {
			dst[0].ForwardDCTFrom(&src[0])
			for i := 0; i < 64; i++ {
				if (i & 7) != 0 {
					dst[0][i] = lowleveljpeg.BlockI16NeutralValue
				}
			}
			dst[1] = lowleveljpeg.BlockI16{}
			dst[2] = lowleveljpeg.BlockI16{}
		},
	}, {

		// Set the Chroma channels to neutral, as well as any medium- or
		// high-frequency Luma coefficients.
		nameFragment: "6_low_frequency_luma",
		quality:      100,
		transform: func(dst *lowleveljpeg.Array3BlockI16, src *lowleveljpeg.Array3BlockU8) {
			dst[0] = src[0].ForwardDCT()
			for i := 0; i < 64; i++ {
				if levels[i] > 2 {
					dst[0][i] = 0
				}
			}
			dst[1] = lowleveljpeg.BlockI16{}
			dst[2] = lowleveljpeg.BlockI16{}
		},
	}}

	// Process each example.
	for _, example := range examples {
		// Initialize the encoder.
		quants := lowleveljpeg.Array2QuantizationFactors{}
		quants.SetToStandardValues(example.quality)
		opts := &lowleveljpeg.EncoderOptions{
			QuantizationFactors: &quants,
		}
		bounds := src.Bounds()
		buf := &bytes.Buffer{}
		enc := lowleveljpeg.Encoder{}
		if err := enc.Reset(buf, lowleveljpeg.ColorTypeYCbCr444, bounds.Dx(), bounds.Dy(), opts); err != nil {
			log.Fatalf("enc.Reset: %v", err)
		}

		// Extract, transform and add each MCU (Mininum Coded Unit). For 4:4:4
		// Chroma subsampling, each MCU is 8×8 and contains 3 blocks: 1 Y, 1
		// Cb, 1 Cr.
		srcU8s := lowleveljpeg.Array3BlockU8{}
		srcI16s := lowleveljpeg.Array3BlockI16{}
		for y := bounds.Min.Y; y < bounds.Max.Y; y += 8 {
			for x := bounds.Min.X; x < bounds.Max.X; x += 8 {
				srcU8s.ExtractFrom(src, x, y)
				example.transform(&srcI16s, &srcU8s)
				if err := enc.Add3(buf, &srcI16s); err != nil {
					log.Fatalf("enc.Add3: %v", err)
				}
			}
		}

		// As a basic consistency check, confirm that the resultant bytes are a
		// valid JPEG. When copy/pasting this code, you can omit this step.
		dstBytes := buf.Bytes()
		if _, err := jpeg.Decode(bytes.NewReader(dstBytes)); err != nil {
			log.Fatalf("jpeg.Decode: %v", err)
		}

		// Change writeTmpFiles to true if you want to visually inspect the output.
		const writeTmpFiles = false
		dstFilename := fmt.Sprintf("/tmp/lowleveljpeg_example_%s.jpeg", example.nameFragment)
		if writeTmpFiles {
			if err := os.WriteFile(dstFilename, dstBytes, 0644); err != nil {
				log.Fatalf("os.WriteFile: %v", err)
			}
			fmt.Println("Wrote  ", dstFilename)
		} else {
			fmt.Println("Skipped", dstFilename)
		}
	}

	// Output:
	// Skipped /tmp/lowleveljpeg_example_0_quality25.jpeg
	// Skipped /tmp/lowleveljpeg_example_1_quality75.jpeg
	// Skipped /tmp/lowleveljpeg_example_2_quality100.jpeg
	// Skipped /tmp/lowleveljpeg_example_3_swap_cb_cr.jpeg
	// Skipped /tmp/lowleveljpeg_example_4_chroma_only.jpeg
	// Skipped /tmp/lowleveljpeg_example_5_vertical_luma.jpeg
	// Skipped /tmp/lowleveljpeg_example_6_low_frequency_luma.jpeg
}
