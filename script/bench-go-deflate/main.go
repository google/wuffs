// Copyright 2019 The Wuffs Authors.
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

// This program exercises the Go standard library's Deflate decoder.
//
// Wuffs' C code doesn't depend on Go per se, but this program gives some
// performance data for specific Go Deflate implementations. The equivalent
// Wuffs benchmarks (on the same test files) are run via:
//
// wuffs bench std/deflate

import (
	"bytes"
	"compress/flate"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"runtime"
	"time"
)

const (
	iterscale = 10
	reps      = 5
)

type testCase = struct {
	benchname     string
	src           []byte
	itersUnscaled uint32
}

// The various magic constants below are copied from test/c/std/deflate.c
var testCases = []testCase{{
	benchname:     "go_deflate_decode_1k",
	src:           mustLoad("test/data/romeo.txt.gz")[20:550],
	itersUnscaled: 2000,
}, {
	benchname:     "go_deflate_decode_10k",
	src:           mustLoad("test/data/midsummer.txt.gz")[24:5166],
	itersUnscaled: 300,
}, {
	benchname:     "go_deflate_decode_100k",
	src:           mustLoad("test/data/pi.txt.gz")[17:48335],
	itersUnscaled: 30,
}}

func mustLoad(filename string) []byte {
	src, err := ioutil.ReadFile("../../" + filename)
	if err != nil {
		panic(err.Error())
	}
	return src
}

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
			runtime.GC()

			start := time.Now()

			iters := uint64(tc.itersUnscaled) * iterscale
			numBytes, err := decode(tc.src)
			if err != nil {
				return err
			}
			for j := uint64(1); j < iters; j++ {
				decode(tc.src)
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

func decode(src []byte) (numBytes uint64, retErr error) {
	r := flate.NewReader(bytes.NewReader(src))
	defer r.Close()
	n, err := io.Copy(ioutil.Discard, r)
	return uint64(n), err
}
