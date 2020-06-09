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
// code just to fit within an 80 column limit) but it can therefore be much
// faster at the basic task of automatically indenting nested blocks. The
// output isn't 'perfect', but it's usually sufficiently readable if the input
// already has sensible line breaks.
//
// To quantify "much faster", on this one C file, `cmd/dumbindent` in this
// repository was 80 times faster than `clang-format`, even without a column
// limit:
//
//     $ wc release/c/wuffs-v0.2.c
//      11858  35980 431885 release/c/wuffs-v0.2.c
//     $ time dumbindent                               < release/c/wuffs-v0.2.c > /dev/null
//     real    0m0.008s
//     user    0m0.005s
//     sys     0m0.005s
//     $ time clang-format-9                           < release/c/wuffs-v0.2.c > /dev/null
//     real    0m0.668s
//     user    0m0.618s
//     sys     0m0.032s
//     $ time clang-format-9 -style='{ColumnLimit: 0}' < release/c/wuffs-v0.2.c > /dev/null
//     real    0m0.641s
//     user    0m0.585s
//     sys     0m0.037s
//
// Apart from some rare and largely uninteresting exceptions, the dumbindent
// algorithm only considers:
//
//   ∙ '{' and '}' curly braces,
//   ∙ '(' and ')' round parentheses,
//   ∙ ';' semi-colons (if they're not within parentheses),
//   ∙ strings, comments and preprocessor directives,
//   ∙ '\n' line breaks, and
//   ∙ ' ' spaces and '\t' tabs adjacent to any of the above.
//
// Everything else is an opaque byte. Consider this input:
//
//     for (i = 0; i < 3; i++) {
//     j = 0; j++;  // Semi-colons not within parentheses.
//     if (i < j) { foo(); }
//     u = (v +
//     w);
//     }
//
// From the algorithm's point of view, this input is equivalent to:
//
//     ... (.................) {
//     .....; ...;  //....................................
//     .. (.....) { ...(); }
//     ... (...
//     .);
//     }
//
// The formatted output (using the default of 2 spaces per indent level) is:
//
//     ... (.................) {
//       .....;
//       ...;  //....................................
//       .. (.....) {
//         ...();
//       }
//       ... (...
//           .);
//     }
//
// Line breaks are generally forced after semi-colons (outside of a for loop's
// triple expressions) and open-braces. Each output line is then indented
// according to the net number of open braces preceding it, although lines
// starting with close braces will outdent first, similar to `gofmt` style. A
// line whose start is inside a so-far-unbalanced open parenthesis, such as the
// "w);" line above, gets 2 additional indent levels.
//
// Dumbindent will remove some superfluous blank lines, but it will not remove
// line breaks between consecutive non-empty lines, such as between "u = (v +"
// and "w);" in the example above.
//
// Dumbindent does not parse the input as C/C++ source code. In particular, the
// algorithm has no interest in solving C++'s "most vexing parse" or
// determining whether "x*y" is a multiplication or a type definition (where
// "x" is a type and "y" is a pointer-typed variable, such as "int*p"). For a
// type definition, where other formatting algorithms would re-write around the
// "*" as either "x* y" or "x *y", dumbindent will not insert spaces.
//
// Similarly, dumbindent will not correct this mis-indentation:
//
//     if (condition)
//       goto fail;
//       goto fail;
//
// Instead, when automatically or manually generating the input for dumbindent,
// it is recommended to always emit curly braces (again, similar to `gofmt`
// style), even for what would otherwise be 'one-liner' if statements.
package dumbindent

import (
	"bytes"
)

// 'Constants', but their type is []byte, not string.
var (
	backTick  = []byte("`")
	externC   = []byte("extern \"C\" {")
	namespace = []byte("namespace ")
	starSlash = []byte("*/")

	spaces = []byte("                                ")
	tabs   = []byte("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t")
)

// hangingBytes is a look-up table for updating the hanging variable.
var hangingBytes = [256]bool{
	'=':  true,
	'\\': true,
}

// Options are formatting options.
type Options struct {
	// Spaces, if positive, is the number of spaces per indentation level. A
	// non-positive value means to use the default: 2 spaces per indent.
	//
	// This field is ignored when Tabs is true.
	Spaces int

	// Tabs is whether to indent with tabs instead of spaces. If true, it's one
	// '\t' tab character per indent and the Spaces field is ignored.
	Tabs bool
}

