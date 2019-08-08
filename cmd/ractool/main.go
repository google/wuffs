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

/*
ractool manipulates Random Access Compression (RAC) files.

Usage:

ractool [flags] [input_filename]

If no input_filename is given, stdin is used. Either way, output is written to
stdout.

The flags should include exactly one of -decode or -encode.

When encoding, the input is partitioned into chunks and each chunk is
compressed independently. You can specify the target chunk size in terms of
either its compressed size or decompressed size. By default (if both
-cchunksize and -dchunksize are zero), a 64KiB -cchunksize is used.

You can also specify a -cpagesize, which is similar to but not exactly the same
concept as alignment. If non-zero, padding is inserted into the output to
minimize the number of pages that each chunk occupies. Look for "CPageSize" in
the "package rac" documentation for more details:
https://godoc.org/github.com/google/wuffs/lib/rac

A RAC file consists of an index and the chunks. The index may be either at the
start or at the end of the file. See the RAC specification for more details:
https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md

Examples:

  ractool -decode foo.rac | sha256sum
  ractool -decode -drange=400:500 foo.rac
  ractool -encode foo.dat > foo.rac
  ractool -encode -codec=zlib -dchunksize=256 foo.dat > foo.raczlib

General Flags:

-decode
    whether to decode the input
-encode
    whether to encode the input

Decode-Related Flags:

-drange
    the "i:j" range to decompress, ":8" means the first 8 bytes

Encode-Related Flags:

-cchunksize
    the chunk size (in CSpace)
-codec
    the compression codec (default "zlib")
-cpagesize
    the page size (in CSpace)
-dchunksize
    the chunk size (in DSpace)
-indexlocation
    the index location, "start" or "end" (default "start")
*/
package main

import (
	"bytes"
	"errors"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"strconv"
	"strings"

	"github.com/google/wuffs/lib/raczlib"
)

// TODO: a flag to use a disk-backed (not memory-backed) TempFile.

var (
	decodeFlag = flag.Bool("decode", false, "whether to decode the input")
	encodeFlag = flag.Bool("encode", false, "whether to encode the input")

	// Decode-related flags.
	drangeFlag = flag.String("drange", ":",
		"the \"i:j\" range to decompress, \":8\" means the first 8 bytes")

	// Encode-related flags.
	codecFlag         = flag.String("codec", "zlib", "the compression codec")
	cpagesizeFlag     = flag.Uint64("cpagesize", 0, "the page size (in CSpace)")
	cchunksizeFlag    = flag.Uint64("cchunksize", 0, "the chunk size (in CSpace)")
	dchunksizeFlag    = flag.Uint64("dchunksize", 0, "the chunk size (in DSpace)")
	indexlocationFlag = flag.String("indexlocation", "start",
		"the index location, \"start\" or \"end\"")
)

func usage() {
	// TODO: fmt.Fprintf(os.Stderr, usageStr)
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

	r, usingStdin := io.Reader(os.Stdin), true
	switch flag.NArg() {
	case 0:
		// No-op.
	case 1:
		f, err := os.Open(flag.Arg(0))
		if err != nil {
			return err
		}
		defer f.Close()
		r, usingStdin = f, false
	default:
		return errors.New("too many filenames; the maximum is one")
	}

	if *decodeFlag && !*encodeFlag {
		return decode(r, usingStdin)
	}
	if *encodeFlag && !*decodeFlag {
		return encode(r)
	}
	return errors.New("must specify exactly one of -decode or -encode")
}

// parseRange parses a string like "1:23", returning i=1 and j=23. Either or
// both numbers can be missing, in which case i and/or j will be negative, and
// it is up to the caller to interpret that placeholder value meaningfully.
func parseRange(s string) (i int64, j int64, ok bool) {
	n := strings.IndexByte(s, ':')
	if n < 0 {
		return 0, 0, false
	}
	var err error

	if n == 0 {
		i = -1
	} else if i, err = strconv.ParseInt(s[:n], 0, 64); (err != nil) || (i < 0) {
		return 0, 0, false
	}

	if n+1 >= len(s) {
		j = -1
	} else if j, err = strconv.ParseInt(s[n+1:], 0, 64); (err != nil) || (j < 0) {
		return 0, 0, false
	}

	if (i >= 0) && (j >= 0) && (i > j) {
		return 0, 0, false
	}
	return i, j, true
}

func decode(r io.Reader, usingStdin bool) error {
	i, j, ok := parseRange(*drangeFlag)
	if !ok {
		return fmt.Errorf("invalid -drange")
	}

	rs, ok := r.(io.ReadSeeker)
	if !ok {
		return fmt.Errorf("input is not seekable")
	}

	// Even if the os.File type is a ReadSeeker, it might not actually support
	// seeking. Instead, read all of stdin into memory.
	if usingStdin {
		if input, err := ioutil.ReadAll(r); err != nil {
			return err
		} else {
			rs = bytes.NewReader(input)
		}
	}

	compressedSize, err := rs.Seek(0, io.SeekEnd)
	if err != nil {
		return err
	}
	if _, err := rs.Seek(0, io.SeekStart); err != nil {
		return err
	}

	racReader := io.ReadSeeker(nil)
	switch *codecFlag {
	case "zlib":
		racReader = &raczlib.Reader{
			ReadSeeker:     rs,
			CompressedSize: compressedSize,
		}
	}
	if racReader == nil {
		return errors.New("unsupported -codec")
	}

	decompressedSize, err := racReader.Seek(0, io.SeekEnd)
	if err != nil {
		return err
	}
	if i < 0 {
		i = 0
	}
	if (j < 0) || (j > decompressedSize) {
		j = decompressedSize
	}
	if i >= j {
		return nil
	}
	if _, err := racReader.Seek(i, io.SeekStart); err != nil {
		return err
	}

	_, err = io.Copy(os.Stdout, &io.LimitedReader{
		R: racReader,
		N: j - i,
	})
	return err
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
