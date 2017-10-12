// Copyright 2017 The Puffs Authors.
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
