// Copyright 2026 The Wuffs Authors.
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

// convert-nie-to-png.go decodes NIA from stdin and encodes (animated) PNG to
// stdout.
//
// Usage: go run convert-nia-to-apng.go < foo.nia > foo.png

import (
	"bytes"
	"errors"
	"io"
	"os"

	"github.com/google/wuffs/lib/uncompng"
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	// When writing APNG, we need to know (at the start of the file) the
	// animation's number of frames, but when reading NIA, we only know that at
	// the end of the file.
	//
	// We therefore use io.ReadAll, slurping the entire NIA data into memory
	// instead of using the io.Reader streaming API directly, so that we can
	// make two passes over the NIA data.
	niaBytes, err := io.ReadAll(os.Stdin)
	if err != nil {
		return err
	} else if len(niaBytes) < 24 {
		return errors.New("input is not in the NIA format")
	}

	premultiplied := niaBytes[6] == 'p'
	depth8 := niaBytes[7] == '8'
	width := u32le(niaBytes[8:])
	height := u32le(niaBytes[12:])
	padded := !depth8 && ((width & 1) != 0) && ((height & 1) != 0)
	if (u32le(niaBytes[:]) != 0x41AF_C36E) || // 0x41AF_C36E is 'nïA'le.
		(niaBytes[4] != 0xFF) ||
		(niaBytes[5] != 'b') ||
		((niaBytes[6] != 'n') && (niaBytes[6] != 'p')) ||
		((niaBytes[7] != '4') && (niaBytes[7] != '8')) ||
		(width >= 0x8000_0000) ||
		(height >= 0x8000_0000) {
		return errors.New("input is not in the NIA format")
	} else if (width > 0xFF_FFFF) || (height > 0xFF_FFFF) {
		return errors.New("input is in an unsupported NIA format")
	}

	nieSize := calculateNIESize(depth8, width, height)
	if nieSize < 0 {
		return errors.New("input image dimensions are unsupported (too large)")
	} else if nieSize < 16 {
		panic("unreachable")
	}

	// NIA's depth8 is measured in *bytes* per *pixel*, but uncompng's depth is
	// measured *bits* per *channel* (times four channels, for BGRA).
	depth := uncompng.Depth8
	if depth8 {
		depth = uncompng.Depth16
	}

	expectedNieHeader := append([]byte(nil), niaBytes[:16]...)
	expectedNieHeader[3] = 'E'

	numFrames := uint64(0)
	prevCDD := uint64(0)
	lastCDD := uint64(0)
	for buf := niaBytes[16:]; len(buf) >= 8; numFrames++ {
		cdd := u64le(buf)
		if (cdd >> 32) == 0x8000_0000 {
			lastCDD = cdd
			break
		} else if ((cdd >> 32) > 0x8000_0000) || (cdd < prevCDD) {
			return errors.New("bad CDD (Cumulative Display Duration)")
		}
		buf = buf[8:]
		if int64(len(buf)) < nieSize {
			return errors.New("bad NIE frame")
		} else if !bytes.Equal(buf[:16], expectedNieHeader) {
			return errors.New("bad NIE header")
		}
		buf = buf[nieSize:]

		if padded {
			if len(buf) < 4 {
				return errors.New("bad NIE padding")
			}
			buf = buf[4:]
		}
		prevCDD = cdd
	}

	enc := uncompng.AnimationEncoder{}
	if err := enc.EncodeHeader(
		os.Stdout, depth, uncompng.ColorTypeNRGBA,
		int(width), int(height), uint32(numFrames), uint32(lastCDD)); err != nil {
		return err
	}

	prevCDD = 0
	for buf := niaBytes[16:]; len(buf) >= 8; {
		cdd := u64le(buf)
		if (cdd >> 32) >= 0x8000_0000 {
			break
		}
		buf = buf[8:]

		if depth8 {
			swapBlueRedEndian8(buf[16:nieSize])
			if premultiplied {
				unpremultiply8(buf[16:nieSize])
			}
		} else {
			swapBlueRedEndian4(buf[16:nieSize])
			if premultiplied {
				unpremultiply4(buf[16:nieSize])
			}
		}

		numer, denom := calculateDurationRatio(cdd - prevCDD)
		if err := enc.EncodeFrame(
			os.Stdout, buf[16:nieSize], 4*int(width), numer, denom); err != nil {
			return err
		}

		buf = buf[nieSize:]

		if padded {
			buf = buf[4:]
		}
		prevCDD = cdd
	}

	return nil
}

