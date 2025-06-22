// Copyright 2024 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

// ----------------

// Package uncompng encodes pixel data as not-compressed PNG-formatted bytes.
// The outputs are valid PNG files but avoids any compression techniques built
// into the PNG format. This obviously results in larger files than what the
// standard library's image/png package produces. Most programmers should use
// the standard package instead.
//
// The main purpose of this alternative PNG-encoding package is its simplicity
// of implementation, including all of its transitive dependencies. Its Go code
// can easily be studied (if you want to learn about the PNG file format),
// tested (the test code can depend on all of the standard library, including
// its image/png package), customized or ported to other languages. Unlike the
// standard package, this package has only two dependencies (the errors.New
// function and the io.Writer interface) and both are trivial.
//
// A secondary purpose is that, when starting with a slice of pixel data,
// producing an uncompressed PNG is faster than producing a compressed one - an
// extreme example of the general trade-off between compression speed and
// compression ratio. Similarly, decoding an uncompressed PNG can also be
// faster, even if the uncompressed PNG is bigger (in terms of byte size). If
// using PNG as a commonly-spoken format between two processes (connected by a
// Unix-like pipe) or between two libraries within the same process, and the
// transient PNG data never hits a disk drive or bandwidth-limited network, it
// may be better (faster) overall to skip any compression and decompression on
// either side of the communication channel.
//
// The Encoder.Encode method also makes no further memory allocations (above
// whatever its given io.Writer makes, if any). It is a lower-level API. See
// the encodeImage function in this package's test code for a higher-level API
// (with more dependencies and without the no-allocation guarantee) that takes
// an image.Image argument.
package uncompng

import (
	"errors"
	"io"
)

// ColorType states how to interpret the []byte pixel data as colors.
type ColorType byte

const (
	// ColorTypeGray means 1 byte per pixel (or 2 for Depth16, big-endian).
	//
	// This matches Go's image.Gray.Pix (or Gray16, for Depth16) layout.
	ColorTypeGray = ColorType(1)

	// ColorTypeRGBX means 4 bytes per pixel (or 8 for Depth16, big-endian).
	// Red, Green, Blue and the 4th channel is ignored.
	//
	// This matches Go's image.RGBA.Pix and image.NRGBA.Pix layouts (or RGBA64
	// or NRGBA64, for Depth16), provided that all of the pixels' alpha values
	// are 0xFF (or 0xFFFF, for Depth16).
	ColorTypeRGBX = ColorType(2)

	// ColorTypeRGBX means 4 bytes per pixel (or 8 for Depth16, big-endian).
	// Red, Green, Blue and Alpha. RGB uses non-premultiplied alpha.
	//
	// This matches Go's image.NRGBA.Pix layout (or NRGBA64, for Depth16). If
	// all of the pixels' alpha values are 0xFF (or 0xFFFF, for Depth16) then
	// either ColorTypeRGBX or ColorTypeNRGBA will produce the same PNG output
	// (in terms of pixels) but smaller (ColorTypeRGBX) or larger
	// (ColorTypeNRGBA) output in terms of byte count.
	ColorTypeNRGBA = ColorType(3)
)

func (c ColorType) pngFileFormatEncoding() byte {
	// https://www.w3.org/TR/2003/REC-PNG-20031110/#6Colour-values
	switch c {
	case ColorTypeGray:
		return 0
	case ColorTypeRGBX:
		return 2
	case ColorTypeNRGBA:
		return 6
	}
	return 0xFF
}

// Depth is the number of bits per channel.
type Depth byte

const (
	// Depth8 means one byte per pixel.
	Depth8 = Depth(8)

	// Depth16 means two bytes per pixel. Values are big-endian like the Go
	// standard library's Gray16, RGBA64 and NRGBA64 image types.
	Depth16 = Depth(16)
)

