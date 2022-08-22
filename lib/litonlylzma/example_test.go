// Copyright 2022 The Wuffs Authors.
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

package litonlylzma_test

import (
	"fmt"
	"log"

	"github.com/google/wuffs/lib/litonlylzma"
)

func ExampleRoundTripLZMA() {
	original := []byte("Hello world.\n")
	compressed, err := litonlylzma.FileFormatLZMA.Encode(nil, original)
	if err != nil {
		log.Fatalf("Encode: %v", err)
	}
	recovered, _, err := litonlylzma.FileFormatLZMA.Decode(nil, compressed)
	if err != nil {
		log.Fatalf("Decode: %v", err)
	}

	for i, c := range compressed {
		if (i & 15) > 0 {
			fmt.Printf(" ")
		}
		fmt.Printf("%02X", c)
		if ((i & 15) == 15) || ((i + 1) == len(compressed)) {
			fmt.Println()
		}
	}
	fmt.Printf("%s", recovered)

	// Output:
	// 5D 00 10 00 00 0D 00 00 00 00 00 00 00 00 24 19
	// 49 86 E7 D5 E5 E2 56 ED 6A E6 0E 81 FE B8 00 00
	// Hello world.
}

func ExampleRoundTripXz() {
	original := []byte("Hello world.\n")
	compressed, err := litonlylzma.FileFormatXz.Encode(nil, original)
	if err != nil {
		log.Fatalf("Encode: %v", err)
	}
	recovered, _, err := litonlylzma.FileFormatXz.Decode(nil, compressed)
	if err != nil {
		log.Fatalf("Decode: %v", err)
	}

	for i, c := range compressed {
		if (i & 15) > 0 {
			fmt.Printf(" ")
		}
		fmt.Printf("%02X", c)
		if ((i & 15) == 15) || ((i + 1) == len(compressed)) {
			fmt.Println()
		}
	}
	fmt.Printf("%s", recovered)

	// Output:
	// FD 37 7A 58 5A 00 00 01 69 22 DE 36 02 00 21 01
	// 00 00 00 00 37 27 97 D6 01 00 0C 48 65 6C 6C 6F
	// 20 77 6F 72 6C 64 2E 0A 00 00 00 00 8E F8 31 35
	// 00 01 21 0D 75 DC A8 D2 90 42 99 0D 01 00 00 00
	// 00 01 59 5A
	// Hello world.
}
