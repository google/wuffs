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

// checksum.go prints a checksum of stdin's bytes. Checksum algorithms include
// "adler32" and "crc32/ieee".
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
)

var (
	algorithm = flag.String("algorithm", "adler32", "checksum algorithm")
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	flag.Parse()

	h32 := hash.Hash32(nil)
	switch *algorithm {
	case "adler32":
		h32 = adler32.New()
	case "crc32/ieee":
		h32 = crc32.NewIEEE()
	default:
		return fmt.Errorf("unknown algorithm %q", *algorithm)
	}

	if h32 != nil {
		if _, err := io.Copy(h32, os.Stdin); err != nil {
			return err
		}
		fmt.Printf("0x%08X\n", h32.Sum32())
	}
	return nil
}
