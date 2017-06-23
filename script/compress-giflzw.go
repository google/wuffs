// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

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
