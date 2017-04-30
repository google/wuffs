// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

package token

import (
	"errors"
	"fmt"
)

type IDMap struct {
	byName map[string]ID
	byKey  []string
}

func (m *IDMap) insert(name string) (ID, error) {
	if name == "" {
		return 0, nil
	}
	if id, ok := builtInsByName[name]; ok {
		return id, nil
	}
	if m.byName == nil {
		m.byName = map[string]ID{}
	}
	if id, ok := m.byName[name]; ok {
		return id, nil
	}

	key := nBuiltInKeys + Key(len(m.byKey))
	if key > maxKey {
		return 0, errors.New("token: too many distinct tokens")
	}
	flags := FlagsImplicitSemicolon
	if numeric(name[0]) {
		flags |= FlagsLiteral
	} else {
		flags |= FlagsIdent
	}
	id := ID(key<<idShift) | ID(flags)
	m.byName[name] = id
	m.byKey = append(m.byKey, name)
	return id, nil
}

func (m *IDMap) ByName(name string) ID {
	if m.byName != nil {
		return m.byName[name]
	}
	return 0
}

func (m *IDMap) ByKey(k Key) string {
	if k < nBuiltInKeys {
		return builtInsByKey[k].name
	}
	k -= nBuiltInKeys
	if uint(k) < uint(len(m.byKey)) {
		return m.byKey[k]
	}
	return ""
}

type Token struct {
	ID   ID
	Line int32
}

func alpha(c byte) bool {
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || (c == '_')
}

func alphaNumeric(c byte) bool {
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || (c == '_') || ('0' <= c && c <= '9')
}

func numeric(c byte) bool {
	return ('0' <= c && c <= '9')
}

func hasPrefix(a []byte, s string) bool {
	if len(s) == 0 {
		return true
	}
	if len(a) < len(s) {
		return false
	}
	a = a[:len(s)]
	for i := range a {
		if a[i] != s[i] {
			return false
		}
	}
	return true
}

func containingLine(src []byte, i int) []byte {
	j := i
	for ; j < len(src) && src[j] != '\n'; j++ {
	}
	for ; i > 0 && src[i-1] != '\n'; i-- {
	}
	return src[i:j]
}

func trim(b []byte) []byte {
	// Trim the back.
	i := len(b)
	for ; i > 0 && b[i-1] <= ' '; i-- {
	}
	b = b[:i]

	// Trim the front.
	i = 0
	for ; i < len(b) && b[i] <= ' '; i++ {
	}
	b = b[i:]

	return b
}

func Tokenize(src []byte, m *IDMap, filename string) (tokens []Token, retErr error) {
	line := int32(1)
loop:
	for i := 0; i < len(src); {
		c := src[i]

		if c <= ' ' {
			if c == '\n' {
				if len(tokens) > 0 && tokens[len(tokens)-1].ID.IsImplicitSemicolon() {
					tokens = append(tokens, Token{IDSemicolon, line})
				}
				line++
			}
			i++
			continue
		}

		if alpha(c) {
			j := i + 1
			for ; j < len(src) && alphaNumeric(src[j]); j++ {
			}
			id, err := m.insert(string(src[i:j]))
			if err != nil {
				return nil, err
			}
			tokens = append(tokens, Token{id, line})
			i = j
			continue
		}

		if numeric(c) {
			// TODO: 0x12 hex and 0b11 binary numbers.
			//
			// TODO: allow underscores like 0b1000_0000_1111?
			//
			// TODO: save numbers in a separate map, not the IDMap??
			j := i + 1
			for ; j < len(src) && numeric(src[j]); j++ {
			}
			id, err := m.insert(string(src[i:j]))
			if err != nil {
				return nil, err
			}
			tokens = append(tokens, Token{id, line})
			i = j
			continue
		}

		if c == '/' && i+1 < len(src) && src[i+1] == '/' {
			// TODO: save comments, dont' just skip them. We'll need to render
			// them for puffsfmt.
			i += 2
			for ; i < len(src) && src[i] != '\n'; i++ {
			}
			continue
		}

		if id := squiggles[c]; id != 0 {
			i++
			if id.Flags() != 0 {
				tokens = append(tokens, Token{id, line})
				continue
			}
			for _, x := range lexers[c] {
				if hasPrefix(src[i:], x.suffix) {
					i += len(x.suffix)
					tokens = append(tokens, Token{x.id, line})
					continue loop
				}
			}
		}

		msg := ""
		if c <= 0x7F {
			msg = fmt.Sprintf("byte '\\x%02X' (%q)", c, c)
		} else {
			msg = fmt.Sprintf("non-ASCII byte '\\x%02X'", c)
		}
		if filename == "" {
			filename = "line"
		}
		return nil, fmt.Errorf("token: unrecognized %s at %s:%d. line contents: %q",
			msg, filename, line, trim(containingLine(src, i)))
	}
	return tokens, nil
}
