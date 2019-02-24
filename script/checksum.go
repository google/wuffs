// Copyright 2017 The Wuffs Authors.
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

// checksum.go prints a checksum of stdin's bytes, or of the opening digits of
// π. Checksum algorithms include "adler32" and "crc32/ieee".
//
// Usage: go run checksum.go -algorithm=crc32/ieee < foo.bar

import (
	"flag"
	"fmt"
	"hash"
	"hash/adler32"
	"hash/crc32"
	"io"
	"os"
	"strings"
)

var (
	algorithm = flag.String("algorithm", "adler32", "checksum algorithm")
	pi        = flag.Bool("pi", false, "checksum the digits of pi instead of stdin")
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Parse()

	if *pi {
		const digits = "3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170"
		if len(digits) != 99 {
			panic("bad len(digits)")
		}
		for i := 0; i < 100; i++ {
			if err := do(strings.NewReader(digits[:i])); err != nil {
				return err
			}
			fmt.Print(",")
			if i&7 == 7 {
				fmt.Println()
			}
		}
	} else {
		if err := do(os.Stdin); err != nil {
			return err
		}
		fmt.Println()
	}
	return nil
}

func do(r io.Reader) error {
	h := io.Writer(nil)
	switch *algorithm {
	case "adler32":
		h = adler32.New()
	case "crc32/ieee":
		h = crc32.NewIEEE()
	default:
		return fmt.Errorf("unknown algorithm %q", *algorithm)
	}

	if _, err := io.Copy(h, r); err != nil {
		return err
	}

	switch h := h.(type) {
	case hash.Hash32:
		fmt.Printf("0x%08X", h.Sum32())
	case hash.Hash64:
		fmt.Printf("0x%016X", h.Sum64())
	default:
		return fmt.Errorf("algorithm %q is not a Hash32 or Hash64", *algorithm)
	}
	return nil
}
