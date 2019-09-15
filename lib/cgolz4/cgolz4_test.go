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

package cgolz4

import (
	"bytes"
	"io/ioutil"
	"strings"
	"testing"
)

const (
	// compressedMore is 21 bytes of lz4 frame:
	// \x04\x22\x4d\x18 \x40 \x40 \xc0 \x06\x00\x00\x80 \x4d\x6f\x72\x65\x21\x0a \x00\x00\x00\x00
	// Magic----------- FLG- BD-- HC-  BlockSize------- BlockData--------------- EndMark---------
	//
	// FLG: Version=1.
	// BD: BlockMaxSize=64K.
	// HC: checksum.
	// Block Size: 6 (uncompressed).
	// BlockData: the literal bytes "More!\n".
	compressedMore = "\x04\x22\x4d\x18\x40\x40\xc0\x06\x00\x00\x80\x4d\x6f\x72\x65\x21\x0a\x00\x00\x00\x00"

	uncompressedMore = "More!\n"
)

func TestRoundTrip(tt *testing.T) {
	if !cgoEnabled {
		tt.Skip("cgo is not enabled")
	}

	wr := &WriterRecycler{}
	defer wr.Close()
	w := &Writer{}
	wr.Bind(w)

	rr := &ReaderRecycler{}
	defer rr.Close()
	r := &Reader{}
	rr.Bind(r)

	for i := 0; i < 3; i++ {
		buf := &bytes.Buffer{}

		// Compress.
		{
			w.Reset(buf, nil, 0)
			if _, err := w.Write([]byte(uncompressedMore)); err != nil {
				w.Close()
				tt.Fatalf("i=%d: Write: %v", i, err)
			}
			if err := w.Close(); err != nil {
				tt.Fatalf("i=%d: Close: %v", i, err)
			}
		}

		compressed := buf.String()
		if compressed != compressedMore {
			tt.Fatalf("i=%d: compressed\ngot  % 02x\nwant % 02x", i, compressed, compressedMore)
		}

		// Uncompress.
		{
			r.Reset(strings.NewReader(compressed), nil)
			gotBytes, err := ioutil.ReadAll(r)
			if err != nil {
				r.Close()
				tt.Fatalf("i=%d: ReadAll: %v", i, err)
			}
			if got, want := string(gotBytes), uncompressedMore; got != want {
				r.Close()
				tt.Fatalf("i=%d:\ngot  %q\nwant %q", i, got, want)
			}
			if err := r.Close(); err != nil {
				tt.Fatalf("i=%d: Close: %v", i, err)
			}
		}
	}
}
