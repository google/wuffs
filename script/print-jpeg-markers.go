// Copyright 2023 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

//go:build ignore
// +build ignore

package main

// print-jpeg-markers.go prints a JPEG's markers' position and type.
//
// Usage: go run print-jpeg-markers.go foo.jpeg

import (
	"flag"
	"fmt"
	"os"
)

var (
	xFlag = flag.Bool("x", false, "print extra information parsed from marker payloads")
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Parse()
	args := flag.Args()
	if len(args) != 1 {
		return fmt.Errorf("usage: progname filename.jpeg")
	}
	src, err := os.ReadFile(args[0])
	if err != nil {
		return err
	}

	pos := 0
	if len(src) < 2 {
		return posError(pos, len(src))
	}
	for pos <= (len(src) - 2) {
		if src[pos] != 0xFF {
			return posError(pos, len(src))
		}
		marker := src[pos+1]
		fmt.Printf("pos = 0x%08X = %10d    marker = 0xFF 0x%02X  %s\n", pos, pos, marker, names[marker])
		pos += 2
		switch {
		case (0xD0 <= marker) && (marker < 0xD9):
			// RSTn (Restart) and SOI (Start Of Image) markers have no payload.
			continue
		case marker == 0xD9:
			// EOI (End Of Image) marker.
			return nil
		}

		if pos >= (len(src) - 2) {
			return posError(pos, len(src))
		}
		payloadLength := (int(src[pos]) << 8) | int(src[pos+1])
		pos += payloadLength
		if (payloadLength < 2) || (pos < 0) || (pos > len(src)) {
			return posError(pos, len(src))
		}
		if *xFlag {
			parse(marker, src[pos+2-payloadLength:pos])
		}

		if marker != 0xDA { // SOS (Start Of Scan) marker.
			continue
		}
		for pos < len(src) {
			if src[pos] != 0xFF {
				pos += 1
			} else if pos >= (len(src) - 1) {
				return posError(pos, len(src))
			} else if m := src[pos+1]; (m != 0x00) && ((m < 0xD0) || (0xD7 < m)) {
				break
			} else {
				pos += 2
			}
		}
	}
	return posError(pos, len(src))
}

func posError(pos int, lenSrc int) error {
	return fmt.Errorf("bad JPEG, pos = 0x%08X = %10d, len(src) = 0x%08X = %10d",
		pos, pos, lenSrc, lenSrc)
}

func parse(marker byte, payload []byte) {
	if ((0xE0 <= marker) && (marker <= 0xEF)) || (marker == 0xFE) {
		n := len(payload)
		if n > 16 {
			n = 16
		}
		buf := [16]byte{}
		for i := range buf {
			buf[i] = '.'
			if i >= len(payload) {
				continue
			} else if c := payload[i]; (0x20 <= c) && (c < 0x7F) {
				buf[i] = c
			}
		}
		fmt.Printf("#                                    contents[..%2d]      %s\n", n, buf[:n])
	}

	switch marker {
	case 0xC0, 0xC1, 0xC2:
		parseSOF(marker, payload)
	case 0xC4:
		parseDHT(marker, payload)
	case 0xDA:
		parseSOS(marker, payload)
	case 0xDB:
		parseDQT(marker, payload)

		// For APPN markers, see https://exiftool.org/TagNames/JPEG.html
	case 0xE0:
		if hasPrefix(payload, "JFIF\x00") {
			parseJFIF(marker, payload)
		}
	case 0xE1:
		if hasPrefix(payload, "Exif\x00\x00") {
			parseExif(marker, payload)
		} else if hasPrefix(payload, "http://ns.adobe.com/xap/1.0/\x00") {
			parseXMP(marker, payload)
		} else if hasPrefix(payload, "http://ns.adobe.com/xmp/extension/\x00") {
			parseExtendedXMP(marker, payload)
		}
	case 0xE2:
		if hasPrefix(payload, "ICC_PROFILE\x00") {
			parseICC(marker, payload)
		}
	case 0xED:
		if hasPrefix(payload, "Photoshop 3.0") {
			parsePhotoshop(marker, payload)
		}
	case 0xEE:
		if hasPrefix(payload, "Adobe") {
			parseAdobe(marker, payload)
		}
	}
}

func parseDHT(marker byte, payload []byte) {
	for len(payload) > 17 {
		c := payload[0] >> 4
		h := payload[0] & 15
		if (c > 1) || (h > 3) {
			return
		}
		payload = payload[1:]

		totalCount := 0
		for _, count := range payload[:16] {
			totalCount += int(count)
		}
		payload = payload[16:]

		if len(payload) < totalCount {
			return
		}
		payload = payload[totalCount:]

		which := 'D'
		if c > 0 {
			which = 'A'
		}
		fmt.Printf("#                                    h_identifier        %cC | %X\n", which, h)
	}
}

func parseDQT(marker byte, payload []byte) {
	for len(payload) > 0 {
		qIdent := payload[0] & 15
		if qIdent > 3 {
			return
		}
		bytesPerElement := 1 + (int(payload[0]) >> 4)
		if bytesPerElement > 2 {
			return
		}
		n := (64 * bytesPerElement) + 1
		if len(payload) < n {
			return
		}
		payload = payload[n:]
		fmt.Printf("#                                    q_identifier        %02X\n", qIdent)
	}
}

