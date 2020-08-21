// Copyright 2020 The Wuffs Authors.
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

// print-json-ascii-escapes.go prints the JSON escapes for ASCII characters.
// For example, the JSON escapes for the ASCII "horizontal tab" and
// "substitute" control characters are "\t" and "\u001A".
//
// Usage: go run print-json-ascii-escapes.go

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
	for c := 0; c < 0x80; c++ {
		s := ""
		switch c {
		case '\b':
			s = "\\b"
		case '\f':
			s = "\\f"
		case '\n':
			s = "\\n"
		case '\r':
			s = "\\r"
		case '\t':
			s = "\\t"
		case '"':
			s = "\\\""
		case '\\':
			s = "\\\\"
		default:
			if c > 0x20 {
				s = string(c)
			} else {
				const hex = "0123456789ABCDEF"
				s = string([]byte{
					'\\',
					'u',
					'0',
					'0',
					hex[c>>4],
					hex[c&0x0F],
				})
			}
		}
		b := make([]byte, 8)
		b[0] = byte(len(s))
		copy(b[1:], s)
		description := s
		if c == 0x7F {
			description = "<DEL>"
		}
		for _, x := range b {
			fmt.Printf("0x%02X, ", x)
		}
		fmt.Printf("// 0x%02X: %q\n", c, description)
	}
	return nil
}
