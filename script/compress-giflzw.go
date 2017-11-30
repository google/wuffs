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

// compress-giflzw.go applies GIF's LZW-compression. The initial byte in each
// written file is the LZW literal width.
//
// Usage: go run compress-giflzw.go < pi.txt > pi.txt.giflzw

import (
	"compress/lzw"
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
	os.Stdout.WriteString("\x08")
	w := lzw.NewWriter(os.Stdout, lzw.LSB, 8)
	if _, err := io.Copy(w, os.Stdin); err != nil {
		w.Close()
		return err
	}
	return w.Close()
}