func parseSOF(marker byte, payload []byte) {
	numComponents := 0
	switch lp := len(payload); lp {
	case 9, 12, 15, 18:
		numComponents = (lp - 6) / 3
	default:
		return
	}

	h := (uint32(payload[1]) << 8) | uint32(payload[2])
	w := (uint32(payload[3]) << 8) | uint32(payload[4])
	isRGB := ""
	if (numComponents == 3) &&
		(payload[6] == 'R') && (payload[9] == 'G') && (payload[12] == 'B') {
		isRGB = " (RGB)"
	}

	cTable := [4]byte{}
	sTable := [4]byte{}
	qTable := [4]byte{}
	for i := 0; i < numComponents; i++ {
		cTable[i] = payload[(3*i)+6]
		sTable[i] = payload[(3*i)+7]
		qTable[i] = payload[(3*i)+8]
	}

	fmt.Printf("#                                    width               0x%04X = %5d\n", w, w)
	fmt.Printf("#                                    height              0x%04X = %5d\n", h, h)
	fmt.Printf("#                                    bit_depth           %d\n", payload[0])
	fmt.Printf("#                                    num_components      %d\n", numComponents)
	fmt.Printf("#                                    c_identifiers       % 02X%s\n", cTable[:numComponents], isRGB)
	fmt.Printf("#                                    subsampling H|V     % 02X\n", sTable[:numComponents])
	fmt.Printf("#                                    q_selectors         % 02X\n", qTable[:numComponents])
}

func parseSOS(marker byte, payload []byte) {
	numComponents := 0
	switch lp := len(payload); lp {
	case 6, 8, 10, 12:
		numComponents = (lp - 4) / 2
	default:
		return
	}
	if numComponents != int(payload[0]) {
		return
	}

	cTable := [4]byte{}
	hTable := [4]byte{}
	for i := 0; i < numComponents; i++ {
		cTable[i] = payload[(2*i)+1]
		hTable[i] = payload[(2*i)+2]
	}

	progSS := payload[(2*numComponents)+1] & 63
	progSE := payload[(2*numComponents)+2] & 63
	progAH := payload[(2*numComponents)+3] >> 4
	progAL := payload[(2*numComponents)+3] & 15

	fmt.Printf("#                                    num_components      %d\n", numComponents)
	fmt.Printf("#                                    c_selectors         % 02X\n", cTable[:numComponents])
	fmt.Printf("#                                    h_selectors D|A     % 02X\n", hTable[:numComponents])
	fmt.Printf("#                                    progression         s: %2d, %2d; a: %2d, %2d\n", progSS, progSE, progAH, progAL)
}

func parseAdobe(marker byte, payload []byte) {
	if len(payload) < 12 {
		return
	}
	transform := "RGB or CMYK"
	switch payload[11] {
	case 1:
		transform = "YCC"
	case 2:
		transform = "YCCK"
	}
	fmt.Printf("#                                    transform           %s\n", transform)
}

func parseExif(marker byte, payload []byte) {
	// Not implemented.
}

func parseExtendedXMP(marker byte, payload []byte) {
	// Not implemented.
}

func parseICC(marker byte, payload []byte) {
	// Not implemented.
}

func parseJFIF(marker byte, payload []byte) {
	// Not implemented.
}

func parsePhotoshop(marker byte, payload []byte) {
	// Not implemented.
}

func parseXMP(marker byte, payload []byte) {
	// Not implemented.
}

func hasPrefix(s []byte, prefix string) bool {
	return (len(s) >= len(prefix)) &&
		(string(s[:len(prefix)]) == prefix)
}

var names = [256]string{
	0xC0: "SOF0 (Sequential/Baseline)",
	0xC1: "SOF1 (Sequential/Extended)",
	0xC2: "SOF2 (Progressive)",
	0xC3: "SOF3",
	0xC4: "DHT",
	0xC5: "SOF5",
	0xC6: "SOF6",
	0xC7: "SOF7",

	0xC8: "JPG",
	0xC9: "SOF9",
	0xCA: "SOF10",
	0xCB: "SOF11",
	0xCC: "DAC",
	0xCD: "SOF13",
	0xCE: "SOF14",
	0xCF: "SOF15",

	0xD0: "RST0",
	0xD1: "RST1",
	0xD2: "RST2",
	0xD3: "RST3",
	0xD4: "RST4",
	0xD5: "RST5",
	0xD6: "RST6",
	0xD7: "RST7",

	0xD8: "SOI",
	0xD9: "EOI",
	0xDA: "SOS",
	0xDB: "DQT",
	0xDC: "DNL",
	0xDD: "DRI",
	0xDE: "DHP",
	0xDF: "EXP",

	0xE0: "APP0",
	0xE1: "APP1",
	0xE2: "APP2",
	0xE3: "APP3",
	0xE4: "APP4",
	0xE5: "APP5",
	0xE6: "APP6",
	0xE7: "APP7",

	0xE8: "APP8",
	0xE9: "APP9",
	0xEA: "APP10",
	0xEB: "APP11",
	0xEC: "APP12",
	0xED: "APP13",
	0xEE: "APP14",
	0xEF: "APP15",

	0xFE: "COM",
}
