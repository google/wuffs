// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// +build ignore

package main

// print-flate-magic-numbers.go prints the std/flate lcode_magic_numbers values
// based on the tables in RFC 1951 secion 3.2.5.
//
// Usage: go run print-flate-magic-numbers.go

import (
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
	for i := 0; i < 2; i++ {
		if i != 0 {
			fmt.Println()
		}
		for j := 0; j < 32; j++ {
			x := uint32(0x08000000)
			if baseNumbers[i][j] != bad {
				x = 0x40000000 | (baseNumbers[i][j] << 8) | (extraBits[i][j] << 4)
			}
			fmt.Printf("0x%08X,", x)
			if j&7 == 7 {
				fmt.Println()
			}
		}
	}
	return nil
}

const bad = 0xFFFFFFFF

var (
	baseNumbers = [2][32]uint32{{
		3, 4, 5, 6, 7, 8, 9, 10,
		11, 13, 15, 17, 19, 23, 27, 31,
		35, 43, 51, 59, 67, 83, 99, 115,
		131, 163, 195, 227, 258, bad, bad, bad,
	}, {
		1, 2, 3, 4, 5, 7, 9, 13,
		17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073,
		4097, 6145, 8193, 12289, 16385, 24577, bad, bad,
	}}

	extraBits = [2][32]uint32{{
		0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 2, 2, 2, 2,
		3, 3, 3, 3, 4, 4, 4, 4,
		5, 5, 5, 5, 0, bad, bad, bad,
	}, {
		0, 0, 0, 0, 1, 1, 2, 2,
		3, 3, 4, 4, 5, 5, 6, 6,
		7, 7, 8, 8, 9, 9, 10, 10,
		11, 11, 12, 12, 13, 13, bad, bad,
	}}
)
