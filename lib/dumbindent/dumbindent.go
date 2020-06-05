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

// ----------------

// Package dumbindent formats C (and C-like) programs.
//
// It is similar in concept to pretty-printers like `indent` or `clang-format`.
// It is much dumber (it will not add line breaks or otherwise re-flow lines of
// code, not to fit within an 80 character limit nor for any other reason) but
// it can therefore be much, much faster at the basic task of automatically
// indenting nested blocks. The output isn't 'perfect', but it's usually
// sufficiently readable if the input already has sensible line breaks.
//
// See `cmd/dumbindent/main.go` in this repository for an example where
// `dumbindent` was 70 times faster than `clang-format`.
//
// There are no configuration options (e.g. tabs versus spaces).
//
// Known bug: it cannot handle /* slash-star comments */ or multi-line strings
// yet. This is tracked at https://github.com/google/wuffs/issues/31
package dumbindent

import (
	"bytes"
	"errors"
)

// 'Constants', but their type is []byte, not string.
var (
	externC = []byte("extern \"C\"")
	spaces  = []byte("                                ")
)

// Global state.
var (
	nBraces   int  // The number of unbalanced '{'s.
	nParens   int  // The number of unbalanced '('s.
	openBrace bool // Whether the previous line ends with '{'.
	hanging   bool // Whether the previous line ends with '=' or '\\'.
)

// hangingBytes is a look-up table for updating the hanging global variable.
var hangingBytes = [256]bool{
	'=':  true,
	'\\': true,
}

// Format formats the C (or C-like) program in src.
func Format(src []byte) (dst []byte, retErr error) {
	dst = make([]byte, 0, len(src)+(len(src)/2))
	blankLine := false
	for line, remaining := src, []byte(nil); len(src) > 0; src = remaining {
		line, remaining = src, nil
		if i := bytes.IndexByte(line, '\n'); i >= 0 {
			line, remaining = line[:i], line[i+1:]
		}
		line = trimSpace(line)

		// Collapse 2 or more consecutive blank lines into 1. Also strip any
		// blank lines:
		//  - immediately after a '{',
		//  - immediately before a '}',
		//  - at the end of file.
		if len(line) == 0 {
			blankLine = true
			continue
		}
		if blankLine {
			blankLine = false
			if !openBrace && (line[0] != '}') {
				dst = append(dst, '\n')
			}
		}

		// Preprocessor lines (#ifdef, #pragma, etc) are never indented.
		//
		// The '{' and '}' for an `extern "C"` are also special cased not to
		// change indentation inside the block. This assumes that the closing
		// brace is followed by a `// extern "C"` comment.
		if (line[0] == '#') ||
			((line[0] == 'e') && bytes.HasPrefix(line, externC)) ||
			((line[0] == '}') && bytes.HasSuffix(line, externC)) {
			dst = append(dst, line...)
			dst = append(dst, '\n')
			openBrace = false
			hanging = lastNonWhiteSpace(line) == '\\'
			continue
		}

		// Account for leading '}'s before we print the line's indentation.
		closeBraces := 0
		for ; (closeBraces < len(line)) && line[closeBraces] == '}'; closeBraces++ {
		}
		nBraces -= closeBraces

		// When debugging, uncomment these (and import "strconv") to prefix
		// every non-blank line with the global state.
		//
		// dst = append(dst, strconv.Itoa(nBraces)...)
		// dst = append(dst, ',')
		// dst = append(dst, strconv.Itoa(nParens)...)
		// if openBrace {
		//   dst = append(dst, '{')
		// } else {
		//   dst = append(dst, ':')
		// }
		// if hanging {
		//   dst = append(dst, '=')
		// } else {
		//   dst = append(dst, ':')
		// }

		// Output a certain number of spaces to rougly approximate the
		// "clang-format -style=Chromium" indentation style.
		indent := 0
		if nBraces > 0 {
			indent += 2 * nBraces
		}
		if (nParens > 0) || hanging {
			indent += 4
		}
		if (indent >= 2) && isLabel(line) {
			indent -= 2
		}
		for indent > 0 {
			n := indent
			if n > len(spaces) {
				n = len(spaces)
			}
			dst = append(dst, spaces[:n]...)
			indent -= n
		}

		// Output the line itself.
		dst = append(dst, line...)
		dst = append(dst, "\n"...)

		// Adjust the global state according to the braces and parentheses
		// within the line (except for those in comments and strings).
		last := lastNonWhiteSpace(line)
	loop:
		for s := line[closeBraces:]; ; {
			for i, c := range s {
				switch c {
				case '{':
					nBraces++
				case '}':
					nBraces--
				case '(':
					nParens++
				case ')':
					nParens--
				case '/':
					if (i + 1) >= len(s) {
						break
					}
					if s[i+1] == '/' {
						// A slash-slash comment. Skip the rest of the line.
						last = lastNonWhiteSpace(s[:i])
						break loop
					} else if s[i+1] == '*' {
						return nil, errors.New("dumbindent: TODO: support slash-star comments")
					}
				case '"', '\'':
					if suffix, err := skipString(s[i+1:], c); err != nil {
						return nil, err
					} else {
						s = suffix
					}
					continue loop
				}
			}
			break loop
		}
		openBrace = last == '{'
		hanging = hangingBytes[last]
	}
	return dst, nil
}

// trimSpace converts "\t  foo bar " to "foo bar".
func trimSpace(s []byte) []byte {
	for (len(s) > 0) && ((s[0] == ' ') || (s[0] == '\t')) {
		s = s[1:]
	}
	for (len(s) > 0) && ((s[len(s)-1] == ' ') || (s[len(s)-1] == '\t')) {
		s = s[:len(s)-1]
	}
	return s
}

// isLabel returns whether s looks like "foo:" or "bar_baz:;".
func isLabel(s []byte) bool {
	for (len(s) > 0) && (s[len(s)-1] == ';') {
		s = s[:len(s)-1]
	}
	if (len(s) < 2) || (s[len(s)-1] != ':') {
		return false
	}
	s = s[:len(s)-1]
	for _, c := range s {
		switch {
		case ('0' <= c) && (c <= '9'):
		case ('A' <= c) && (c <= 'Z'):
		case ('a' <= c) && (c <= 'z'):
		case c == '_':
		default:
			return false
		}
	}
	return true
}

// lastNonWhiteSpace returns the 'z' in "abc xyz  ". It returns '\x00' if s
// consists entirely of spaces or tabs.
func lastNonWhiteSpace(s []byte) byte {
	for i := len(s) - 1; i >= 0; i-- {
		if x := s[i]; (x != ' ') && (x != '\t') {
			return x
		}
	}
	return 0
}

// skipString converts `ijk \" lmn" pqr` to ` pqr`.
func skipString(s []byte, quote byte) (suffix []byte, retErr error) {
	for i := 0; i < len(s); {
		if x := s[i]; x == quote {
			return s[i+1:], nil
		} else if x != '\\' {
			i += 1
		} else if (i + 1) < len(s) {
			i += 2
		} else {
			break
		}
	}
	return nil, errors.New("dumbindent: TODO: support multi-line strings")
}
