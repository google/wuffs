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

//go:build ignore
// +build ignore

package main

import (
	"bytes"
	"fmt"
	"os"
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	w := &bytes.Buffer{}
	w.WriteString(header)

	genUpsampleFrom(w)
	w.WriteString("\n")
	genHardCodedDHTSegments(w)
	w.WriteString("\n")
	genHuffmanBitWriters(w)

	return os.WriteFile("data.go", w.Bytes(), 0644)
}

func genUpsampleFrom(w *bytes.Buffer) {
	w.WriteString("// UpsampleFrom produces one 16×16 quad-block from one 8×8 block.\n")
	w.WriteString("//\n")
	w.WriteString("// It uses a triangle filter.\n")
	w.WriteString("func (dst *QuadBlockU8) UpsampleFrom(src *BlockU8) {\n")
	w.WriteString("\tif (dst == nil) || (src == nil) {\n\t\treturn\n\t}\n")
	for y := 0; y < 16; y++ {
		for x := 0; x < 16; x++ {
			d := (y << 4) | x

			s0 := ((y >> 1) << 3) | (x >> 1)
			sx := ((x & 1) * 2) - 1
			sy := ((y & 1) * 16) - 8

			// Handle corners.
			if ((x == 0x00) || (x == 0x0F)) && ((y == 0x00) || (y == 0x0F)) {
				fmt.Fprintf(w, "\tdst[0x%02X] = src[0x%02X]\n", d, s0)
				continue
			}

			// Handle top and bottom edges.
			if (y == 0x00) || (y == 0x0F) {
				fmt.Fprintf(w, "\tdst[0x%02X] = uint8(((3 * uint32(src[0x%02X])) + uint32(src[0x%02X]) + 2) / 4)\n", d, s0, s0+sx)
				continue
			}

			// Handle left and right edges.
			if (x == 0x00) || (x == 0x0F) {
				fmt.Fprintf(w, "\tdst[0x%02X] = uint8(((3 * uint32(src[0x%02X])) + uint32(src[0x%02X]) + 2) / 4)\n", d, s0, s0+sy)
				continue
			}

			fmt.Fprintf(w, "\tdst[0x%02X] = uint8(("+
				"(9 * uint32(src[0x%02X])) + "+
				"(3 * uint32(src[0x%02X])) + "+
				"(3 * uint32(src[0x%02X])) + "+
				"uint32(src[0x%02X]) + 8) / 16)\n", d, s0, s0+sx, s0+sy, s0+sx+sy)
		}
	}
	w.WriteString("}\n")
}

func fillInPlaceholderPayloadLength(payload []byte) {
	n := len(payload) - 2 // Minus 2 for the JPEG marker.
	payload[2] = uint8(n >> 8)
	payload[3] = uint8(n >> 0)
}

func genConstStringData(w *bytes.Buffer, contents []byte) {
	for {
		prefix := contents
		if len(prefix) > 16 {
			prefix, contents = contents[:16], contents[16:]
		} else {
			contents = nil
		}

		w.WriteString("\t\"")
		for _, v := range prefix {
			fmt.Fprintf(w, "\\x%02X", v)
		}
		w.WriteString("\"")

		if len(contents) == 0 {
			break
		}
		w.WriteString(" +\n")
	}
}

func genHardCodedDHTSegments(w *bytes.Buffer) {
	w.WriteString("const hardCodedDHTSegments = \"\" +\n")

	for i := 0; i < 2; i++ {
		contents := append([]byte(nil),
			0xFF, 0xC4, // DHT marker.
			0x00, 0x00, // Placeholder payload length.
		)

		if i == 0 {
			w.WriteString("\t// 212 bytes for the first DHT segment.\n")
		} else {
			w.WriteString(" +\n\t// 212 bytes for the second DHT segment.\n")
		}

		contents = append(contents, 0x00|uint8(i)) // 0x00 means DC.
		contents = append(contents, theHuffmanSpec[(2*i)+0].counts[:]...)
		contents = append(contents, theHuffmanSpec[(2*i)+0].values...)
		contents = append(contents, 0x10|uint8(i)) // 0x10 means AC.
		contents = append(contents, theHuffmanSpec[(2*i)+1].counts[:]...)
		contents = append(contents, theHuffmanSpec[(2*i)+1].values...)

		fillInPlaceholderPayloadLength(contents)
		genConstStringData(w, contents)
	}
	w.WriteString("\n")
}