// Encoder is an opaque type that can convert a slice of pixel data to
// PNG-formatted bytes.
//
// It contains all of the buffers needed for the Encode method. Once the
// Encoder itself is allocated, Encoder.Encode makes no further allocations
// (above whatever its given io.Writer makes, if any).
//
// It can be re-used (but it is not thread-safe). One Encoder can make multiple
// Encode calls, each call producing a complete, stand-alone PNG image.
type Encoder struct {
	// buf is the fixed-size buffer to accumulate PNG-formatted output.
	//
	// A minimal PNG file consists of:
	//  - An 8-byte magic signature.
	//  - An IHDR chunk.
	//  - One or more IDAT chunks.
	//  - An IEND chunk.
	//
	// Each chunk consists of:
	//  - A 4-byte chunk payload length.
	//  - A 4-byte chunk type.
	//  - The chunk payload.
	//  - A 4-byte CRC-32/IEEE checksum of the chunk type and payload.
	//
	// Concatenating the IDAT payloads produces a ZLIB-compressed data stream.
	// The ZLIB-decoded data consists of (((BPP * W) + 1) * H) bytes, where:
	//  - BPP is the number of bytes per pixel (e.g. 4 for RGBA).
	//  - W is the image width in pixels.
	//  - H is the image height in pixels.
	//
	// The +1 is because every row's pixel data is preceded by one byte for the
	// per-row PNG filter. This package hard-codes a zero byte (no filtering).
	//
	// For a normal PNG encoder, the ZLIB-encoded data is shorter than the
	// ZLIB-decoded data (as compression is the whole point of ZLIB). This PNG
	// encoder prioritizes simplicity of implementation over compression ratio,
	// so the ZLIB-encoded data is actually longer. The ZLIB-formatted stream
	// consists of:
	//  - A 2-byte header.
	//  - A raw DEFLATE stream.
	//  - A 4-byte Adler-32 checksum of the ZLIB-decoded data.
	//
	// A raw DEFLATE stream is just one or more DEFLATE blocks.
	//
	// The DEFLATE format allows for uncompressed and compressed blocks. This
	// encoder only emits uncompressed blocks. Each uncompressed block is:
	//  - A 1-byte header (0 means non-final, 1 means final).
	//  - A 2-byte u16le payload length.
	//  - That 2-byte u16le payload length again, xor'ed with 0xFFFF.
	//  - The uncompressed block payload (literal bytes).
	//
	// ----
	//
	// After that long preamble, here's the data layout of the buf field. Each
	// time a slice of buf is passed to an io.Writer, it contains exactly one
	// IDAT chunk. The final time will also contain an IEND chunk after the
	// IDAT chunk, unless it won't fit, in which case there will be one more
	// Write call with just the 12-byte IEND chunk (and no IDAT chunk).
	//
	// For all but the first Write, the 4-byte IDAT chunk length starts at
	// buf[0x0000:]. For the first Write, it will start at buf[0x0021:], since
	// it is preceded by the magic signature and the IHDR chunk.
	//
	// Either way, each IDAT chunk contains exactly one DEFLATE block (an
	// uncompressed block). The very first IDAT (in the very first Write) also
	// contains the 2-byte ZLIB header. The very last IDAT also contains the
	// 4-byte Adler-32 checksum.
	//
	// Each DEFLATE uncompressed block's payload spans e.buf[ei:ej], for two
	// indexes ei and ej, either explicit local variables or implicit state.
	// Writing pixel data to the Encoder increments ej, flushing to the
	// io.Writer beforehand (which resets ej = ei) if the data won't fit. ei is
	// 0x0030 before the first Write call and at 0x000D afterwards.
	//
	// "The data won't fit" if ej would end up greater than ejMax, where ejMax
	// is the size of the buffer (0x10000) minus 8 bytes that leaves enough
	// room for two trailing 4-byte checksums (Adler-32 and CRC-32/IEEE).
	//
	// The last 4 bytes are also repurposed to hold the in-progress Adler-32
	// checksum state, as these 4 bytes won't be overwritten unless emitting
	// the final IDAT chunk (containing the final DEFLATE block).
	buf [65536]byte
}

