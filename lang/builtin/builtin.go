// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// Package builtin lists Puffs' built-in concepts such as status codes.
package builtin

import (
	t "github.com/google/puffs/lang/token"
)

type Status struct {
	Keyword t.ID
	Message string
}

func (z Status) String() string {
	prefix := "status "
	switch z.Keyword {
	case t.IDError:
		prefix = "error "
	case t.IDSuspension:
		prefix = "suspension "
	}
	return prefix + z.Message
}

var StatusList = [...]Status{
	{0, "ok"},
	{t.IDError, "bad puffs version"},
	{t.IDError, "bad receiver"},
	{t.IDError, "bad argument"},
	{t.IDError, "initializer not called"},
	{t.IDError, "closed for writes"},
	{t.IDError, "unexpected EOF"},  // Used if reading when closed == true.
	{t.IDSuspension, "short read"}, // Used if reading when closed == false.
	{t.IDSuspension, "short write"},
}

var StatusMap = map[string]Status{}

func init() {
	for _, s := range StatusList {
		StatusMap[s.Message] = s
	}
}

func TrimQuotes(s string) string {
	if len(s) >= 2 && s[0] == '"' && s[len(s)-1] == '"' {
		return s[1 : len(s)-1]
	}
	return s
}
