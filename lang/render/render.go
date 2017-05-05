// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package render

// TODO: render an *ast.Node instead of a []token.Token, as this will better
// allow automated refactoring tools, not just automated formatting. For now,
// rendering tokens is easier, especially with respect to tracking comments.

// TODO: handle comments.

import (
	"errors"
	"io"

	"github.com/google/puffs/lang/token"
)

var newLine = []byte{'\n'}

func RenderFile(w io.Writer, src []token.Token, m *token.IDMap) (err error) {
	if len(src) == 0 {
		return nil
	}

	const maxIndent = 0xFFFF
	indent := 0
	buf := make([]byte, 0, 1024)
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

		// Strip any trailing semi-colons.
		hanging := prevLineHanging
		prevLineHanging = true
		for len(lineTokens) > 0 && lineTokens[len(lineTokens)-1].ID == token.IDSemicolon {
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
		}

		// Render any leading indentation. If this line starts with a close
		// curly, outdent it so that it hopefully aligns vertically with the
		// line containing the matching open curly.
		buf = buf[:0]
		indentAdjustment := 0
		if lineTokens[0].ID == token.IDCloseCurly {
			indentAdjustment--
		} else if hanging {
			indentAdjustment++
		}
		if n := indent + indentAdjustment; n > 0 {
			const tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
			for ; n > len(tabs); n -= len(tabs) {
				buf = append(buf, tabs...)
			}
			buf = append(buf, tabs[:n]...)
		}

		// Render the lineTokens.
		prevID, prevIsTightRight := token.ID(0), false
		for _, t := range lineTokens {
			const (
				flagsUB  = token.FlagsUnaryOp | token.FlagsBinaryOp
				flagsLIC = token.FlagsLiteral | token.FlagsIdent | token.FlagsClose
			)

			if prevID != 0 && !prevIsTightRight && !t.IsTightLeft() {
				// The "(" token's tight-left-ness is context dependent. For
				// "f(x)", the "(" is tight-left. For "a * (b + c)", it is not.
				if t.ID != token.IDOpenParen || prevID.Flags()&flagsLIC == 0 {
					buf = append(buf, ' ')
				}
			}

			buf = append(buf, m.ByKey(t.Key())...)

			if t.ID == token.IDOpenCurly {
				if indent == maxIndent {
					return errors.New("render: too many \"{\" tokens")
				}
				indent++
			} else if t.ID == token.IDCloseCurly {
				if indent == 0 {
					return errors.New("render: too many \"}\" tokens")
				}
				indent--
			}

			prevIsTightRight = t.ID.IsTightRight()
			// The "+" and "-" tokens' tight-right-ness is context dependent.
			// The unary flavor is tight-right, the binary flavor is not.
			if prevID != 0 && t.ID.Flags()&flagsUB == flagsUB {
				// Token-based (not ast.Node-based) heuristic for whether the
				// operator looks unary instead of binary.
				prevIsTightRight = prevID.Flags()&flagsLIC == 0
			}

			prevID = t.ID
		}

		buf = append(buf, '\n')
		if _, err = w.Write(buf); err != nil {
			return err
		}
		prevLine = line
		prevLineHanging = prevLineHanging && lineTokens[len(lineTokens)-1].ID != token.IDOpenCurly
	}

	return nil
}