const (
	eiFirst = 0x0030
	eiLater = 0x000D
	ejMax   = 0xFFF8
)

// Encode writes the pixel data to w. It makes no allocations above whatever
// w.Write makes, if any.
//
// pix holds the pixel data, either 1 or 4 bytes per pixel (doubled for
// Depth16) depending on the colorType. width and height are measured in
// pixels. stride is measured in bytes. depth must be either Depth8 or Depth16.
func (e *Encoder) Encode(w io.Writer, pix []byte, width int, height int, stride int, depth Depth, colorType ColorType) error {
	if (width < 0) || (height < 0) ||
		((depth != Depth8) && (depth != Depth16)) ||
		(colorType.pngFileFormatEncoding() == 0xFF) {
		return errors.New("uncompng: invalid argument")
	} else if (width > 0xFFFFFF) || (height > 0xFFFFFF) {
		return errors.New("uncompng: unsupported image size")
	}

	e.init(width, height, depth, colorType)
	ej := eiFirst

	for y := 0; y < height; y++ {
		if (ej + 1) > ejMax {
			if err := e.flush(w, ej, false); err != nil {
				return err
			}
			ej = eiLater
		}
		e.buf[ej+0] = 0 // PNG 'none' filter.
		ej += 1

		row := pix[y*stride:]

		switch ColorType(depth) | colorType {
		case 0x08 | ColorTypeGray:
			row = row[:1*width]
			for x := 0; x < width; x++ {
				if (ej + 1) > ejMax {
					if err := e.flush(w, ej, false); err != nil {
						return err
					}
					ej = eiLater
				}
				e.buf[ej+0] = row[0]
				ej += 1
				row = row[1:]
			}

		case 0x08 | ColorTypeRGBX:
			row = row[:4*width]
			for x := 0; x < width; x++ {
				if (ej + 3) > ejMax {
					if err := e.flush(w, ej, false); err != nil {
						return err
					}
					ej = eiLater
				}
				e.buf[ej+0] = row[0]
				e.buf[ej+1] = row[1]
				e.buf[ej+2] = row[2]
				ej += 3
				row = row[4:]
			}

		case 0x08 | ColorTypeNRGBA:
			row = row[:4*width]
			for x := 0; x < width; x++ {
				if (ej + 4) > ejMax {
					if err := e.flush(w, ej, false); err != nil {
						return err
					}
					ej = eiLater
				}
				e.buf[ej+0] = row[0]
				e.buf[ej+1] = row[1]
				e.buf[ej+2] = row[2]
				e.buf[ej+3] = row[3]
				ej += 4
				row = row[4:]
			}

		case 0x10 | ColorTypeGray:
			row = row[:2*width]
			for x := 0; x < width; x++ {
				if (ej + 2) > ejMax {
					if err := e.flush(w, ej, false); err != nil {
						return err
					}
					ej = eiLater
				}
				e.buf[ej+0] = row[0]
				e.buf[ej+1] = row[1]
				ej += 2
				row = row[2:]
			}

		case 0x10 | ColorTypeRGBX:
			row = row[:8*width]
			for x := 0; x < width; x++ {
				if (ej + 6) > ejMax {
					if err := e.flush(w, ej, false); err != nil {
						return err
					}
					ej = eiLater
				}
				e.buf[ej+0] = row[0]
				e.buf[ej+1] = row[1]
				e.buf[ej+2] = row[2]
				e.buf[ej+3] = row[3]
				e.buf[ej+4] = row[4]
				e.buf[ej+5] = row[5]
				ej += 6
				row = row[8:]
			}

		case 0x10 | ColorTypeNRGBA:
			row = row[:8*width]
			for x := 0; x < width; x++ {
				if (ej + 8) > ejMax {
					if err := e.flush(w, ej, false); err != nil {
						return err
					}
					ej = eiLater
				}
				e.buf[ej+0] = row[0]
				e.buf[ej+1] = row[1]
				e.buf[ej+2] = row[2]
				e.buf[ej+3] = row[3]
				e.buf[ej+4] = row[4]
				e.buf[ej+5] = row[5]
				e.buf[ej+6] = row[6]
				e.buf[ej+7] = row[7]
				ej += 8
				row = row[8:]
			}
		}
	}

	return e.flush(w, ej, true)
}

