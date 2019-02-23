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

package render

// TODO: render an *ast.Node instead of a []token.Token, as this will better
// allow automated refactoring tools, not just automated formatting. For now,
// rendering tokens is easier, especially with respect to tracking comments.

import (
	"errors"
	"io"

	t "github.com/google/wuffs/lang/token"
)

var newLine = []byte{'\n'}

func max(a, b int) int {
	if a > b {
		return a
	}
	return b
}

func Render(w io.Writer, tm *t.Map, src []t.Token, comments []string) (err error) {
	if len(src) == 0 {
		return nil
	}

	const maxIndent = 0xFFFF
	indent := 0
	buf := make([]byte, 0, 1024)
	commentLine := uint32(0)

	inStruct := false
	varNameLength := uint32(0)

	prevLine := src[0].Line - 1
	prevLineHanging := false

	for len(src) > 0 {
		// Find the tokens in this line.
		line := src[0].Line
		i := 1
		for ; i < len(src) && src[i].Line == line; i++ {
			if i == len(src) || src[i].Line != line {
				break
			}
		}
		lineTokens := src[:i]
		src = src[i:]

		// Print any previous comments.
		commentIndent := indent
		if prevLineHanging {
			commentIndent++
		}
		for ; commentLine < line; commentLine++ {
			buf = buf[:0]
			buf = appendComment(buf, comments, commentLine, commentIndent, true)
			if len(buf) == 0 {
				continue
			}
			if commentLine > prevLine+1 {
				if _, err = w.Write(newLine); err != nil {
					return err
				}
			}
			buf = append(buf, '\n')
			if _, err = w.Write(buf); err != nil {
				return err
			}
			varNameLength = 0
			prevLine = commentLine
		}

		// Strip any trailing semi-colons.
		hanging := prevLineHanging
		prevLineHanging = true
		for len(lineTokens) > 0 && lineTokens[len(lineTokens)-1].ID == t.IDSemicolon {
			prevLineHanging = false
			lineTokens = lineTokens[:len(lineTokens)-1]
		}
		if len(lineTokens) == 0 {
			continue
		}

		// Collapse one or more blank lines to just one.
		if prevLine < line-1 {
			if _, err = w.Write(newLine); err != nil {
				return err
			}
			varNameLength = 0
		}

		// Render any leading indentation. If this line starts with a close
		// token, outdent it so that it hopefully aligns vertically with the
		// line containing the matching open token.
		buf = buf[:0]
		indentAdjustment := 0
		if lineTokens[0].ID.IsClose() {
			indentAdjustment--
		} else if hanging && lineTokens[0].ID != t.IDOpenCurly {
			indentAdjustment++
		}
		buf = appendTabs(buf, indent+indentAdjustment)

		// Apply or update varNameLength.
		if len(lineTokens) < 3 {
			varNameLength = 0
		} else if id := lineTokens[0].ID; id == t.IDPri || id == t.IDPub {
			inStruct = lineTokens[1].ID == t.IDStruct
			varNameLength = 0
		} else if id == t.IDVar || inStruct {
			if varNameLength == 0 {
				varNameLength = measureVarNameLength(tm, lineTokens, src)
			}
			if id == t.IDVar {
				buf = append(buf, "var "...)
				lineTokens = lineTokens[1:]
			}
			name := tm.ByID(lineTokens[0].ID)
			lineTokens = lineTokens[1:]
			buf = append(buf, name...)
			for i := uint32(len(name)); i <= varNameLength; i++ {
				buf = append(buf, ' ')
			}
		} else {
			varNameLength = 0
		}

		// Render the lineTokens.
		prevID, prevIsTightRight := t.ID(0), false
		for _, tok := range lineTokens {
			if prevID == t.IDEq || (prevID != 0 && !prevIsTightRight && !tok.ID.IsTightLeft()) {
				// The "(" token's tight-left-ness is context dependent. For
				// "f(x)", the "(" is tight-left. For "a * (b + c)", it is not.
				if tok.ID != t.IDOpenParen || !isCloseIdentStrLiteral(tm, prevID) {
					buf = append(buf, ' ')
				}
			}

			buf = append(buf, tm.ByID(tok.ID)...)

			if tok.ID == t.IDOpenCurly {
				if indent == maxIndent {
					return errors.New("render: too many \"{\" tokens")
				}
				indent++
			} else if tok.ID == t.IDCloseCurly {
				if indent == 0 {
					return errors.New("render: too many \"}\" tokens")
				}
				indent--
			} else if (tok.ID == t.IDQuestion) && (prevID == t.IDYield) {
				buf = append(buf, ' ')
			}

			prevIsTightRight = tok.ID.IsTightRight()
			// The "+" and "-" tokens' tight-right-ness is context dependent.
			// The unary flavor is tight-right, the binary flavor is not.
			if prevID != 0 && tok.ID.IsUnaryOp() && tok.ID.IsBinaryOp() {
				// Token-based (not ast.Node-based) heuristic for whether the
				// operator looks unary instead of binary.
				prevIsTightRight = !isCloseIdentLiteral(tm, prevID)
			}

			prevID = tok.ID
		}

		buf = appendComment(buf, comments, line, 0, false)
		buf = append(buf, '\n')
		if _, err = w.Write(buf); err != nil {
			return err
		}
		commentLine = line + 1
		prevLine = line
		prevLineHanging = prevLineHanging && lineTokens[len(lineTokens)-1].ID != t.IDOpenCurly
	}

	// Print any trailing comments.
	for ; uint(commentLine) < uint(len(comments)); commentLine++ {
		buf = buf[:0]
		buf = appendComment(buf, comments, commentLine, indent, true)
		if len(buf) > 0 {
			if commentLine > prevLine+1 {
				if _, err = w.Write(newLine); err != nil {
					return err
				}
			}
			buf = append(buf, '\n')
			if _, err = w.Write(buf); err != nil {
				return err
			}
			prevLine = commentLine
		}
	}

	return nil
}