// swapBlueRedEndian4 corrects for the fact that NIA uses little-endian BGRA
// order but the *Go* lib/uncompng library (unlike the *C* snippet/uncompng.c)
// uses big-endian RGBA order, the same as the Go standard library (and PNG).
func swapBlueRedEndian4(buf []byte) {
	for ; len(buf) >= 4; buf = buf[4:] {
		buf[0], buf[2] = buf[2], buf[0]
	}
}

func swapBlueRedEndian8(buf []byte) {
	for ; len(buf) >= 8; buf = buf[8:] {
		buf[0], buf[1], buf[4], buf[5] = buf[5], buf[4], buf[1], buf[0]
		buf[2], buf[3], buf[6], buf[7] = buf[3], buf[2], buf[7], buf[6]
	}
}

func unpremultiply4(buf []byte) {
	for ; len(buf) >= 4; buf = buf[4:] {
		r := 0x101 * uint32(buf[0])
		g := 0x101 * uint32(buf[1])
		b := 0x101 * uint32(buf[2])
		a := 0x101 * uint32(buf[3])
		if (a == 0xFFFF) || (a == 0x0000) {
			continue
		}
		r = (r * 0xFFFF) / a
		g = (g * 0xFFFF) / a
		b = (b * 0xFFFF) / a
		buf[0] = byte(r >> 8)
		buf[1] = byte(g >> 8)
		buf[2] = byte(b >> 8)
	}
}

func unpremultiply8(buf []byte) {
	for ; len(buf) >= 4; buf = buf[4:] {
		r := (uint32(buf[0]) << 8) | uint32(buf[1])
		g := (uint32(buf[2]) << 8) | uint32(buf[3])
		b := (uint32(buf[4]) << 8) | uint32(buf[5])
		a := (uint32(buf[6]) << 8) | uint32(buf[7])
		if (a == 0xFFFF) || (a == 0x0000) {
			continue
		}
		r = (r * 0xFFFF) / a
		g = (g * 0xFFFF) / a
		b = (b * 0xFFFF) / a
		buf[0] = byte(r >> 8)
		buf[1] = byte(r >> 0)
		buf[2] = byte(g >> 8)
		buf[3] = byte(g >> 0)
		buf[4] = byte(b >> 8)
		buf[5] = byte(b >> 0)
	}
}

// calculateDurationRatio converts from a time duration measured in flicks
// (frame-ticks, 1 / 705600000 of a second, to APNG's representation, which is
// a ratio of two uint16 values.
//
// TODO: be smarter about how to express a durationInFlicks that isn't an
// integer number of milliseconds (or is over 65.535 seconds). For now, just
// hard-code the denominator to 1000, meaning milliseconds, and round to
// nearest (capped at 65535) to get the numerator.
func calculateDurationRatio(durationInFlicks uint64) (numerator uint16, denominator uint16) {
	const flicksPerMillisecond = 705600
	millis := (durationInFlicks + (flicksPerMillisecond / 2)) / flicksPerMillisecond
	return uint16(min(0xFFFF, millis)), 1000
}

func calculateNIESize(depth8 bool, width uint32, height uint32) int64 {
	const maxInt64 = (1 << 63) - 1
	const maxUint64 = (1 << 64) - 1

	n := uint64(width) * uint64(height) * 4
	if depth8 {
		if n > (maxUint64 / 2) {
			return -1
		}
		n *= 2
	}
	if n > (maxInt64 - 16) {
		return -1
	}
	return int64(n + 16)
}

func u32le(b []byte) uint32 {
	return (uint32(b[0]) << 0) |
		(uint32(b[1]) << 8) |
		(uint32(b[2]) << 16) |
		(uint32(b[3]) << 24)
}

func u64le(b []byte) uint64 {
	return (uint64(b[0]) << 0) |
		(uint64(b[1]) << 8) |
		(uint64(b[2]) << 16) |
		(uint64(b[3]) << 24) |
		(uint64(b[4]) << 32) |
		(uint64(b[5]) << 40) |
		(uint64(b[6]) << 48) |
		(uint64(b[7]) << 56)
}