func (e *Encoder) init(width int, height int, depth Depth, colorType ColorType) {
	// PNG magic signature.
	e.buf[0x0000] = 0x89
	e.buf[0x0001] = 'P'
	e.buf[0x0002] = 'N'
	e.buf[0x0003] = 'G'
	e.buf[0x0004] = 0x0D
	e.buf[0x0005] = 0x0A
	e.buf[0x0006] = 0x1A
	e.buf[0x0007] = 0x0A

	// IHDR chunk length.
	e.buf[0x0008] = 0
	e.buf[0x0009] = 0
	e.buf[0x000A] = 0
	e.buf[0x000B] = 0x0D
	// IHDR chunk type.
	e.buf[0x000C] = 'I'
	e.buf[0x000D] = 'H'
	e.buf[0x000E] = 'D'
	e.buf[0x000F] = 'R'
	// IHDR chunk payload.
	e.buf[0x0010] = byte(width >> 24)
	e.buf[0x0011] = byte(width >> 16)
	e.buf[0x0012] = byte(width >> 8)
	e.buf[0x0013] = byte(width >> 0)
	e.buf[0x0014] = byte(height >> 24)
	e.buf[0x0015] = byte(height >> 16)
	e.buf[0x0016] = byte(height >> 8)
	e.buf[0x0017] = byte(height >> 0)
	e.buf[0x0018] = byte(depth)
	e.buf[0x0019] = colorType.pngFileFormatEncoding()
	e.buf[0x001A] = 0 // Compression method.
	e.buf[0x001B] = 0 // Filter method.
	e.buf[0x001C] = 0 // Interlace method.
	// IHDR CRC-32/IEEE checksum.
	ihdrCRC32 := crc32IEEE(e.buf[0x000C:0x001D])
	e.buf[0x001D] = byte(ihdrCRC32 >> 24)
	e.buf[0x001E] = byte(ihdrCRC32 >> 16)
	e.buf[0x001F] = byte(ihdrCRC32 >> 8)
	e.buf[0x0020] = byte(ihdrCRC32 >> 0)

	// IDAT chunk length placeholder.
	e.buf[0x0021] = 0
	e.buf[0x0022] = 0
	e.buf[0x0023] = 0
	e.buf[0x0024] = 0
	// IDAT chunk type.
	e.buf[0x0025] = 'I'
	e.buf[0x0026] = 'D'
	e.buf[0x0027] = 'A'
	e.buf[0x0028] = 'T'
	// ZLIB header: CM=8, CINFO=7, FDICT=0, FLEVEL=0. See RFC 1950.
	e.buf[0x0029] = 0x78
	e.buf[0x002A] = 0x01
	// DEFLATE uncompressed block header, with 4-byte placeholder for length.
	e.buf[0x002B] = 0
	e.buf[0x002C] = 0
	e.buf[0x002D] = 0
	e.buf[0x002E] = 0
	e.buf[0x002F] = 0
	// eiFirst is therefore 0x0030.

	// AdlerB=0, Adler32A=1.
	e.buf[0xFFFC] = 0
	e.buf[0xFFFD] = 0
	e.buf[0xFFFE] = 0
	e.buf[0xFFFF] = 1
}

