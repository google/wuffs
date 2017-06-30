// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// +build ignore

package main

// extract-giflzw.go extracts the LZW-compressed block data in a GIF image. The
// initial byte in each written file is the LZW literal width.
//
// Usage: go run extract-giflzw.go foo.gif bar.gif

import (
	"bytes"
	"compress/lzw"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	for _, a := range os.Args[1:] {
		data, err := decode(a)
		if err != nil {
			return err
		}
		// As a sanity check, the result should be valid LZW.
		if err := checkLZW(data); err != nil {
			return err
		}
		b := a[:len(a)-len(filepath.Ext(a))]
		if err := ioutil.WriteFile(b+".indexes.giflzw", data, 0644); err != nil {
			return err
		}
	}
	return nil
}

func decode(filename string) (ret []byte, err error) {
	src, err := ioutil.ReadFile(filename)
	if err != nil {
		return nil, err
	}

	// Read the header (6 bytes) and screen descriptor (7 bytes).
	if len(src) < 6+7 {
		return nil, fmt.Errorf("not a GIF")
	}
	switch string(src[:6]) {
	case "GIF87a", "GIF89a":
		// No-op.
	default:
		return nil, fmt.Errorf("not a GIF")
	}
	ctSize := 0
	if src[10]&fColorTable != 0 {
		ctSize = 1 << (1 + src[10]&fColorTableBitsMask)
	}
	if src, err = skipColorTable(src[13:], ctSize); err != nil {
		return nil, err
	}

	seenID := false
	for len(src) > 0 {
		switch src[0] {
		case sExtension:
			if src, err = skipExtension(src[1:]); err != nil {
				return nil, err
			}
		case sImageDescriptor:
			compressed := []byte(nil)
			if src, compressed, err = readImageDescriptor(src[1:]); err != nil {
				return nil, err
			}
			// For an animated GIF, take only the first frame.
			if !seenID {
				seenID = true
				ret = compressed
			}
		case sTrailer:
			if len(src) != 1 {
				return nil, fmt.Errorf("extraneous data after GIF trailer section")
			}
			return ret, nil
		default:
			return nil, fmt.Errorf("unsupported GIF section")
		}
	}
	return nil, fmt.Errorf("missing GIF trailer section")
}

func skipColorTable(src []byte, ctSize int) (src1 []byte, err error) {
	if ctSize == 0 {
		return src, nil
	}
	n := 3 * ctSize
	if len(src) < n {
		return nil, fmt.Errorf("short color table")
	}
	return src[n:], nil
}

func skipExtension(src []byte) (src1 []byte, err error) {
	if len(src) < 2 {
		return nil, fmt.Errorf("bad GIF extension")
	}
	ext := src[0]
	blockSize := int(src[1])
	if len(src) < 2+blockSize {
		return nil, fmt.Errorf("bad GIF extension")
	}
	src = src[2+blockSize:]
	switch ext {
	case ePlainText, eGraphicControl, eComment, eApplication:
		src1, _, err = readBlockData(src, nil)
		return src1, err
	}
	return nil, fmt.Errorf("unsupported GIF extension")
}

func readImageDescriptor(src []byte) (src1 []byte, ret1 []byte, err error) {
	if len(src) < 9 {
		return nil, nil, fmt.Errorf("bad GIF image descriptor")
	}
	ctSize := 0
	if src[8]&fColorTable != 0 {
		ctSize = 1 << (1 + src[8]&fColorTableBitsMask)
	}
	if src, err = skipColorTable(src[9:], ctSize); err != nil {
		return nil, nil, err
	}

	if len(src) < 1 {
		return nil, nil, fmt.Errorf("bad GIF image descriptor")
	}
	literalWidth := src[0]
	if literalWidth < 2 || 8 < literalWidth {
		return nil, nil, fmt.Errorf("bad GIF literal width")
	}
	return readBlockData(src[1:], []byte{literalWidth})
}

func readBlockData(src []byte, ret []byte) (src1 []byte, ret1 []byte, err error) {
	for {
		if len(src) < 1 {
			return nil, nil, fmt.Errorf("bad GIF block")
		}
		n := int(src[0]) + 1
		if len(src) < n {
			return nil, nil, fmt.Errorf("bad GIF block")
		}
		ret = append(ret, src[1:n]...)
		src = src[n:]
		if n == 1 {
			return src, ret, nil
		}
	}
}

func checkLZW(x []byte) error {
	if len(x) == 0 {
		return fmt.Errorf("missing GIF literal width")
	}
	rc := lzw.NewReader(bytes.NewReader(x[1:]), lzw.LSB, int(x[0]))
	defer rc.Close()
	_, err := ioutil.ReadAll(rc)
	if err != nil {
		return fmt.Errorf("block data is not valid LZW: %v", err)
	}
	return nil
}

// The constants below are intrinsic to the GIF file format. The spec is
// http://www.w3.org/Graphics/GIF/spec-gif89a.txt
const (
	// Flags.
	fColorTableBitsMask = 0x07
	fColorTable         = 0x80

	// Sections.
	sExtension       = 0x21
	sImageDescriptor = 0x2C
	sTrailer         = 0x3B

	// Extensions.
	ePlainText      = 0x01
	eGraphicControl = 0xF9
	eComment        = 0xFE
	eApplication    = 0xFF
)
