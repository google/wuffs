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
	{t.IDSuspension, "end of data"},
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

	"empty_struct",
	"bool",
	"status",
	"io_reader",
	"io_writer",
	"image_config",
}

var Funcs = []string{
	// TODO: some methods like "mark" and "set_limit" should probably have a
	// trailing "!".

	// TODO: refine n's type from u32 to u32[..8], u32[..16], etc.
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

	"io_reader.read_u8?()(ret u8)",
	"io_reader.read_u16be?()(ret u16)",
	"io_reader.read_u16le?()(ret u16)",
	"io_reader.read_u32be?()(ret u32)",
	"io_reader.read_u32le?()(ret u32)",
	"io_reader.read_u64be?()(ret u64)",
	"io_reader.read_u64le?()(ret u64)",

	"io_reader.available()(ret u64)",
	"io_reader.limit(l u64)(ret io_reader)",
	"io_reader.set_limit(l u64)()",
	"io_reader.mark()()",
	"io_reader.since_mark()(ret slice u8)",
	"io_reader.skip32?(n u32)()",
	"io_reader.skip64?(n u64)()",
	"io_reader.unread_u8?()()",

	"io_writer.write_u8?(x u8)()",
	"io_writer.write_u16be?(x u16)()",
	"io_writer.write_u16le?(x u16)()",
	"io_writer.write_u32be?(x u32)()",
	"io_writer.write_u32le?(x u32)()",
	"io_writer.write_u64be?(x u64)()",
	"io_writer.write_u64le?(x u64)()",

	"io_writer.available()(ret u64)",
	"io_writer.copy_from_history32(distance u32, length u32)(ret u32)",
	"io_writer.copy_from_reader32(r io_reader, length u32)(ret u32)",
	"io_writer.copy_from_slice(s slice u8)(ret u64)",
	"io_writer.copy_from_slice32(s slice u8, length u32)(ret u32)",
	"io_writer.limit(l u64)(ret io_writer)",
	"io_writer.set_limit(l u64)()",
	"io_writer.mark()()",
	"io_writer.since_mark()(ret slice u8)",

	"image_config.initialize!(pixfmt u32, pixsub u32, width u32, height u32, num_loops u32)()",
}

// The "T" types here are placeholders for generic "slice U" types. After
// tokenizing (but before parsing) these SliceFunc strings (e.g. in the
// lang/check package), replace the "T" receiver type with the "◊" diamond, to
// avoid collision with a user-defined "T" type.

const (
	PlaceholderOldName = t.IDCapitalT
	PlaceholderNewName = t.IDDiamond
)

var SliceFuncs = []string{
	"T.copy_from_slice(s T)(ret u64)",
	"T.length()(ret u64)",
	"T.prefix(up_to u64)(ret T)",
	"T.suffix(up_to u64)(ret T)",
}
