// Copyright 2021 The Wuffs Authors.
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

//go:build ignore
// +build ignore

package main

// make-red-blue-gradient.go makes a simple PNG image with optional color
// correction chunks.
//
// Usage: go run make-red-blue-gradient.go > foo.png

import (
	"bytes"
	"flag"
	"hash/crc32"
	"image"
	"image/color"
	"image/png"
	"math"
	"os"
)

var (
	gama = flag.Float64("gama", 2.2, "Gamma correction value (e.g. 2.2)")
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Parse()

	// Make a red-blue gradient.
	m := image.NewRGBA(image.Rect(0, 0, 16, 16))
	for y := 0; y < 16; y++ {
		for x := 0; x < 16; x++ {
			m.SetRGBA(x, y, color.RGBA{
				R: uint8(x * 0x11),
				G: 0x00,
				B: uint8(y * 0x11),
				A: 0xFF,
			})
		}
	}

	// Encode to PNG.
	buf := &bytes.Buffer{}
	png.Encode(buf, m)
	original := buf.Bytes()

	// Split the encoding into a magic+IHDR prefix and an IDAT+IEND suffix. The
	// starting magic number is 8 bytes long. The IHDR chunk is 25 bytes long.
	prefix, suffix := original[:33], original[33:]
	out := []byte(nil)
	out = append(out, prefix...)

	// Insert the optional gAMA chunk between the prefix and suffix.
	if *gama > 0 {
		g := uint32(math.Round(100000 / *gama))
		out = appendPNGChunk(out, "gAMA",
			byte(g>>24),
			byte(g>>16),
			byte(g>>8),
			byte(g>>0),
		)
	}

	// Append the suffix and write to stdout.
	out = append(out, suffix...)
	_, err := os.Stdout.Write(out)
	return err
}

func appendPNGChunk(b []byte, name string, chunk ...byte) []byte {
	n := uint32(len(chunk))
	b = append(b,
		byte(n>>24),
		byte(n>>16),
		byte(n>>8),
		byte(n>>0),
	)
	b = append(b, name...)
	b = append(b, chunk...)
	hasher := crc32.NewIEEE()
	hasher.Write([]byte(name))
	hasher.Write(chunk)
	c := hasher.Sum32()
	return append(b,
		byte(c>>24),
		byte(c>>16),
		byte(c>>8),
		byte(c>>0),
	)
}
