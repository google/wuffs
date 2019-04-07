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

package rac_test

import (
	"bytes"
	"compress/zlib"
	"encoding/hex"
	"fmt"
	"hash/adler32"
	"io"
	"log"

	"github.com/google/wuffs/lib/rac"
)

// ExampleILAEnd demonstrates using the low level "rac" package to encode a
// RAC+Zlib formatted file with IndexLocationAtEnd.
//
// The sibling "raczlib" package (TODO) provides a higher level API that is
// easier to use.
//
// See the RAC specification for an explanation of the file format.
func ExampleILAEnd() {
	// Manually construct a zlib encoding of "More!\n", one that uses a literal
	// block (that's easy to see in a hex dump) instead of a more compressible
	// Huffman block.
	const src = "More!\n"
	hasher := adler32.New()
	hasher.Write([]byte(src))
	enc := []byte{ // See RFCs 1950 and 1951 for details.
		0x78,       // Deflate compression method; 32KiB window size.
		0x9C,       // Default encoding algorithm; FCHECK bits.
		0x01,       // Literal block (final).
		0x06, 0x00, // Literal length.
		0xF9, 0xFF, // Inverse of the literal length.
	}
	enc = append(enc, src...) // Literal bytes.
	enc = hasher.Sum(enc)     // Adler-32 hash.

	// Check that we've constructed a valid zlib-formatted encoding, by
	// checking that decoding enc produces src.
	{
		b := &bytes.Buffer{}
		r, err := zlib.NewReader(bytes.NewReader(enc))
		if err != nil {
			log.Fatalf("NewReader: %v", err)
		}
		if _, err := io.Copy(b, r); err != nil {
			log.Fatalf("Copy: %v", err)
		}
		if got := b.String(); got != src {
			log.Fatalf("zlib check: got %q, want %q", got, src)
		}
	}

	buf := &bytes.Buffer{}
	w := &rac.Writer{
		Writer: buf,
		Codec:  rac.CodecZlib,
	}
	if err := w.AddChunk(uint64(len(src)), enc, 0, 0); err != nil {
		log.Fatalf("AddChunk: %v", err)
	}
	if err := w.Close(); err != nil {
		log.Fatalf("Close: %v", err)
	}

	fmt.Printf("RAC file:\n%s", hex.Dump(buf.Bytes()))

	// Output:
	// RAC file:
	// 00000000  72 c3 63 00 78 9c 01 06  00 f9 ff 4d 6f 72 65 21  |r.c.x......More!|
	// 00000010  0a 07 42 01 bf 72 c3 63  01 65 a9 00 ff 06 00 00  |..B..r.c.e......|
	// 00000020  00 00 00 00 01 04 00 00  00 00 00 01 ff 35 00 00  |.............5..|
	// 00000030  00 00 00 01 01                                    |.....|
}

