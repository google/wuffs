// Copyright 2025 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

// ----------------

// handsum decodes and encodes the Handsum lossy image file format.
package main

import (
	"bytes"
	"errors"
	"flag"
	"image"
	"image/png"
	"os"

	"github.com/google/wuffs/lib/handsum"

	_ "image/gif"
	_ "image/jpeg"

	_ "golang.org/x/image/bmp"
	_ "golang.org/x/image/tiff"
	_ "golang.org/x/image/webp"
)

var (
	decodeFlag    = flag.Bool("decode", false, "whether to decode the input")
	encodeFlag    = flag.Bool("encode", false, "whether to encode the input")
	roundtripFlag = flag.Bool("roundtrip", false, "whether to encode-and-decode the input")
)

const usageStr = `handsum decodes and encodes the Handsum lossy image file format.

Usage: choose one of

    handsum -decode    [path]
    handsum -encode    [path]
    handsum -roundtrip [path]

The path to the input image file is optional. If omitted, stdin is read.

The output image (in Handsum or PNG format) is written to stdout.

Decode inputs Handsum and outputs PNG.
Encode inputs BMP, GIF, JPEG, PNG, TIFF or WEBP and outputs Handsum.
Roundtrip is equivalent to encode (to an ephemeral file) and then decode.
`

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Usage = func() { os.Stderr.WriteString(usageStr) }
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

	if *decodeFlag && !*encodeFlag && !*roundtripFlag {
		return decode(inFile)
	}
	if !*decodeFlag && *encodeFlag && !*roundtripFlag {
		return encode(inFile)
	}
	if !*decodeFlag && !*encodeFlag && *roundtripFlag {
		return roundtrip(inFile)
	}
	return errors.New("must specify exactly one of -decode, -encode, -roundtrip or -help")
}

func decode(inFile *os.File) error {
	src, err := handsum.Decode(inFile)
	if err != nil {
		return err
	}
	return png.Encode(os.Stdout, src)
}

func encode(inFile *os.File) error {
	src, _, err := image.Decode(inFile)
	if err != nil {
		return err
	}
	return handsum.Encode(os.Stdout, src, nil)
}

func roundtrip(inFile *os.File) error {
	src, _, err := image.Decode(inFile)
	if err != nil {
		return err
	}
	buf := &bytes.Buffer{}
	err = handsum.Encode(buf, src, nil)
	if err != nil {
		return err
	}
	dst, err := handsum.Decode(buf)
	if err != nil {
		return err
	}
	return png.Encode(os.Stdout, dst)
}
