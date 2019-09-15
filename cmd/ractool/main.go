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

Random access means that it is possible to reconstruct part of the decompressed
file, starting at a given offset into the decompressed file, without always
having to first decompress all of the preceding data.

In comparison to some other popular compression formats, all four of the Zlib,
Brotli, LZ4 and Zstandard specifications explicitly contain the identical
phrase: "the data format defined by this specification does not attempt to
allow random access to compressed data".

See the RAC specification for more details:
https://github.com/google/wuffs/blob/master/doc/spec/rac-spec.md

Usage:

    ractool [flags] [input_filename]

If no input_filename is given, stdin is used. Either way, output is written to
stdout.

The flags should include exactly one of -decode or -encode.

By default, a RAC file's chunks are decoded in parallel, using more total CPU
time to substantially reduce the real (wall clock) time taken. Batch (instead
of interactive) processing of many RAC files may want to pass -singlethreaded
to prefer minimizing total CPU time.

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
    ractool -decode -drange=400..500 foo.rac
    ractool -encode foo.dat > foo.rac
    ractool -encode -codec=zlib -dchunksize=256k foo.dat > foo.rac

The "400..500" flag value means the 100 bytes ranging from a DSpace offset
(offset in terms of decompressed bytes, not compressed bytes) of 400
(inclusive) to 500 (exclusive). Either or both bounds may be omitted, similar
to Rust slice syntax. A "400.." flag value would mean ranging from 400
(inclusive) to the end of the decompressed file.

The "256k" flag value means 256 kibibytes (262144 bytes), as does "256K".
Similarly, "1m" and "1M" both mean 1 mebibyte (1048576 bytes).

General Flags:

    -decode
        whether to decode the input
    -encode
        whether to encode the input

Decode-Related Flags:

    -drange
        the "i..j" range to decompress, "..8" means the first 8 bytes
    -singlethreaded
        whether to decode on a single execution thread

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
    -resources
        comma-separated list of resource files, such as shared dictionaries

Installation:

Like any other implemented-in-Go program, to install the ractool program:

    go get github.com/google/wuffs/cmd/ractool

Extended Example:

    --------
    $ # Fetch and unzip the enwik8 test file, a sample of Wikipedia.
    $ wget http://mattmahoney.net/dc/enwik8.zip
    $ unzip enwik8.zip

    $ # Create a shared dictionary. The dictionary_generator program
    $ # comes from https://github.com/google/brotli
    $ dictionary_generator --chunk_len=64k dict.dat enwik8

    $ # RAC-encode it twice, with and without that shared dictionary.
    $ ractool -encode -resources=dict.dat enwik8 > shared.rac
    $ ractool -encode                     enwik8 > vanilla.rac

    $ # The size overhead (compared to the .zip) is about 2.4% or 4.8%,
    $ # depending on whether we used a shared dictionary.
    $ ls -l
    total 207012
    -rw-r--r-- 1 tao tao     16384 Aug  9 19:12 dict.dat
    -rw-r--r-- 1 tao tao 100000000 Jun  2  2011 enwik8
    -rw-r--r-- 1 tao tao  36445475 Sep  2  2011 enwik8.zip
    -rw-r--r-- 1 tao tao  37320896 Aug  9 19:16 shared.rac
    -rw-r--r-- 1 tao tao  38185178 Aug  9 19:17 vanilla.rac

    $ # Check that the decompressed forms all match.
    $ cat enwik8                  | sha256sum
    2b49720ec4d78c3c9fabaee6e4179a5e997302b3a70029f30f2d582218c024a8  -
    $ unzip -p enwik8.zip         | sha256sum
    2b49720ec4d78c3c9fabaee6e4179a5e997302b3a70029f30f2d582218c024a8  -
    $ ractool -decode shared.rac  | sha256sum
    2b49720ec4d78c3c9fabaee6e4179a5e997302b3a70029f30f2d582218c024a8  -
    $ ractool -decode vanilla.rac | sha256sum
    2b49720ec4d78c3c9fabaee6e4179a5e997302b3a70029f30f2d582218c024a8  -

    $ # Compare how long it takes to produce 8 bytes from the middle of
    $ # the decompressed file, which happens to be the word "Business".
    $ time unzip -p enwik8.zip | dd if=/dev/stdin status=none \
    >     iflag=skip_bytes,count_bytes skip=50000000 count=8
    Business
    real    0m0.392s
    user    0m0.407s
    sys     0m0.118s
    $ time ractool -decode -drange=50000000..50000008 shared.rac
    Business
    real    0m0.006s
    user    0m0.003s
    sys     0m0.003s

    $ # A RAC file's chunks can be decoded in parallel, unlike ZIP,
    $ # substantially reducing the real (wall clock) time taken even
    $ # though both of these files use DEFLATE (RFC 1951) compression.
    $ time unzip -p enwik8.zip                        > /dev/null
    real    0m0.737s
    user    0m0.713s
    sys     0m0.025s
    $ time ractool -decode -singlethreaded shared.rac > /dev/null
    real    0m0.523s
    user    0m0.508s
    sys     0m0.028s
    $ time ractool -decode                 shared.rac > /dev/null
    real    0m0.052s
    user    0m0.657s
    sys     0m0.049s
    --------
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
	"runtime"
	"strconv"
	"strings"

	"github.com/google/wuffs/lib/rac"
	"github.com/google/wuffs/lib/raclz4"
	"github.com/google/wuffs/lib/raczlib"
	"github.com/google/wuffs/lib/raczstd"
)

