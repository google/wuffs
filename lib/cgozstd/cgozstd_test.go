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

package cgozstd

import (
	"bytes"
	"io/ioutil"
	"strings"
	"testing"
)

const (
	// compressedMore is 15 bytes of zstd frame:
	// \x28\xb5\x2f\xfd \x00 \x58 \x31\x00\x00 \x4d\x6f\x72\x65\x21\x0a"
	// Magic----------- FHD- WD-- BH---------- BlockData----------------
	//
	// Frame Header Descriptor 0x00 means no flags set.
	// Window Descriptor: Exponent=11, Mantissa=0, Window_Size=2MiB.
	// Block Header: Last_Block=1, Block_Type=0 (Raw_Block), Block_Size=6.
	// BlockData is the literal bytes "More!\n".
	compressedMore = "\x28\xb5\x2f\xfd\x00\x58\x31\x00\x00\x4d\x6f\x72\x65\x21\x0a"

	uncompressedMore = "More!\n"
)

func TestRoundTrip(t *testing.T) {
	if !cgoEnabled {
		t.Skip("cgo is not enabled")
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
				t.Fatalf("i=%d: Write: %v", i, err)
			}
			if err := w.Close(); err != nil {
				t.Fatalf("i=%d: Close: %v", i, err)
			}
		}

		compressed := buf.String()
		if compressed != compressedMore {
			t.Fatalf("i=%d: compressed\ngot  % 02x\nwant % 02x", i, compressed, compressedMore)
		}

		// Uncompress.
		{
			r.Reset(strings.NewReader(compressed), nil)
			gotBytes, err := ioutil.ReadAll(r)
			if err != nil {
				r.Close()
				t.Fatalf("i=%d: ReadAll: %v", i, err)
			}
			if got, want := string(gotBytes), uncompressedMore; got != want {
				r.Close()
				t.Fatalf("i=%d:\ngot  %q\nwant %q", i, got, want)
			}
			if err := r.Close(); err != nil {
				t.Fatalf("i=%d: Close: %v", i, err)
			}
		}
	}
}