func appendComment(buf []byte, comments []string, line uint32, indent int, otherwiseEmpty bool) []byte {
	if uint(line) < uint(len(comments)) {
		if com := comments[line]; com != "" {
			// Strip trailing spaces.
			for len(com) > 0 {
				if com[len(com)-1] != ' ' {
					break
				}
				com = com[:len(com)-1]
			}

			if otherwiseEmpty {
				buf = appendTabs(buf, indent)
			} else {
				buf = append(buf, "  "...)
			}
			buf = append(buf, com...)
		}
	}
	return buf
}

func appendTabs(buf []byte, n int) []byte {
	if n > 0 {
		const tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
		for ; n > len(tabs); n -= len(tabs) {
			buf = append(buf, tabs...)
		}
		buf = append(buf, tabs[:n]...)
	}
	return buf
}

func isCloseIdentLiteral(tm *t.Map, x t.ID) bool {
	return x.IsClose() || x.IsIdent(tm) || x.IsLiteral(tm)
}

func isCloseIdentStrLiteral(tm *t.Map, x t.ID) bool {
	return x.IsClose() || x.IsIdent(tm) || x.IsStrLiteral(tm)
}

func measureVarNameLength(tm *t.Map, lineTokens []t.Token, remaining []t.Token) uint32 {
	if len(lineTokens) < 2 {
		return 0
	}

	x := 0 // "x T" struct field.
	if lineTokens[0].ID == t.IDVar {
		x = 1 // "var x T" var statement.
	}

	line := lineTokens[0].Line
	length := len(tm.ByID(lineTokens[x].ID))
	for (len(remaining) > x) &&
		((x == 0) || (remaining[0].ID == t.IDVar)) &&
		(remaining[0].Line == line+1) &&
		(remaining[x].Line == line+1) &&
		(remaining[x].ID.IsIdent(tm)) {

		line = remaining[0].Line
		length = max(length, len(tm.ByID(remaining[x].ID)))

		remaining = remaining[x+1:]
		for len(remaining) > 0 && remaining[0].Line == line {
			remaining = remaining[1:]
		}
	}
	return uint32(length)
}