func (e *Encoder) updateAdler32(ei int, ej int) {
	b := (uint32(e.buf[0xFFFC]) << 8) | uint32(e.buf[0xFFFD])
	a := (uint32(e.buf[0xFFFE]) << 8) | uint32(e.buf[0xFFFF])
	for ei < ej {
		end := ei + 5552
		if end > ej {
			end = ej
		}
		for ; ei < end; ei++ {
			a += uint32(e.buf[ei])
			b += a
		}
		a %= 65521
		b %= 65521
	}
	e.buf[0xFFFC] = byte(b >> 8)
	e.buf[0xFFFD] = byte(b >> 0)
	e.buf[0xFFFE] = byte(a >> 8)
	e.buf[0xFFFF] = byte(a >> 0)
}

func (e *Encoder) flush(w io.Writer, ej int, final bool) error {
	// Update the IDAT chunk length placeholder.
	crc32Start, ei := 0, 0
	if e.buf[0x0004] == 0x0D { // First IDAT chunk.
		idatChunkLen := ej - 0x0029
		if final {
			idatChunkLen += 4
		}
		e.buf[0x0021] = byte(idatChunkLen >> 24)
		e.buf[0x0022] = byte(idatChunkLen >> 16)
		e.buf[0x0023] = byte(idatChunkLen >> 8)
		e.buf[0x0024] = byte(idatChunkLen >> 0)
		crc32Start, ei = 0x0025, eiFirst
	} else { // Later IDAT chunk.
		idatChunkLen := ej - 0x0008
		if final {
			idatChunkLen += 4
		}
		e.buf[0x0000] = byte(idatChunkLen >> 24)
		e.buf[0x0001] = byte(idatChunkLen >> 16)
		e.buf[0x0002] = byte(idatChunkLen >> 8)
		e.buf[0x0003] = byte(idatChunkLen >> 0)
		crc32Start, ei = 0x0004, eiLater
	}

	// Update the DEFLATE uncompressed block header placeholder.
	deflateBlockLen := ej - ei
	e.buf[ei-5] = btou8(final)
	e.buf[ei-4] = 0x00 ^ byte(deflateBlockLen>>0)
	e.buf[ei-3] = 0x00 ^ byte(deflateBlockLen>>8)
	e.buf[ei-2] = 0xFF ^ byte(deflateBlockLen>>0)
	e.buf[ei-1] = 0xFF ^ byte(deflateBlockLen>>8)

	// Update (and maybe write) the Adler-32 checksum.
	e.updateAdler32(ei, ej)
	if final {
		e.buf[ej+0] = e.buf[0xFFFC]
		e.buf[ej+1] = e.buf[0xFFFD]
		e.buf[ej+2] = e.buf[0xFFFE]
		e.buf[ej+3] = e.buf[0xFFFF]
		ej += 4
	}

	// Write the CRC-32/IEEE checksum.
	idatCRC32 := crc32IEEE(e.buf[crc32Start:ej])
	e.buf[ej+0] = byte(idatCRC32 >> 24)
	e.buf[ej+1] = byte(idatCRC32 >> 16)
	e.buf[ej+2] = byte(idatCRC32 >> 8)
	e.buf[ej+3] = byte(idatCRC32 >> 0)
	ej += 4

	if !final {
		if _, err := w.Write(e.buf[:ej]); err != nil {
			return err
		}
		// Write (or reserve space for) 0x000D bytes total:
		//  - 4 bytes for the IDAT chunk length placeholder.
		//  - 4 bytes for the IDAT chunk type.
		//  - 5 bytes for the DEFLATE uncompressed block header placeholder.
		//
		// eiLater is therefore 0x000D.
		e.buf[0x0004] = 'I'
		e.buf[0x0005] = 'D'
		e.buf[0x0006] = 'A'
		e.buf[0x0007] = 'T'
		return nil
	}

	const iendChunk = "\x00\x00\x00\x00IEND\xAE\x42\x60\x82"
	writeSeparateIENDChunk := (ej + len(iendChunk)) > len(e.buf)
	if !writeSeparateIENDChunk {
		copy(e.buf[ej:ej+len(iendChunk)], iendChunk)
		ej += len(iendChunk)
	}

	if _, err := w.Write(e.buf[:ej]); err != nil {
		return err
	}

	if writeSeparateIENDChunk {
		copy(e.buf[:], iendChunk)
		if _, err := w.Write(e.buf[:len(iendChunk)]); err != nil {
			return err
		}
	}

	return nil
}