func genHuffmanBitWriters(w *bytes.Buffer) {
	w.WriteString("// huffmanBitWriter is a Huffman code LUT (Look-Up Table) for encoding.\n")
	w.WriteString("//\n")
	w.WriteString("// huffmanBitWriter[i] is the Huffman encoding for the value i: two uint16\n")
	w.WriteString("// halves packed as a uint32. The low half holds the bitstring bits. The high\n")
	w.WriteString("// half holds the bitstring length. Bitstrings are also known as code words.\n")
	w.WriteString("//\n")
	w.WriteString("// For example, huffmanBitWriters[0][5] being 0x00030006 means the Luma DC\n")
	w.WriteString("// value 5 has the bitstring \"110\".\n")
	w.WriteString("type huffmanBitWriter [256]uint32\n\n")

	w.WriteString("var huffmanBitWriters = [4]huffmanBitWriter{{\n")

	for i := range theHuffmanSpec {
		if i != 0 {
			w.WriteString("}, {\n")
		}

		hbw := huffmanBitWriter{}
		hbw.initialize(&theHuffmanSpec[i])

		for i, v := range hbw {
			if (i & 7) == 0 {
				w.WriteString("\t")
			}
			fmt.Fprintf(w, "0x%08X,", v)
			if (i & 7) == 7 {
				w.WriteString("\n")
			} else {
				w.WriteString(" ")
			}
		}
	}

	w.WriteString("}}\n")
}

// huffmanSpec specifies a Huffman encoding.
type huffmanSpec struct {
	// counts[i] is the number of code words of length i+1 bits.
	counts [16]byte
	// values[i] is the decoded value of the i'th code word.
	values []byte
}

// theHuffmanSpec corresponds to Section K.3 "Typical Huffman tables for 8-bit
// precision luminance and chrominance" of the JPEG spec.
//
// Each Huffman code is deliberately incomplete. No code word (also known as a
// bitstring) is all-1-bits, since that would interact poorly with JPEG's byte
// stuffing.
//
// See the JPEG spec (https://www.w3.org/Graphics/JPEG/itu-t81.pdf) Section C:
// "the codes shall be generated such that the all-1-bits code word of any
// length is reserved".
var theHuffmanSpec = [4]huffmanSpec{{
	// Luma DC. 12 elements.
	//
	// See the JPEG spec, Table K.3.
	//
	// value : bitstring : len(bitstring)
	// 0x00  : 00        : 2
	// 0x01  : 010       : 3
	// 0x02  : 011       : 3
	// 0x03  : 100       : 3
	// 0x04  : 101       : 3
	// 0x05  : 110       : 3
	// 0x06  : 1110      : 4
	// 0x07  : 11110     : 5
	// 0x08  : 111110    : 6
	// 0x09  : 1111110   : 7
	// 0x0A  : 11111110  : 8
	// 0x0B  : 111111110 : 9
	[16]byte{0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
	[]byte{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
}, {

	// Luma AC. 162 elements.
	//
	// See the JPEG spec, Table K.5.
	//
	// value : bitstring : len(bitstring)
	// 0x01  : 00        : 2
	// 0x02  : 01        : 2
	// 0x03  : 100       : 3
	// 0x00  : 1010      : 4
	// 0x04  : 1011      : 4
	// 0x11  : 1100      : 4
	// 0x05  : 11010     : 5
	// etc   : etc       : etc
	[16]byte{0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125},
	[]byte{
		0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
		0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
		0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
		0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
		0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
		0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
		0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
		0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
		0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
		0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
		0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
		0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
		0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
		0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
		0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
		0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
		0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
		0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
		0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
		0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
		0xF9, 0xFA,
	},
}, {

	// Chroma DC. 12 elements.
	//
	// See the JPEG spec, Table K.4.
	[16]byte{0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
	[]byte{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
}, {

	// Chroma AC. 162 elements.
	//
	// See the JPEG spec, Table K.6.
	[16]byte{0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119},
	[]byte{
		0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
		0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
		0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
		0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
		0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34,
		0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
		0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38,
		0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
		0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
		0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
		0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
		0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
		0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96,
		0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
		0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4,
		0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
		0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
		0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
		0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9,
		0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
		0xF9, 0xFA,
	},
}}

// huffmanBitWriter is a Huffman code LUT (Look-Up Table) for encoding.
//
// huffmanBitWriter[i] is the Huffman encoding for the value i: two uint16
// halves packed as a uint32. The low half holds the bitstring bits. The high
// half holds the bitstring length. Bitstrings are also known as code words.
type huffmanBitWriter [256]uint32

func (b *huffmanBitWriter) initialize(s *huffmanSpec) {
	bits, k := uint32(0), 0
	for i, count := range s.counts {
		length16 := uint32(i+1) << 16
		for j := uint8(0); j < count; j++ {
			if bits > 0xFFFF {
				panic("bits overflowed")
			}
			b[s.values[k]] = length16 | bits
			bits++
			k++
		}
		bits <<= 1
	}
}

const header = `// Code generated by running "go generate". DO NOT EDIT.

// Copyright 2025 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

package lowleveljpeg

`
