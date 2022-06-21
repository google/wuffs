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

func ExampleRoundTrip() {
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
		if i > 0 {
			fmt.Printf(" ")
		}
		fmt.Printf("%02X", c)
	}
	fmt.Printf("\n%s\n", recovered)

	// Output:
	// 5D 00 10 00 00 0D 00 00 00 00 00 00 00 00 24 19 49 86 E7 D5 E5 E2 56 ED 6A E6 0E 81 FE B8 00 00
	// Hello world.
}
