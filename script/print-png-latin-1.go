// Copyright 2021 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//go:build ignore
// +build ignore

package main

// print-png-latin-1.go prints the UTF-8 encoding of the std/png Latin-1 table.
//
// Usage: go run print-png-latin-1.go

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
	// The PNG spec (https://www.w3.org/TR/PNG/) says "*printable* [emphasis
	// added] Latin-1 characters and spaces (only character codes 32-126 and
	// 161-255 decimal are allowed)".
	//
	// See also https://www.w3.org/TR/2003/REC-PNG-20031110/iso_8859-1.txt
	for r := rune(0); r <= 0xFF; r++ {
		if (r < 32) || ((126 < r) && (r < 161)) {
			fmt.Printf(" 0x0000,")
		} else if r < 128 {
			fmt.Printf(" 0x%04X,", r)
		} else {
			s := fmt.Sprintf("%c", r)
			fmt.Printf(" 0x%02X%02X,", s[1], s[0]) // UTF-8 as little-endian uint16.
		}

		if r%8 == 7 {
			fmt.Println()
		}
	}
	return nil
}
