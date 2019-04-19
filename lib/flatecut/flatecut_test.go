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

func TestHuffmanDecode(t *testing.T) {
	// This exercises the "ABCDEFGH" example from RFC 1951 section 3.2.2,
	// discussed in the "type huffman" doc comment.

	const src = "HEADFACE"
	codes := []string{
		'A': " 010",
		'B': " 011",
		'C': " 100",
		'D': " 101",
		'E': " 110",
		'F': "  00",
		'G': "1110",
		'H': "1111",
	}

	encoded := []byte(nil)
	encBits := uint32(0)
	encNBits := uint32(0)
	for _, s := range src {
		for _, c := range codes[s] {
			if c == ' ' {
				continue
			}

			if c == '1' {
				encBits |= 1 << encNBits
			}
			encNBits++

			if encNBits == 8 {
				encoded = append(encoded, uint8(encBits))
				encBits = 0
				encNBits = 0
			}
		}
	}
	if encNBits > 0 {
		encoded = append(encoded, uint8(encBits))
	}

	h := huffman{
		counts: [maxCodeBits + 1]uint32{
			2: 1, 3: 5, 4: 2,
		},
		symbols: [maxNumCodes]int32{
			0: 'F', 1: 'A', 2: 'B', 3: 'C', 4: 'D', 5: 'E', 6: 'G', 7: 'H',
		},
	}
	h.constructLookUpTable()

	b := &bitstream{
		bytes: encoded,
	}

	decoded := []byte(nil)
	for _ = range src {
		c := h.decode(b)
		if c < 0 {
			decoded = append(decoded, '!')
			break
		}
		decoded = append(decoded, uint8(c))
	}

	if got, want := string(decoded), src; got != want {
		t.Fatalf("got %q, want %q", got, want)
	}
}
