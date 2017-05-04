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

	const maxOpens = 0xFFFF
	opens := make([]uint32, 0, 256)
	indent := uint32(0)
	buf := make([]byte, 0, 1024)
	prevLine := src[0].Line - 1
	prevLineEndedWithOpen := false

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
		for len(lineTokens) > 0 && lineTokens[len(lineTokens)-1].ID == token.IDSemicolon {
			lineTokens = lineTokens[:len(lineTokens)-1]
		}
		if len(lineTokens) == 0 {
			continue
		}

		// Collapse one or more blank lines to just one. Collapse them to zero
		// if the previous line ended with an open token (such as an open curly
		// brace) or this line started with a close token.
		if prevLine < line-1 && !prevLineEndedWithOpen && !lineTokens[0].IsClose() {
			if _, err = w.Write(newLine); err != nil {
				return err
			}
		}

		// Render any leading indentation. If this line starts with a close
		// token, indent it so that it aligns vertically with the line
		// containing the matching open token.
		buf = buf[:0]
		if lineTokens[0].IsClose() {
			if len(opens) > 0 {
				indent = opens[len(opens)-1]
			} else {
				indent = 0
			}
		}
		if indent != 0 {
			const tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
			n := int(indent)
			for ; n > len(tabs); n -= len(tabs) {
				buf = append(buf, tabs...)
			}
			buf = append(buf, tabs[:n]...)
		}

		// Render the lineTokens.
		numOpens := len(opens)
		prevID := token.ID(0)
		for _, t := range lineTokens {
			if prevID != 0 && !prevID.IsTightRight() && !t.IsTightLeft() {
				// The "(" token's tight-left-ness is context dependent. For
				// "f(x)", the "(" is tight-left. For "a * (b + c)", it is not.
				if t.ID != token.IDOpenParen || prevID.IsBinaryOp() {
					buf = append(buf, ' ')
				}
			}

			buf = append(buf, m.ByKey(t.Key())...)

			if t.IsOpen() {
				if len(opens) == maxOpens {
					return errors.New("render: too many open tokens")
				}
				opens = append(opens, indent)
			} else if t.IsClose() {
				if len(opens) == 0 {
					return errors.New("render: too many close tokens")
				}
				opens = opens[:len(opens)-1]
			}

			prevID = t.ID
		}
		if numOpens != len(opens) {
			indent++
		}

		buf = append(buf, '\n')
		if _, err = w.Write(buf); err != nil {
			return err
		}
		prevLine = line
		prevLineEndedWithOpen = lineTokens[len(lineTokens)-1].IsOpen()
	}

	return nil
}
