// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// +build ignore

package main

// extract-flate-offsets.go extracts the start and end offsets of the
// flate-compressed data wrapped in a .gz file.
//
// Usage: go run extract-flate-offsets.go foo.gz bar.gz

import (
	"bytes"
	"compress/flate"
	"crypto/md5"
	"fmt"
	"io/ioutil"
	"os"
)

// GZIP wraps a header and footer around flate data. The format is described in
// RFC 1952: https://www.ietf.org/rfc/rfc1952.txt
const (
	flagText    = 1 << 0
	flagHCRC    = 1 << 1
	flagExtra   = 1 << 2
	flagName    = 1 << 3
	flagComment = 1 << 4
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	for _, a := range os.Args[1:] {
		if err := decode(a); err != nil {
			return err
		}
	}
	return nil
}

func decode(filename string) error {
	src, err := ioutil.ReadFile(filename)
	if err != nil {
		return err
	}

	const (
		headerSize = 10
		footerSize = 8
	)
	if len(src) < headerSize+footerSize || src[0] != 0x1F || src[1] != 0x8B || src[2] != 0x08 {
		return fmt.Errorf("not a GZIP")
	}
	if len(src) >= 0x10000000 {
		return fmt.Errorf("file too large")
	}
	flags := src[3]
	i := headerSize
	src = src[:len(src)-footerSize]

	if flags&flagExtra != 0 {
		return fmt.Errorf("TODO: support gzip extra flag")
	}

	if flags&flagName != 0 {
		if i, err = readString(src, i); err != nil {
			return err
		}
	}

	if flags&flagComment != 0 {
		if i, err = readString(src, i); err != nil {
			return err
		}
	}

	if flags&flagHCRC != 0 {
		return fmt.Errorf("TODO: support gzip HCRC flag")
	}

	// As a sanity check, the result should be valid flate.
	hash, err := checkFlate(src[i:])
	if err != nil {
		return err
	}

	fmt.Printf("%7d %7d %x  %s\n", i, len(src), hash, filename)
	return nil
}

func readString(src []byte, i int) (int, error) {
	for {
		if i >= len(src) {
			return 0, fmt.Errorf("bad GZIP string")
		}
		if src[i] == 0 {
			return i + 1, nil
		}
		i++
	}
}

func checkFlate(x []byte) ([md5.Size]byte, error) {
	rc := flate.NewReader(bytes.NewReader(x))
	defer rc.Close()
	x, err := ioutil.ReadAll(rc)
	if err != nil {
		return [md5.Size]byte{}, fmt.Errorf("data is not valid flate: %v", err)
	}
	return md5.Sum(x), nil
}