func btou8(a bool) uint8 {
	if a {
		return 1
	}
	return 0
}

func crc32IEEE(b []byte) uint32 {
	hash := uint32(0xFFFF_FFFF)
	for _, v := range b {
		hash = crc32IEEETable[uint8(hash)^v] ^ (hash >> 8)
	}
	return hash ^ uint32(0xFFFF_FFFF)
}

var crc32IEEETable = [256]uint32{
	0x0000_0000, 0x7707_3096, 0xEE0E_612C, 0x9909_51BA, 0x076D_C419, 0x706A_F48F, 0xE963_A535, 0x9E64_95A3,
	0x0EDB_8832, 0x79DC_B8A4, 0xE0D5_E91E, 0x97D2_D988, 0x09B6_4C2B, 0x7EB1_7CBD, 0xE7B8_2D07, 0x90BF_1D91,
	0x1DB7_1064, 0x6AB0_20F2, 0xF3B9_7148, 0x84BE_41DE, 0x1ADA_D47D, 0x6DDD_E4EB, 0xF4D4_B551, 0x83D3_85C7,
	0x136C_9856, 0x646B_A8C0, 0xFD62_F97A, 0x8A65_C9EC, 0x1401_5C4F, 0x6306_6CD9, 0xFA0F_3D63, 0x8D08_0DF5,
	0x3B6E_20C8, 0x4C69_105E, 0xD560_41E4, 0xA267_7172, 0x3C03_E4D1, 0x4B04_D447, 0xD20D_85FD, 0xA50A_B56B,
	0x35B5_A8FA, 0x42B2_986C, 0xDBBB_C9D6, 0xACBC_F940, 0x32D8_6CE3, 0x45DF_5C75, 0xDCD6_0DCF, 0xABD1_3D59,
	0x26D9_30AC, 0x51DE_003A, 0xC8D7_5180, 0xBFD0_6116, 0x21B4_F4B5, 0x56B3_C423, 0xCFBA_9599, 0xB8BD_A50F,
	0x2802_B89E, 0x5F05_8808, 0xC60C_D9B2, 0xB10B_E924, 0x2F6F_7C87, 0x5868_4C11, 0xC161_1DAB, 0xB666_2D3D,
	0x76DC_4190, 0x01DB_7106, 0x98D2_20BC, 0xEFD5_102A, 0x71B1_8589, 0x06B6_B51F, 0x9FBF_E4A5, 0xE8B8_D433,
	0x7807_C9A2, 0x0F00_F934, 0x9609_A88E, 0xE10E_9818, 0x7F6A_0DBB, 0x086D_3D2D, 0x9164_6C97, 0xE663_5C01,
	0x6B6B_51F4, 0x1C6C_6162, 0x8565_30D8, 0xF262_004E, 0x6C06_95ED, 0x1B01_A57B, 0x8208_F4C1, 0xF50F_C457,
	0x65B0_D9C6, 0x12B7_E950, 0x8BBE_B8EA, 0xFCB9_887C, 0x62DD_1DDF, 0x15DA_2D49, 0x8CD3_7CF3, 0xFBD4_4C65,
	0x4DB2_6158, 0x3AB5_51CE, 0xA3BC_0074, 0xD4BB_30E2, 0x4ADF_A541, 0x3DD8_95D7, 0xA4D1_C46D, 0xD3D6_F4FB,
	0x4369_E96A, 0x346E_D9FC, 0xAD67_8846, 0xDA60_B8D0, 0x4404_2D73, 0x3303_1DE5, 0xAA0A_4C5F, 0xDD0D_7CC9,
	0x5005_713C, 0x2702_41AA, 0xBE0B_1010, 0xC90C_2086, 0x5768_B525, 0x206F_85B3, 0xB966_D409, 0xCE61_E49F,
	0x5EDE_F90E, 0x29D9_C998, 0xB0D0_9822, 0xC7D7_A8B4, 0x59B3_3D17, 0x2EB4_0D81, 0xB7BD_5C3B, 0xC0BA_6CAD,
	0xEDB8_8320, 0x9ABF_B3B6, 0x03B6_E20C, 0x74B1_D29A, 0xEAD5_4739, 0x9DD2_77AF, 0x04DB_2615, 0x73DC_1683,
	0xE363_0B12, 0x9464_3B84, 0x0D6D_6A3E, 0x7A6A_5AA8, 0xE40E_CF0B, 0x9309_FF9D, 0x0A00_AE27, 0x7D07_9EB1,
	0xF00F_9344, 0x8708_A3D2, 0x1E01_F268, 0x6906_C2FE, 0xF762_575D, 0x8065_67CB, 0x196C_3671, 0x6E6B_06E7,
	0xFED4_1B76, 0x89D3_2BE0, 0x10DA_7A5A, 0x67DD_4ACC, 0xF9B9_DF6F, 0x8EBE_EFF9, 0x17B7_BE43, 0x60B0_8ED5,
	0xD6D6_A3E8, 0xA1D1_937E, 0x38D8_C2C4, 0x4FDF_F252, 0xD1BB_67F1, 0xA6BC_5767, 0x3FB5_06DD, 0x48B2_364B,
	0xD80D_2BDA, 0xAF0A_1B4C, 0x3603_4AF6, 0x4104_7A60, 0xDF60_EFC3, 0xA867_DF55, 0x316E_8EEF, 0x4669_BE79,
	0xCB61_B38C, 0xBC66_831A, 0x256F_D2A0, 0x5268_E236, 0xCC0C_7795, 0xBB0B_4703, 0x2202_16B9, 0x5505_262F,
	0xC5BA_3BBE, 0xB2BD_0B28, 0x2BB4_5A92, 0x5CB3_6A04, 0xC2D7_FFA7, 0xB5D0_CF31, 0x2CD9_9E8B, 0x5BDE_AE1D,
	0x9B64_C2B0, 0xEC63_F226, 0x756A_A39C, 0x026D_930A, 0x9C09_06A9, 0xEB0E_363F, 0x7207_6785, 0x0500_5713,
	0x95BF_4A82, 0xE2B8_7A14, 0x7BB1_2BAE, 0x0CB6_1B38, 0x92D2_8E9B, 0xE5D5_BE0D, 0x7CDC_EFB7, 0x0BDB_DF21,
	0x86D3_D2D4, 0xF1D4_E242, 0x68DD_B3F8, 0x1FDA_836E, 0x81BE_16CD, 0xF6B9_265B, 0x6FB0_77E1, 0x18B7_4777,
	0x8808_5AE6, 0xFF0F_6A70, 0x6606_3BCA, 0x1101_0B5C, 0x8F65_9EFF, 0xF862_AE69, 0x616B_FFD3, 0x166C_CF45,
	0xA00A_E278, 0xD70D_D2EE, 0x4E04_8354, 0x3903_B3C2, 0xA767_2661, 0xD060_16F7, 0x4969_474D, 0x3E6E_77DB,
	0xAED1_6A4A, 0xD9D6_5ADC, 0x40DF_0B66, 0x37D8_3BF0, 0xA9BC_AE53, 0xDEBB_9EC5, 0x47B2_CF7F, 0x30B5_FFE9,
	0xBDBD_F21C, 0xCABA_C28A, 0x53B3_9330, 0x24B4_A3A6, 0xBAD0_3605, 0xCDD7_0693, 0x54DE_5729, 0x23D9_67BF,
	0xB366_7A2E, 0xC461_4AB8, 0x5D68_1B02, 0x2A6F_2B94, 0xB40B_BE37, 0xC30C_8EA1, 0x5A05_DF1B, 0x2D02_EF8D,
}
