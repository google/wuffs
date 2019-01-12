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
// benchmarks (on the same test images) are run via:
//
// wuffs bench std/gif

import (
	"bytes"
	"fmt"
	"image"
	"image/draw"
	"image/gif"
	"io/ioutil"
	"os"
	"runtime"
	"strings"
	"time"
)

const (
	iterscale = 20
	reps      = 5
)

type testCase = struct {
	benchname     string
	filename      string
	itersUnscaled uint32
}

var testCases = []testCase{{
	benchname:     "go_gif_decode_1k_bw",
	filename:      "test/data/pjw-thumbnail.gif",
	itersUnscaled: 2000,
}, {
	benchname:     "go_gif_decode_1k_color",
	filename:      "test/data/hippopotamus.regular.gif",
	itersUnscaled: 1000,
}, {
	benchname:     "go_gif_decode_10k_bgra",
	filename:      "test/data/hat.gif",
	itersUnscaled: 100,
}, {
	benchname:     "go_gif_decode_10k_indexed",
	filename:      "test/data/hat.gif",
	itersUnscaled: 100,
}, {
	benchname:     "go_gif_decode_20k",
	filename:      "test/data/bricks-gray.gif",
	itersUnscaled: 50,
}, {
	benchname:     "go_gif_decode_100k_artificial",
	filename:      "test/data/hibiscus.primitive.gif",
	itersUnscaled: 15,
}, {
	benchname:     "go_gif_decode_100k_realistic",
	filename:      "test/data/hibiscus.regular.gif",
	itersUnscaled: 10,
}, {
	benchname:     "go_gif_decode_1000k",
	filename:      "test/data/harvesters.gif",
	itersUnscaled: 1,
}, {
	benchname:     "go_gif_decode_anim_screencap",
	filename:      "test/data/gifplayer-muybridge.gif",
	itersUnscaled: 1,
}}

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	fmt.Printf("# Go %s\n", runtime.Version())
	fmt.Printf("#\n")
	fmt.Printf("# The output format, including the \"Benchmark\" prefixes, is compatible with the\n")
	fmt.Printf("# https://godoc.org/golang.org/x/perf/cmd/benchstat tool. To install it, first\n")
	fmt.Printf("# install Go, then run \"go get golang.org/x/perf/cmd/benchstat\".\n")

	for i := -1; i < reps; i++ {
		for _, tc := range testCases {
			src, err := ioutil.ReadFile("../../" + tc.filename)
			if err != nil {
				return err
			}

			runtime.GC()

			start := time.Now()

			iters := uint64(tc.itersUnscaled) * iterscale
			bgra := strings.HasSuffix(tc.benchname, "_bgra")
			numBytes, err := decode(src, bgra)
			if err != nil {
				return err
			}
			for i := uint64(1); i < iters; i++ {
				decode(src, bgra)
			}

			elapsedNanos := time.Since(start)

			kbPerS := numBytes * uint64(iters) * 1000000 / uint64(elapsedNanos)

			if i < 0 {
				continue // Warm up rep.
			}

			fmt.Printf("Benchmark%-30s %8d %12d ns/op %8d.%03d MB/s\n",
				tc.benchname, iters, uint64(elapsedNanos)/iters, kbPerS/1000, kbPerS%1000)
		}
	}

	return nil
}

func decode(src []byte, bgra bool) (numBytes uint64, retErr error) {
	g, err := gif.DecodeAll(bytes.NewReader(src))
	if err != nil {
		return 0, err
	}

	for _, m := range g.Image {
		b := m.Bounds()
		n := uint64(b.Dx()) * uint64(b.Dy())

		// Go converts to RGBA (as that's what Go's image/draw standard library is
		// optimized for); Wuffs converts to BGRA. The difference isn't important,
		// as we just want measure how long it takes.
		if bgra {
			dst := image.NewRGBA(b)
			draw.Draw(dst, b, m, b.Min, draw.Src)
			n *= 4
		}

		numBytes += n
	}

	return numBytes, nil
}
