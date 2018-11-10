// Copyright 2018 The Wuffs Authors.
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

// +build ignore

package main

// print-crc32-magic-numbers.go prints the std/crc32 magic number tables.
//
// Usage: go run print-crc32-magic-numbers.go

import (
	"fmt"
	"hash/crc32"
	"os"
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	tables := [16]crc32.Table{}
	tables[0] = *crc32.MakeTable(crc32.IEEE)

	// See "Multi-Byte Lookup Tables" in std/crc32/README.md for more detail on
	// the slicing-by-M algorithm. We use an M of 16.
	for i := 0; i < 256; i++ {
		crc := tables[0][i]
		for j := 1; j < 16; j++ {
			crc = tables[0][crc&0xFF] ^ (crc >> 8)
			tables[j][i] = crc
		}
	}

	for i, t := range tables {
		if i != 0 {
			fmt.Println("],[")
		}
		for j, x := range t {
			fmt.Printf("0x%08X,", x)
			if j&7 == 7 {
				fmt.Println()
			}
		}
	}
	return nil
}
