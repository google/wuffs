// Copyright 2017 The Puffs Authors.
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

// adler32sum.go prints the Adler-32 checksum of stdin's bytes.
//
// Usage: go run adler32sum.go < foo.bar

import (
	"fmt"
	"hash/adler32"
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
	a := adler32.New()
	if _, err := io.Copy(a, os.Stdin); err != nil {
		return err
	}
	fmt.Printf("0x%08X\n", a.Sum32())
	return nil
}
