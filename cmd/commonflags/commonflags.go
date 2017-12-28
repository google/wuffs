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

// commonflags holds flag defaults and usage messages that are common to the
// Wuffs command line tools.
//
// It also holds functions to parse and validate these flag values.
package commonflags

import (
	"path"
)

const (
	CcompilersDefault = "clang,gcc"
	CcompilersUsage   = `comma-separated list of C compilers, e.g. "clang,gcc"`

	FocusDefault = ""
	FocusUsage   = `comma-separated list of tests or benchmarks (name prefixes) to focus on, e.g. "wuffs_gif_decode"`

	MimicDefault = false
	MimicUsage   = `whether to compare Wuffs' output with other libraries' output`

	RepsDefault = 5
	RepsMin     = 0
	RepsMax     = 1000000
	RepsUsage   = `the number of repetitions per benchmark`
)

// TODO: do IsAlphaNumericIsh and IsValidUsePath belong in a separate package,
// such as lang/validate? Perhaps together with token.Unescape?

// IsAlphaNumericIsh returns whether s contains only ASCII alpha-numerics and a
// limited set of punctuation such as commas and slahes, but not containing
// e.g. spaces, semi-colons, colons or backslashes.
//
// The intent is that if s is alpha-numeric-ish, then it should not need
// escaping when passed to other programs as command line arguments.
func IsAlphaNumericIsh(s string) bool {
	for i := 0; i < len(s); i++ {
		c := s[i]
		if c == ',' || c == '-' || c == '.' || c == '/' || ('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') ||
			c == '_' || ('a' <= c && c <= 'z') {
			continue
		}
		return false
	}
	return true
}

func IsValidUsePath(s string) bool {
	return s == path.Clean(s) && s != "" && s[0] != '.' && s[0] != '/'
}
