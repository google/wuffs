// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// +build ignore

package main

// print-bits.go prints stdin's bytes in hexadecimal and binary.
//
// Usage: go run print-bits.go < foo.bin

import (
	"fmt"
	"io"
	"os"
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	in := make([]byte, 4096)
	bits := []byte("...._....\n")
	os.Stdout.WriteString("offset  ASCII   hex     binary\n")
	for iBase := 0; ; {
		n, err := os.Stdin.Read(in)
		for i, x := range in[:n] {
			bits[0] = bit(x >> 7)
			bits[1] = bit(x >> 6)
			bits[2] = bit(x >> 5)
			bits[3] = bit(x >> 4)
			bits[5] = bit(x >> 3)
			bits[6] = bit(x >> 2)
			bits[7] = bit(x >> 1)
			bits[8] = bit(x >> 0)
			ascii := rune(x)
			if (x < 0x20) || (0x7F <= x) {
				ascii = '.'
			}
			fmt.Printf("%06d  %c       0x%02X    0b_%s", iBase+i, ascii, x, bits)
		}
		if err == io.EOF {
			return nil
		}
		if err != nil {
			return err
		}
		iBase += n
	}
}

func bit(x byte) byte {
	if x&1 != 0 {
		return '1'
	}
	return '0'
}
