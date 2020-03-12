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

// base38-encode.go prints the base38 encoding of each argument.
//
// Usage: go run base38-encode.go gif json

import (
	"fmt"
	"os"
	"strings"

	"github.com/google/wuffs/lib/base38"
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	args := os.Args
	if len(args) > 1 {
		for _, arg := range args[1:] {
			arg := (strings.ToLower(arg) + "    ")[:4]
			if code, ok := base38.Encode(arg); ok {
				code0 := fmt.Sprintf("0x%06X", code)
				code1 := fmt.Sprintf("0x%08X", code<<10)
				fmt.Printf("%s    code: %s %s    code<<10: %s %s\n",
					arg, code0, underscore(code0), code1, underscore(code1))
			} else {
				fmt.Printf("%s -\n", arg)
			}
		}
	}
	return nil
}

func underscore(s string) string {
	if n := len(s) - 4; n > 0 {
		return s[:n] + "_" + s[n:]
	}
	return s
}