// ExampleILAStart demonstrates using the low level "rac" package to encode and
// then decode a RAC+Zlib formatted file with IndexLocationAtStart.
//
// The sibling "raczlib" package (TODO) provides a higher level API that is
// easier to use.
//
// See the RAC specification for an explanation of the file format.
func ExampleILAStart() {
	buf := &bytes.Buffer{}
	w := &rac.Writer{
		Writer:        buf,
		Codec:         rac.CodecZlib,
		IndexLocation: rac.IndexLocationAtStart,
		TempFile:      &bytes.Buffer{},
	}

	dict := []byte(" sheep.\n")
	hasher := adler32.New()
	hasher.Write(dict)
	if len(dict) > 0xFFFF {
		log.Fatal("len(dict) is too large")
	}
	encodedDict := []byte{
		uint8(len(dict) >> 0),
		uint8(len(dict) >> 8),
	}
	encodedDict = append(encodedDict, dict...)
	encodedDict = hasher.Sum(encodedDict)
	fmt.Printf("Encoded dictionary resource:\n%s\n", hex.Dump(encodedDict))

	dictResource, err := w.AddResource(encodedDict)
	if err != nil {
		log.Fatalf("AddResource: %v", err)
	}

	chunks := []string{
		"One sheep.\n",
		"Two sheep.\n",
		"Three sheep.\n",
	}

	for i, chunk := range chunks {
		b := &bytes.Buffer{}
		if z, err := zlib.NewWriterLevelDict(b, zlib.BestCompression, dict); err != nil {
			log.Fatalf("NewWriterLevelDict: %v", err)
		} else if _, err := z.Write([]byte(chunk)); err != nil {
			log.Fatalf("Write: %v", err)
		} else if err := z.Close(); err != nil {
			log.Fatalf("Close: %v", err)
		}
		encodedChunk := b.Bytes()

		if err := w.AddChunk(uint64(len(chunk)), encodedChunk, dictResource, 0); err != nil {
			log.Fatalf("AddChunk: %v", err)
		}

		fmt.Printf("Encoded chunk #%d:\n%s\n", i, hex.Dump(encodedChunk))
	}

	if err := w.Close(); err != nil {
		log.Fatalf("Close: %v", err)
	}

	fmt.Printf("RAC file:\n%s", hex.Dump(buf.Bytes()))

	// TODO: decode the encoded bytes (the RAC-formatted bytes) to recover the
	// original "One sheep.\nTwo sheep\.Three sheep.\n" source.

	// Note that these exact bytes depends on the zlib encoder's algorithm, but
	// there is more than one valid zlib encoding of any given input. This
	// "compare to golden output" test is admittedly brittle, as the standard
	// library's zlib package's output isn't necessarily stable across Go
	// releases.

	// Output:
	// Encoded dictionary resource:
	// 00000000  08 00 20 73 68 65 65 70  2e 0a 0b e0 02 6e        |.. sheep.....n|
	//
	// Encoded chunk #0:
	// 00000000  78 f9 0b e0 02 6e f2 cf  4b 85 31 01 01 00 00 ff  |x....n..K.1.....|
	// 00000010  ff 17 21 03 90                                    |..!..|
	//
	// Encoded chunk #1:
	// 00000000  78 f9 0b e0 02 6e 0a 29  cf 87 31 01 01 00 00 ff  |x....n.)..1.....|
	// 00000010  ff 18 0c 03 a8                                    |.....|
	//
	// Encoded chunk #2:
	// 00000000  78 f9 0b e0 02 6e 0a c9  28 4a 4d 85 71 00 01 00  |x....n..(JM.q...|
	// 00000010  00 ff ff 21 6e 04 66                              |...!n.f|
	//
	// RAC file:
	// 00000000  72 c3 63 04 71 b5 00 ff  00 00 00 00 00 00 00 ff  |r.c.q...........|
	// 00000010  0b 00 00 00 00 00 00 ff  16 00 00 00 00 00 00 ff  |................|
	// 00000020  23 00 00 00 00 00 00 01  50 00 00 00 00 00 01 ff  |#.......P.......|
	// 00000030  5e 00 00 00 00 00 01 00  73 00 00 00 00 00 01 00  |^.......s.......|
	// 00000040  88 00 00 00 00 00 01 00  9f 00 00 00 00 00 01 04  |................|
	// 00000050  08 00 20 73 68 65 65 70  2e 0a 0b e0 02 6e 78 f9  |.. sheep.....nx.|
	// 00000060  0b e0 02 6e f2 cf 4b 85  31 01 01 00 00 ff ff 17  |...n..K.1.......|
	// 00000070  21 03 90 78 f9 0b e0 02  6e 0a 29 cf 87 31 01 01  |!..x....n.)..1..|
	// 00000080  00 00 ff ff 18 0c 03 a8  78 f9 0b e0 02 6e 0a c9  |........x....n..|
	// 00000090  28 4a 4d 85 71 00 01 00  00 ff ff 21 6e 04 66     |(JM.q......!n.f|

}
