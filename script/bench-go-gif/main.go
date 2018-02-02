// Copyright 2018 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// +build ignore

package main

// This program exercises the Go standard library's GIF decoder.
//
// Wuffs' C code doesn't depend on Go per se, but this program gives some
// performance data for specific Go GIF implementations. The equivalent Wuffs
// benchmarks (on the same test image) are run via:
//
// wuffs bench -mimic -focus=wuffs_gif_decode_1000k,mimic_gif_decode_1000k std/gif
//
// The "1000k" is because the test image (harvesters.gif) has approximately 1
// million pixels.
//
// The Wuffs benchmark reports megabytes per second. This program reports
// megapixels per second. The two concepts should be equivalent, since GIF
// images' pixel data are always 1 byte per pixel indices into a color palette.

import (
	"bytes"
	"fmt"
	"image"
	"image/gif"
	"io/ioutil"
	"os"
	"time"
)

// These constants are hard-coded to the harvesters.gif test image.
const WIDTH = 1165
const HEIGHT = 859
const BYTES_PER_PIXEL = 1 // Palette index.
const NUM_BYTES = WIDTH * HEIGHT * BYTES_PER_PIXEL
const FIRST_PIXEL = 0 // Top left pixel's palette index is 0x00.
const LAST_PIXEL = 1  // Bottom right pixel's palette index is 0x01.

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	src, err := ioutil.ReadFile("../../test/testdata/harvesters.gif")
	if err != nil {
		return err
	}

	start := time.Now()

	const REPS = 50
	for i := 0; i < REPS; i++ {
		decode(src)
	}

	elapsedNanos := time.Since(start)

	totalPixels := uint64(WIDTH * HEIGHT * REPS)
	kpPerS := totalPixels * 1000000 / uint64(elapsedNanos)

	fmt.Printf("Go      %3d.%03d megapixels/second\n", kpPerS/1000, kpPerS%1000)
	return nil
}

func decode(src []byte) {
	img, err := gif.Decode(bytes.NewReader(src))
	if err != nil {
		panic(err)
	}
	m := img.(*image.Paletted)
	if len(m.Pix) != NUM_BYTES {
		panic("wrong num_bytes")
	}

	// A hard-coded sanity check that we decoded the pixel data correctly.
	if (m.Pix[0] != FIRST_PIXEL) || (m.Pix[NUM_BYTES-1] != LAST_PIXEL) {
		panic("wrong dst pixels")
	}
}
