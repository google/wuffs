// Copyright 2022 The Wuffs Authors.
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

package litonlylzma

import (
	"bytes"
	"os"
	"testing"
)

const (
	oneThousand0x00s = "oneThousand0x00s"
	oneThousand0xFFs = "oneThousand0xFFs"
)

func testRoundTrip(tt *testing.T, filename string) {
	original := []byte(nil)
	switch filename {
	case oneThousand0x00s:
		original = make([]byte, 1000)
	case oneThousand0xFFs:
		original = make([]byte, 1000)
		for i := range original {
			original[i] = 0xFF
		}
	default:
		o, err := os.ReadFile("../../test/data/" + filename)
		if err != nil {
			tt.Fatalf("ReadFile: %v", err)
		}
		original = o
	}

	fileFormats := [...]FileFormat{
		FileFormatLZMA,
		FileFormatXz,
	}

	for _, f := range fileFormats {
		compressed, err := f.Encode(nil, original)
		if err != nil {
			tt.Fatalf("%v.Encode: %v", f, err)
		}
		recovered, _, err := f.Decode(nil, compressed)
		if err != nil {
			tt.Fatalf("%v.Decode: %v", f, err)
		}
		if !bytes.Equal(original, recovered) {
			tt.Fatalf("%v: round trip produced different bytes", f)
		}
	}
}

func TestRoundTrip0Bytes(tt *testing.T)           { testRoundTrip(tt, "0.bytes") }
func TestRoundTripArchiveTar(tt *testing.T)       { testRoundTrip(tt, "archive.tar") }
func TestRoundTripOneThousand0x00s(tt *testing.T) { testRoundTrip(tt, oneThousand0x00s) }
func TestRoundTripOneThousand0xFFs(tt *testing.T) { testRoundTrip(tt, oneThousand0xFFs) }
func TestRoundTripPiTxt(tt *testing.T)            { testRoundTrip(tt, "pi.txt") }
func TestRoundTripRomeoTxt(tt *testing.T)         { testRoundTrip(tt, "romeo.txt") }
