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

package raczlib

import (
	"bytes"
	"io"
	"strings"
	"testing"
)

func TestReaderWithDictionary(t *testing.T) {
	// encoded and decoded come from Example_indexLocationAtStart in
	// "../rac/example_test.go".
	const (
		encoded = "" +
			"\x72\xC3\x63\x04\x71\xB5\x00\xFF\x00\x00\x00\x00\x00\x00\x00\xFF" +
			"\x0B\x00\x00\x00\x00\x00\x00\xFF\x16\x00\x00\x00\x00\x00\x00\xFF" +
			"\x23\x00\x00\x00\x00\x00\x00\x01\x50\x00\x00\x00\x00\x00\x01\xFF" +
			"\x5E\x00\x00\x00\x00\x00\x01\x00\x73\x00\x00\x00\x00\x00\x01\x00" +
			"\x88\x00\x00\x00\x00\x00\x01\x00\x9F\x00\x00\x00\x00\x00\x01\x04" +
			"\x08\x00\x20\x73\x68\x65\x65\x70\x2E\x0A\x0B\xE0\x02\x6E\x78\xF9" +
			"\x0B\xE0\x02\x6E\xF2\xCF\x4B\x85\x31\x01\x01\x00\x00\xFF\xFF\x17" +
			"\x21\x03\x90\x78\xF9\x0B\xE0\x02\x6E\x0A\x29\xCF\x87\x31\x01\x01" +
			"\x00\x00\xFF\xFF\x18\x0C\x03\xA8\x78\xF9\x0B\xE0\x02\x6E\x0A\xC9" +
			"\x28\x4A\x4D\x85\x71\x00\x01\x00\x00\xFF\xFF\x21\x6E\x04\x66"

		decoded = "" +
			"One sheep.\n" +
			"Two sheep.\n" +
			"Three sheep.\n"
	)

	buf := &bytes.Buffer{}
	r := &Reader{
		ReadSeeker:     strings.NewReader(encoded),
		CompressedSize: int64(len(encoded)),
	}
	if _, err := io.Copy(buf, r); err != nil {
		t.Fatalf("Copy: %v", err)
	}

	if got, want := buf.String(), decoded; got != want {
		t.Fatalf("got:\n%s\nwant:\n%s", got, want)
	}
}
