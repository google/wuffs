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

// Package builtin lists Wuffs' built-in concepts such as status codes.
package builtin

import (
	t "github.com/google/wuffs/lang/token"
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
	// TODO: sort these somehow, when the list has stabilized?
	{0, "ok"},
	{t.IDError, "bad wuffs version"},
	{t.IDError, "bad receiver"},
	{t.IDError, "bad argument"},
	{t.IDError, "initializer not called"},
	{t.IDError, "invalid I/O operation"},
	{t.IDError, "closed for writes"}, // TODO: is this unused? Should callee or caller check closed-ness?
	{t.IDError, "unexpected EOF"},    // Used if reading when closed == true.
	{t.IDSuspension, "short read"},   // Used if reading when closed == false.
	{t.IDSuspension, "short write"},
	{t.IDError, "cannot return a suspension"},
	{t.IDError, "invalid call sequence"},
}

var StatusMap = map[string]Status{}

func init() {
	for _, s := range StatusList {
		StatusMap[s.Message] = s
	}
}

// TODO: a collection of forbidden variable names like and, or, not, as, ref,
// deref, false, true, in, out, this, u8, u16, etc?

var Types = []string{
	// TODO: i8, i16, i32, i64.
	"u8",
	"u16",
	"u32",
	"u64",
	"bool",
	"status",
	"reader1",
	"writer1",
	"image_config",
}

var Funcs = []string{
	// TODO: some methods like "mark" should probably have a trailing "!".
	//
	// TODO: [] u8 methods (length, etc).

	"u8.high_bits(n u32)(ret u8)",
	"u8.low_bits(n u32)(ret u8)",
	"u16.high_bits(n u32)(ret u16)",
	"u16.low_bits(n u32)(ret u16)",
	"u32.high_bits(n u32)(ret u32)",
	"u32.low_bits(n u32)(ret u32)",
	"u64.high_bits(n u32)(ret u64)",
	"u64.low_bits(n u32)(ret u64)",

	"status.is_error()(ret bool)",
	"status.is_ok()(ret bool)",
	"status.is_suspension()(ret bool)",

	"reader1.read_u8?()(ret u8)",
	"reader1.read_u16be?()(ret u16)",
	"reader1.read_u16le?()(ret u16)",
	"reader1.read_u32be?()(ret u32)",
	"reader1.read_u32le?()(ret u32)",
	"reader1.read_u64be?()(ret u64)",
	"reader1.read_u64le?()(ret u64)",

	"reader1.available()(ret u64)",
	"reader1.is_marked()(ret bool)",
	"reader1.limit(l u64)(ret reader1)",
	"reader1.mark()()",
	"reader1.since_mark()(ret[] u8)",
	"reader1.skip32?(n u32)()",
	"reader1.skip64?(n u64)()",
	"reader1.unread_u8?()()",

	"writer1.write_u8?(x u8)()",
	"writer1.write_u16be?(x u16)()",
	"writer1.write_u16le?(x u16)()",
	"writer1.write_u32be?(x u32)()",
	"writer1.write_u32le?(x u32)()",
	"writer1.write_u64be?(x u64)()",
	"writer1.write_u64le?(x u64)()",

	"writer1.available()(ret u64)",
	"writer1.copy_from_history32(distance u32, length u32)(ret u32)",
	"writer1.copy_from_reader32(r reader1, length u32)(ret u32)",
	"writer1.copy_from_slice(s[] u8)(ret u64)",
	"writer1.copy_from_slice32(s[] u8, length u32)(ret u32)",
	"writer1.is_marked()(ret bool)",
	"writer1.limit(l u64)(ret writer1)",
	"writer1.mark()()",
	"writer1.since_mark()(ret[] u8)",
}

const (
	GenericReplaceFrom = t.IDCapitalT
	GenericReplaceTo   = t.IDDiamond
)

var SliceFuncs = []string{
	// The "T" types here are generic placeholders for every "[] etc" slice
	// type. When parsing these strings (e.g. in the lang/check package), "T"
	// will be replaced by the "◊" diamond to denote a generic slice method, to
	// avoid any possible ambiguity with a user-defined, non-generic "T" type.
	"T.copy_from_slice(s T)(ret u64)",
	"T.length()(ret u64)",
	"T.prefix(up_to u64)(ret T)",
	"T.suffix(up_to u64)(ret T)",
}
