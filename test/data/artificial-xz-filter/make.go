// Copyright 2024 The Wuffs Authors.
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

// Usage: go run make.go

import (
	"fmt"
	"hash/crc32"
	"os"
	"os/exec"
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	recipes := []struct {
		filterID   string
		filterName string
		generator  func() []byte
	}{
		{"07", "arm", genArm},
		{"08", "armthumb", genArmthumb},
		{"09", "sparc", genSparc},
	}

	for _, recipe := range recipes {
		contents := recipe.generator()
		filename := fmt.Sprintf("xz-filter-%s-%08x-%s.dat",
			recipe.filterID,
			crc32.ChecksumIEEE(contents),
			recipe.filterName)
		if err := os.WriteFile(filename, contents, 0644); err != nil {
			return err
		}
		fmt.Printf("Wrote %s\n", filename)

		cmd := exec.Command("xz", "--keep", "--force", "--"+recipe.filterName, "--lzma2", filename)
		cmd.Stderr = os.Stderr
		if err := cmd.Run(); err != nil {
			return err
		}
		fmt.Printf("Wrote %s.xz\n", filename)
	}
	return nil
}

// pi's digits are a source of random-looking numbers. len(pi) is 128.
const pi = "3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844"

// The artificial data generated below is designed to tickle XZ's BCJ (Branch,
// Call, Jump) filters. See xz-embedded's linux/lib/xz/xz_dec_bcj.c for C code
// implementations.

func genArm() []byte {
	b := make([]byte, 256)
	for i := range b {
		b[i] = byte(i)
		if ((i & 3) == 3) &&
			((pi[i>>2] & 1) != 0) {
			b[i] = 0xEB
		}
	}
	return b
}

func genArmthumb() []byte {
	b := make([]byte, 256)
	for i := range b {
		b[i] = byte(i)
	}
	for i := 0; (i + 4) <= len(b); {
		x := peekU32LE(b[i:])
		switch pi[i>>2] & 1 {
		case 0:
			i += 2
		case 1:
			x = (x & 0x07FF_07FF) | 0xF800_F000
			pokeU32LE(b[i:], x)
			i += 4
		}
	}
	return b
}

func genSparc() []byte {
	b := make([]byte, 256)
	for i := range b {
		b[i] = byte(i)
	}
	for i := 0; (i + 4) <= len(b); i += 4 {
		x := peekU32BE(b[i:])
		switch pi[i>>2] & 3 {
		case 0:
			x = (x & 0x003F_FFFF) | 0x4000_0000
		case 1:
			x = (x & 0x003F_FFFF) | 0x7FC0_0000
		default:
			// No-op.
		}
		pokeU32BE(b[i:], x)
	}
	return b
}

func peekU32BE(b []byte) uint32 {
	return (uint32(b[0]) << 0x18) |
		(uint32(b[1]) << 0x10) |
		(uint32(b[2]) << 0x08) |
		(uint32(b[3]) << 0x00)
}

func peekU32LE(b []byte) uint32 {
	return (uint32(b[0]) << 0x00) |
		(uint32(b[1]) << 0x08) |
		(uint32(b[2]) << 0x10) |
		(uint32(b[3]) << 0x18)
}

func pokeU32BE(b []byte, x uint32) {
	b[0] = uint8(x >> 0x18)
	b[1] = uint8(x >> 0x10)
	b[2] = uint8(x >> 0x08)
	b[3] = uint8(x >> 0x00)
}

func pokeU32LE(b []byte, x uint32) {
	b[0] = uint8(x >> 0x00)
	b[1] = uint8(x >> 0x08)
	b[2] = uint8(x >> 0x10)
	b[3] = uint8(x >> 0x18)
}
