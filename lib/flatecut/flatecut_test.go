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

package flatecut

import (
	"compress/flate"
	"io"
	"testing"

	"github.com/google/wuffs/internal/testcut"
)

func TestCut(t *testing.T) {
	testcut.Test(t, SmallestValidMaxEncodedLen, Cut, newReader, []string{
		"artificial/deflate-backref-crosses-blocks.deflate",
		"artificial/deflate-distance-32768.deflate",
		"romeo.txt.deflate",
		"romeo.txt.fixed-huff.deflate",
	})
}

func BenchmarkCut(b *testing.B) {
	testcut.Benchmark(b, SmallestValidMaxEncodedLen, Cut, newReader,
		"pi.txt.zlib", 2, 4, 100003)
}

func newReader(r io.Reader) (io.ReadCloser, error) {
	return flate.NewReader(r), nil
}
