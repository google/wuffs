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

package zlibcut

import (
	"compress/zlib"
	"testing"

	"github.com/google/wuffs/internal/testcut"
)

func TestCut(tt *testing.T) {
	testcut.Test(tt, SmallestValidMaxEncodedLen, Cut, zlib.NewReader, []string{
		"midsummer.txt.zlib",
		"pi.txt.zlib",
		"romeo.txt.zlib",
	})
}

func BenchmarkCut(b *testing.B) {
	testcut.Benchmark(b, SmallestValidMaxEncodedLen, Cut, zlib.NewReader,
		"pi.txt.zlib", 0, 0, 100003)
}