// TODO: a flag to use a disk-backed (not memory-backed) TempFile.

var (
	decodeFlag = flag.Bool("decode", false, "whether to decode the input")
	encodeFlag = flag.Bool("encode", false, "whether to encode the input")

	// Decode-related flags.
	drangeFlag = flag.String("drange", "..",
		"the \"i..j\" range to decompress, \"..8\" means the first 8 bytes")
	singlethreadedFlag = flag.Bool("singlethreaded", false,
		"whether to decode on a single execution thread")

	// Encode-related flags.
	codecFlag         = flag.String("codec", "zlib", "the compression codec")
	cpagesizeFlag     = flag.String("cpagesize", "0", "the page size (in CSpace)")
	cchunksizeFlag    = flag.String("cchunksize", "0", "the chunk size (in CSpace)")
	dchunksizeFlag    = flag.String("dchunksize", "0", "the chunk size (in DSpace)")
	indexlocationFlag = flag.String("indexlocation", "start",
		"the index location, \"start\" or \"end\"")
	resourcesFlag = flag.String("resources", "",
		"comma-separated list of resource files, such as shared dictionaries")
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

	inFile := os.Stdin
	switch flag.NArg() {
	case 0:
		// No-op.
	case 1:
		f, err := os.Open(flag.Arg(0))
		if err != nil {
			return err
		}
		defer f.Close()
		inFile = f
	default:
		return errors.New("too many filenames; the maximum is one")
	}

	if *decodeFlag && !*encodeFlag {
		return decode(inFile)
	}
	if *encodeFlag && !*decodeFlag {
		return encode(inFile)
	}
	return errors.New("must specify exactly one of -decode, -encode or -help")
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

// parseRange parses a string like "1..23", returning i=1 and j=23. Either or
// both numbers can be missing, in which case i and/or j will be negative, and
// it is up to the caller to interpret that placeholder value meaningfully.
//
// Like Rust range syntax, it also accepts "i..=j", not just "i..j", in which
// case the upper bound is inclusive, not exclusive.
func parseRange(s string) (i int64, j int64, ok bool) {
	n := strings.Index(s, "..")
	if n < 0 {
		return 0, 0, false
	}

	if n == 0 {
		i = -1
	} else if i = parseNumber(s[:n]); i < 0 {
		return 0, 0, false
	}

	// Look for "i..j" versus "i..=j".
	eq := 0
	if (n+2 < len(s)) && (s[n+2] == '=') {
		eq = 1
	}

	if n+2+eq >= len(s) {
		if eq > 0 {
			return 0, 0, false
		}
		j = -1
	} else if j = parseNumber(s[n+2+eq:]); j < 0 {
		return 0, 0, false
	} else {
		j += int64(eq)
		if j < 0 {
			return 0, 0, false
		}
	}

	if (i >= 0) && (j >= 0) && (i > j) {
		return 0, 0, false
	}
	return i, j, true
}

func decode(inFile *os.File) error {
	i, j, ok := parseRange(*drangeFlag)
	if !ok {
		return fmt.Errorf("invalid -drange")
	}

	rs := io.ReadSeeker(inFile)
	compressedSize, err := inFile.Seek(0, io.SeekEnd)
	if err != nil {
		// This seek-to-end error isn't fatal. The input might not actually be
		// seekable, despite being an *os.File: "cat foo | ractool -decode".
		// Instead, read all of the inFile into memory.
		if inBytes, err := ioutil.ReadAll(inFile); err != nil {
			return err
		} else {
			rs = bytes.NewReader(inBytes)
			compressedSize = int64(len(inBytes))
		}
	}
	chunkReader := &rac.ChunkReader{
		ReadSeeker:     rs,
		CompressedSize: compressedSize,
	}
	decompressedSize, err := chunkReader.DecompressedSize()
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

	r := &rac.Reader{
		ReadSeeker:     rs,
		CompressedSize: compressedSize,
		CodecReaders: []rac.CodecReader{
			&raclz4.CodecReader{},
			&raczlib.CodecReader{},
			&raczstd.CodecReader{},
		},
	}

	// The r.Close method might need to wait for its goroutines to shut down
	// cleanly, to guarantee that the underlying io.ReadSeeker won't be used
	// after r.Close returns.
	//
	// But here, we're a program ("package main"), not a library. After this
	// function (decode) returns, we'll exit the program. There's no need to
	// hold that up, so we call CloseWithoutWaiting instead of Close.
	defer r.CloseWithoutWaiting()

	if !*singlethreadedFlag {
		n := runtime.NumCPU()
		// After 16 workers, we seem to hit diminishing speed returns, but
		// still face increasing memory costs.
		if n > 16 {
			n = 16
		}
		r.Concurrency = n
	}
	if err := r.SeekRange(i, j); err != nil {
		return err
	}
	_, err = io.Copy(os.Stdout, r)
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
	case "lz4":
		w.CodecWriter = &raclz4.CodecWriter{}
	case "zlib":
		w.CodecWriter = &raczlib.CodecWriter{}
	case "zstd":
		w.CodecWriter = &raczstd.CodecWriter{}
	default:
		return errors.New("unsupported -codec")
	}

	if *resourcesFlag != "" {
		for _, filename := range strings.Split(*resourcesFlag, ",") {
			resource, err := ioutil.ReadFile(filename)
			if err != nil {
				return err
			}
			w.ResourcesData = append(w.ResourcesData, resource)
		}
	}

	if _, err := io.Copy(w, r); err != nil {
		return err
	}
	return w.Close()
}
