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

//go:generate go run gen.go

/*
ractool manipulates Random Access Compression (RAC) files.

See the RAC specification for more details:
https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md

Usage:

ractool [flags] [input_filename]

If no input_filename is given, stdin is used. Either way, output is written to
stdout.

The flags should include exactly one of -decode or -encode.

When encoding, the input is partitioned into chunks and each chunk is
compressed independently. You can specify the target chunk size in terms of
either its compressed size or decompressed size. By default (if both
-cchunksize and -dchunksize are zero), a 64KiB -dchunksize is used.

You can also specify a -cpagesize, which is similar to but not exactly the same
concept as alignment. If non-zero, padding is inserted into the output to
minimize the number of pages that each chunk occupies. Look for "CPageSize" in
the "package rac" documentation for more details:
https://godoc.org/github.com/google/wuffs/lib/rac

A RAC file consists of an index and the chunks. The index may be either at the
start or at the end of the file. At the start results in slightly smaller and
slightly more efficient RAC files, but the encoding process needs more memory
or temporary disk space.

Examples:

  ractool -decode foo.rac | sha256sum
  ractool -decode -drange=400:500 foo.rac
  ractool -encode foo.dat > foo.rac
  ractool -encode -codec=zlib -dchunksize=256k foo.dat > foo.rac

The "400:500" flag value means the 100 bytes ranging from a DSpace offset
(offset in terms of decompressed bytes, not compressed bytes) of 400
(inclusive) to 500 (exclusive). Either or both bounds may be omitted, similar
to Go slice syntax. A "400:" flag value would mean ranging from 400 (inclusive)
to the end of the decompressed file.

The "256k" flag value means 256 kibibytes (262144 bytes). Similarly, "1m" and
"1M" both mean 1 mebibyte (1048576 bytes).

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

	"github.com/google/wuffs/lib/rac"
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
	cpagesizeFlag     = flag.String("cpagesize", "0", "the page size (in CSpace)")
	cchunksizeFlag    = flag.String("cchunksize", "0", "the chunk size (in CSpace)")
	dchunksizeFlag    = flag.String("dchunksize", "0", "the chunk size (in DSpace)")
	indexlocationFlag = flag.String("indexlocation", "start",
		"the index location, \"start\" or \"end\"")
)

func usage() {
	os.Stderr.WriteString(usageStr)
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

// parseNumber converts strings like "3", "4k" and "0x50" to the integers 3,
// 4096 and 48. It returns a negative value if and only if an error is
// encountered.
func parseNumber(s string) int64 {
	if s == "" {
		return -1
	}
	shift := uint32(0)
	switch n := len(s) - 1; s[n] {
	case 'k', 'K':
		shift, s = 10, s[:n]
	case 'm', 'M':
		shift, s = 20, s[:n]
	case 'g', 'G':
		shift, s = 30, s[:n]
	case 't', 'T':
		shift, s = 40, s[:n]
	case 'p', 'P':
		shift, s = 50, s[:n]
	case 'e', 'E':
		shift, s = 60, s[:n]
	}
	i, err := strconv.ParseInt(s, 0, 64)
	if (err != nil) || (i < 0) {
		return -1
	}
	const int64Max = (1 << 63) - 1
	if i > (int64Max >> shift) {
		return -1
	}
	return i << shift
}

// parseRange parses a string like "1:23", returning i=1 and j=23. Either or
// both numbers can be missing, in which case i and/or j will be negative, and
// it is up to the caller to interpret that placeholder value meaningfully.
func parseRange(s string) (i int64, j int64, ok bool) {
	n := strings.IndexByte(s, ':')
	if n < 0 {
		return 0, 0, false
	}

	if n == 0 {
		i = -1
	} else if i = parseNumber(s[:n]); i < 0 {
		return 0, 0, false
	}

	if n+1 >= len(s) {
		j = -1
	} else if j = parseNumber(s[n+1:]); j < 0 {
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

	racReader := &rac.Reader{
		ReadSeeker:     rs,
		CompressedSize: compressedSize,
	}
	switch *codecFlag {
	case "zlib":
		racReader.CodecReaders = []rac.CodecReader{&raczlib.CodecReader{}}
	default:
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
	indexLocation := rac.IndexLocation(0)
	switch *indexlocationFlag {
	case "start":
		indexLocation = rac.IndexLocationAtStart
	case "end":
		indexLocation = rac.IndexLocationAtEnd
	default:
		return errors.New("invalid -indexlocation")
	}

	cchunksize := parseNumber(*cchunksizeFlag)
	if cchunksize < 0 {
		return errors.New("invalid -cchunksize")
	}
	cpagesize := parseNumber(*cpagesizeFlag)
	if cpagesize < 0 {
		return errors.New("invalid -cpagesize")
	}
	dchunksize := parseNumber(*dchunksizeFlag)
	if dchunksize < 0 {
		return errors.New("invalid -dchunksize")
	}

	if (cchunksize != 0) && (dchunksize != 0) {
		return errors.New("must specify none or one of -cchunksize or -dchunksize")
	} else if (cchunksize == 0) && (dchunksize == 0) {
		dchunksize = 65536 // 64 KiB.
	}

	w := &rac.Writer{
		Writer:        os.Stdout,
		IndexLocation: indexLocation,
		TempFile:      &bytes.Buffer{},
		CPageSize:     uint64(cpagesize),
		CChunkSize:    uint64(cchunksize),
		DChunkSize:    uint64(dchunksize),
	}
	switch *codecFlag {
	case "zlib":
		w.CodecWriter = &raczlib.CodecWriter{}
	default:
		return errors.New("unsupported -codec")
	}

	if _, err := io.Copy(w, r); err != nil {
		return err
	}
	return w.Close()
}
