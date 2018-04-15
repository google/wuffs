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

package token

import (
	"errors"
	"fmt"
)

const (
	maxID        = 1048575
	maxLine      = 1048575
	maxTokenSize = 1023
)

func Unescape(s string) (unescaped string, ok bool) {
	if len(s) < 2 || s[0] != '"' || s[len(s)-1] != '"' {
		return "", false
	}
	return s[1 : len(s)-1], true
}

type Map struct {
	byName map[string]ID
	byID   []string
}

func (m *Map) Insert(name string) (ID, error) {
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

	id := nBuiltInIDs + ID(len(m.byID))
	if id > maxID {
		return 0, errors.New("token: too many distinct tokens")
	}
	m.byName[name] = id
	m.byID = append(m.byID, name)
	return id, nil
}

func (m *Map) ByName(name string) ID {
	if id, ok := builtInsByName[name]; ok {
		return id
	}
	if m.byName != nil {
		return m.byName[name]
	}
	return 0
}

func (m *Map) ByID(x ID) string {
	if x < nBuiltInIDs {
		return builtInsByID[x]
	}
	x -= nBuiltInIDs
	if uint(x) < uint(len(m.byID)) {
		return m.byID[x]
	}
	return ""
}

func alpha(c byte) bool {
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || (c == '_')
}

func alphaNumeric(c byte) bool {
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || (c == '_') || ('0' <= c && c <= '9')
}

func hexaNumeric(c byte) bool {
	return ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f') || ('0' <= c && c <= '9')
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

func Tokenize(m *Map, filename string, src []byte) (tokens []Token, comments []string, retErr error) {
	line := uint32(1)
loop:
	for i := 0; i < len(src); {
		c := src[i]

		if c <= ' ' {
			if c == '\n' {
				if len(tokens) > 0 && tokens[len(tokens)-1].ID.IsImplicitSemicolon(m) {
					tokens = append(tokens, Token{IDSemicolon, line})
				}
				if line == maxLine {
					return nil, nil, fmt.Errorf("token: too many lines in %q", filename)
				}
				line++
			}
			i++
			continue
		}

		// TODO: recognize escapes such as `\t`, `\"` and `\\`. For now, we
		// assume that strings don't contain control bytes or backslashes.
		// Neither should be necessary to parse `use "foo/bar"` lines.
		if c == '"' {
			j := i + 1
			for ; j < len(src); j++ {
				c = src[j]
				if c == '"' {
					j++
					break
				}
				if c == '\\' {
					return nil, nil, fmt.Errorf("token: backslash in string at %s:%d", filename, line)
				}
				if c == '\n' {
					return nil, nil, fmt.Errorf("token: expected final '\"' in string at %s:%d", filename, line)
				}
				if c < ' ' {
					return nil, nil, fmt.Errorf("token: control character in string at %s:%d", filename, line)
				}
				// The -1 is because we still haven't seen the final '"'.
				if j-i == maxTokenSize-1 {
					return nil, nil, fmt.Errorf("token: string too long at %s:%d", filename, line)
				}
			}
			id, err := m.Insert(string(src[i:j]))
			if err != nil {
				return nil, nil, err
			}
			tokens = append(tokens, Token{id, line})
			i = j
			continue
		}

		if alpha(c) {
			j := i + 1
			for ; j < len(src) && alphaNumeric(src[j]); j++ {
				if j-i == maxTokenSize {
					return nil, nil, fmt.Errorf("token: identifier too long at %s:%d", filename, line)
				}
			}
			id, err := m.Insert(string(src[i:j]))
			if err != nil {
				return nil, nil, err
			}
			tokens = append(tokens, Token{id, line})
			i = j
			continue
		}

		if numeric(c) {
			// TODO: 0b11 binary numbers.
			//
			// TODO: allow underscores like 0b1000_0000_1111?
			j, isDigit := i+1, numeric
			if c == '0' && j < len(src) {
				if next := src[j]; next == 'x' || next == 'X' {
					j, isDigit = j+1, hexaNumeric
				} else if numeric(next) {
					return nil, nil, fmt.Errorf("token: legacy octal syntax at %s:%d", filename, line)
				}
			}
			for ; j < len(src) && isDigit(src[j]); j++ {
				if j-i == maxTokenSize {
					return nil, nil, fmt.Errorf("token: constant too long at %s:%d", filename, line)
				}
			}
			id, err := m.Insert(string(src[i:j]))
			if err != nil {
				return nil, nil, err
			}
			tokens = append(tokens, Token{id, line})
			i = j
			continue
		}

		if c == '/' && i+1 < len(src) && src[i+1] == '/' {
			h := i
			i += 2
			for ; i < len(src) && src[i] != '\n'; i++ {
			}
			for uint32(len(comments)) < line {
				comments = append(comments, "")
			}
			comments = append(comments, string(src[h:i]))
			continue
		}

		if id := squiggles[c]; id != 0 {
			i++
			tokens = append(tokens, Token{id, line})
			continue
		}
		for _, x := range lexers[c] {
			if hasPrefix(src[i+1:], x.suffix) {
				i += len(x.suffix) + 1
				tokens = append(tokens, Token{x.id, line})
				continue loop
			}
		}

		msg := ""
		if c <= 0x7F {
			msg = fmt.Sprintf("byte '\\x%02X' (%q)", c, c)
		} else {
			msg = fmt.Sprintf("non-ASCII byte '\\x%02X'", c)
		}
		return nil, nil, fmt.Errorf("token: unrecognized %s at %s:%d", msg, filename, line)
	}
	return tokens, comments, nil
}