// FormatBytes formats the C (or C-like) program in src, appending the result
// to dst, and returns that longer slice.
//
// It is valid to pass a dst slice (such as nil) whose unused capacity
// (cap(dst) - len(dst)) is too short to hold the formatted program. In this
// case, a new slice will be allocated and returned.
//
// Passing a nil opts is valid and equivalent to passing &Options{}.
func FormatBytes(dst []byte, src []byte, opts *Options) []byte {
	src = trimLeadingWhiteSpaceAndNewLines(src)
	if len(src) == 0 {
		return dst
	} else if len(dst) == 0 {
		dst = make([]byte, 0, len(src)+(len(src)/2))
	}

	indentBytes := spaces
	indentCount := 2
	if opts != nil {
		if opts.Tabs {
			indentBytes = tabs
			indentCount = 1
		} else if opts.Spaces > 0 {
			indentCount = opts.Spaces
		}
	}

	nBraces := 0       // The number of unbalanced '{'s.
	nParens := 0       // The number of unbalanced '('s.
	openBrace := false // Whether the previous non-blank line ends with '{'.
	hanging := false   // Whether the previous non-blank line ends with '=' or '\\'.
	blankLine := false // Whether the previous line was blank.

outer:
	for line, remaining := src, []byte(nil); len(src) > 0; src = remaining {
		src = trimLeadingWhiteSpace(src)
		line, remaining = src, nil
		if i := bytes.IndexByte(line, '\n'); i >= 0 {
			line, remaining = line[:i], line[i+1:]
		}
		lineLength := len(line)

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
		// Also catch `extern "C" {` and `namespace foo {`.
		if (line[0] == '#') ||
			((line[0] == 'e') && bytes.HasPrefix(line, externC)) ||
			((line[0] == 'n') && bytes.HasPrefix(line, namespace)) {
			line = trimTrailingWhiteSpace(line)
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

		// The heuristics aren't perfect, and sometimes do not catch braces or
		// parentheses in #define macros. They also don't increment nBraces for
		// `extern "C"` or namespace lines. We work around that here, clamping
		// to zero.
		if nBraces < 0 {
			nBraces = 0
		}
		if nParens < 0 {
			nParens = 0
		}

		// Output a certain number of spaces to rougly approximate the
		// "clang-format -style=Chromium" indentation style.
		indent := 0
		if nBraces > 0 {
			indent += indentCount * nBraces
		}
		if (nParens > 0) || hanging {
			indent += indentCount * 2
		}
		for indent > 0 {
			n := indent
			if n > len(indentBytes) {
				n = len(indentBytes)
			}
			dst = append(dst, indentBytes[:n]...)
			indent -= n
		}

		// Output the leading '}'s.
		dst = append(dst, line[:closeBraces]...)
		line = line[closeBraces:]

		// Adjust the state according to the braces and parentheses within the
		// line (except for those in comments and strings).
		last := lastNonWhiteSpace(line)
	inner:
		for {
			for i, c := range line {
				switch c {
				case '{':
					nBraces++
					if breakAfterBrace(line[i+1:]) {
						dst = append(dst, line[:i+1]...)
						dst = append(dst, '\n')
						restOfLine := line[i+1:]
						remaining = src[lineLength-len(restOfLine):]
						openBrace = true
						hanging = false
						continue outer
					}
				case '}':
					nBraces--
				case '(':
					nParens++
				case ')':
					nParens--

				case ';':
					if (nParens == 0) && (breakAfterSemicolon(line[i+1:])) {
						dst = append(dst, line[:i+1]...)
						dst = append(dst, '\n')
						restOfLine := line[i+1:]
						remaining = src[lineLength-len(restOfLine):]
						openBrace = false
						hanging = false
						continue outer
					}

				case '/':
					if (i + 1) >= len(line) {
						break
					}
					if line[i+1] == '/' {
						// A slash-slash comment. Skip the rest of the line.
						last = lastNonWhiteSpace(line[:i])
						break inner
					} else if line[i+1] == '*' {
						// A slash-star comment.
						dst = append(dst, line[:i+2]...)
						restOfLine := line[i+2:]
						restOfSrc := src[lineLength-len(restOfLine):]
						dst, line, remaining = handleRaw(dst, restOfSrc, starSlash)
						last = lastNonWhiteSpace(line)
						continue inner
					}

				case '"', '\'':
					// A cooked string, whose contents are backslash-escaped.
					suffix := skipCooked(line[i+1:], c)
					dst = append(dst, line[:len(line)-len(suffix)]...)
					line = suffix
					continue inner

				case '`':
					// A raw string.
					dst = append(dst, line[:i+1]...)
					restOfLine := line[i+1:]
					restOfSrc := src[lineLength-len(restOfLine):]
					dst, line, remaining = handleRaw(dst, restOfSrc, backTick)
					last = lastNonWhiteSpace(line)
					continue inner
				}
			}
			break inner
		}
		openBrace = last == '{'
		hanging = hangingBytes[last]

		// Output the line (minus any trailing space).
		line = trimTrailingWhiteSpace(line)
		dst = append(dst, line...)
		dst = append(dst, "\n"...)
	}
	return dst
}

// trimLeadingWhiteSpaceAndNewLines converts "\t\n  foo bar " to "foo bar ".
func trimLeadingWhiteSpaceAndNewLines(s []byte) []byte {
	for (len(s) > 0) && ((s[0] == ' ') || (s[0] == '\t') || (s[0] == '\n')) {
		s = s[1:]
	}
	return s
}

// trimLeadingWhiteSpace converts "\t\t  foo bar " to "foo bar ".
func trimLeadingWhiteSpace(s []byte) []byte {
	for (len(s) > 0) && ((s[0] == ' ') || (s[0] == '\t')) {
		s = s[1:]
	}
	return s
}

// trimTrailingWhiteSpace converts "\t\t  foo bar " to "\t\t  foo bar".
func trimTrailingWhiteSpace(s []byte) []byte {
	for (len(s) > 0) && ((s[len(s)-1] == ' ') || (s[len(s)-1] == '\t')) {
		s = s[:len(s)-1]
	}
	return s
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

// skipCooked converts `ijk \" lmn" pqr` to ` pqr`.
func skipCooked(s []byte, quote byte) (suffix []byte) {
	for i := 0; i < len(s); {
		if x := s[i]; x == quote {
			return s[i+1:]
		} else if x != '\\' {
			i += 1
		} else if (i + 1) < len(s) {
			i += 2
		} else {
			break
		}
	}
	return nil
}

// handleRaw copies a raw string from restOfSrc to dst, re-calculating the
// (line, remaining) pair afterwards.
func handleRaw(dst []byte, restOfSrc []byte, endQuote []byte) (retDst []byte, line []byte, remaining []byte) {
	end := bytes.Index(restOfSrc, endQuote)
	if end < 0 {
		end = len(restOfSrc)
	} else {
		end += len(endQuote)
	}
	dst = append(dst, restOfSrc[:end]...)
	line, remaining = restOfSrc[end:], nil
	if i := bytes.IndexByte(line, '\n'); i >= 0 {
		line, remaining = line[:i], line[i+1:]
	}
	return dst, line, remaining
}

// breakAfterBrace returns whether s starts with a slash-slash comment or with
// the rest of a "0}" or ".a=1, .b=2}" literal, including the matching "}".
//
// This implementation isn't perfect, but it's good enough in practice.
func breakAfterBrace(s []byte) bool {
	s = trimLeadingWhiteSpace(s)
	if len(s) == 0 {
		return false
	} else if (len(s) > 1) && (s[0] == '/') && (s[1] == '/') {
		return false
	}

	n := 1
	for i, c := range s {
		switch c {
		case ';':
			return true
		case '{':
			n++
		case '}':
			n--
			if n == 0 {
				return false
			}
		case '/':
			if (i + 1) < len(s) {
				switch s[i+1] {
				case '/', '*':
					return true
				}
			}
		}
	}
	return true
}

// breakAfterSemicolon returns whether the first non-space non-tab byte of s
// (if any) does not look like a comment.
func breakAfterSemicolon(s []byte) bool {
	for _, c := range s {
		if (c != ' ') && (c != '\t') {
			return c != '/'
		}
	}
	return false
}
