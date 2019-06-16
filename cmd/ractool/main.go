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

// ----------------

// ractool manipulates Random Access Compression (RAC) files.
package main

import (
	"bytes"
	"errors"
	"flag"
	"fmt"
	"io"
	"os"

	"github.com/google/wuffs/lib/raczlib"
)

// TODO: a flag to use a disk-backed (not memory-backed) TempFile.

var (
	decodeFlag = flag.Bool("decode", false, "whether to decode the input")
	encodeFlag = flag.Bool("encode", false, "whether to encode the input")

	codecFlag         = flag.String("codec", "zlib", "when encoding, the compression codec")
	cpagesizeFlag     = flag.Uint64("cpagesize", 0, "when encoding, the page size (in CSpace)")
	cchunksizeFlag    = flag.Uint64("cchunksize", 0, "when encoding, the chunk size (in CSpace)")
	dchunksizeFlag    = flag.Uint64("dchunksize", 0, "when encoding, the chunk size (in DSpace)")
	indexlocationFlag = flag.String("indexlocation", "start",
		"when encoding, the index location, \"start\" or \"end\"")
)

const usageStr = `usage: ractool [flags] [input_filename]

If no input_filename is given, stdin is used. Either way, output is written to
stdout.

The flags should include exactly one of -decode or -encode.

Decoding is not yet implemented.

When encoding, the input is partitioned into chunks and each chunk is
compressed independently. You can specify the target chunk size in terms of
either its compressed size or decompressed size. By default (if both
-cchunksize and -dchunksize are zero), a 64KiB -cchunksize is used.

You can also specify a -cpagesize, which is similar to but not exactly the same
concept as alignment. If non-zero, padding is inserted into the output to
minimize the number of pages that each chunk occupies. Look for "CPageSize" in
the "package rac" documentation for more details.

A RAC file consists of an index and the chunks. The index may be either at the
start or at the end of the file. See the RAC specification for more details:

https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md

`

func usage() {
	fmt.Fprintf(os.Stderr, usageStr)
	flag.PrintDefaults()
}

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Usage = usage
	flag.Parse()

	r := io.Reader(os.Stdin)
	switch flag.NArg() {
	case 0:
		// No-op.
	case 1:
		f, err := os.Open(flag.Arg(0))
		if err != nil {
			return err
		}
		defer f.Close()
		r = f
	default:
		return errors.New("too many filenames; the maximum is one")
	}

	if *decodeFlag && !*encodeFlag {
		return decode(r)
	}
	if *encodeFlag && !*decodeFlag {
		return encode(r)
	}
	return errors.New("must specify exactly one of -decode or -encode")
}

func decode(r io.Reader) error {
	return errors.New("TODO: implement a decoder")
}

func encode(r io.Reader) error {
	indexLocation := raczlib.IndexLocation(0)
	switch *indexlocationFlag {
	case "start":
		indexLocation = raczlib.IndexLocationAtStart
	case "end":
		indexLocation = raczlib.IndexLocationAtEnd
	default:
		return errors.New("invalid -indexlocation")
	}

	if (*cchunksizeFlag != 0) && (*dchunksizeFlag != 0) {
		return errors.New("must specify none or one of -cchunksize or -dchunksize")
	} else if (*cchunksizeFlag == 0) && (*dchunksizeFlag == 0) {
		*cchunksizeFlag = 65536 // 64 KiB.
	}

	switch *codecFlag {
	case "zlib":
		w := &raczlib.Writer{
			Writer:        os.Stdout,
			IndexLocation: indexLocation,
			TempFile:      &bytes.Buffer{},
			CPageSize:     *cpagesizeFlag,
			CChunkSize:    *cchunksizeFlag,
			DChunkSize:    *dchunksizeFlag,
		}
		if _, err := io.Copy(w, r); err != nil {
			return err
		}
		return w.Close()
	}
	return errors.New("unsupported -codec")
}
