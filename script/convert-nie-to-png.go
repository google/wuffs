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

//go:build ignore
// +build ignore

package main

// convert-nie-to-png.go decodes NIE from stdin and encodes PNG to stdout.
//
// Usage: go run convert-nie-to-png.go < foo.nie > foo.png

import (
	"image/png"
	"os"

	"github.com/google/wuffs/lib/nie"
)

func main() {
	if err := main1(); err != nil {
		os.Stderr.WriteString(err.Error() + "\n")
		os.Exit(1)
	}
}

func main1() error {
	img, err := nie.Decode(os.Stdin)
	if err != nil {
		return err
	}
	return png.Encode(os.Stdout, img)
}
