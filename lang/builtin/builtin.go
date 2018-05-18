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

	"image_buffer",
	"image_config",
	"io_reader",
	"io_writer",
	"status",
}

var Funcs = []string{
	"u8.high_bits(n u32[..8])(ret u8)",
	"u8.low_bits(n u32[..8])(ret u8)",
	"u8.max(x u8)(ret u8)",
	"u8.min(x u8)(ret u8)",

	"u16.high_bits(n u32[..16])(ret u16)",
	"u16.low_bits(n u32[..16])(ret u16)",
	"u16.max(x u16)(ret u16)",
	"u16.min(x u16)(ret u16)",

	"u32.high_bits(n u32[..32])(ret u32)",
	"u32.low_bits(n u32[..32])(ret u32)",
	"u32.max(x u32)(ret u32)",
	"u32.min(x u32)(ret u32)",

	"u64.high_bits(n u32[..64])(ret u64)",
	"u64.low_bits(n u32[..64])(ret u64)",
	"u64.max(x u64)(ret u64)",
	"u64.min(x u64)(ret u64)",

	// ---- image_buffer

	"image_buffer.plane(p u32[..3])(ret table u8)",

	// ---- image_config

	"image_config.initialize!(pixfmt u32, pixsub u32, width u32, height u32, num_loops u32)()",

	// ---- io_reader

	"io_reader.unread_u8?()()",
	"io_reader.read_u8?()(ret u8)",
	"io_reader.read_u16be?()(ret u16)",
	"io_reader.read_u16le?()(ret u16)",
	"io_reader.read_u32be?()(ret u32)",
	"io_reader.read_u32le?()(ret u32)",
	"io_reader.read_u64be?()(ret u64)",
	"io_reader.read_u64le?()(ret u64)",

	"io_reader.available()(ret u64)",
	"io_reader.set!(s slice u8, closed bool)()",
	"io_reader.set_limit!(l u64)()",
	"io_reader.set_mark!()()",
	"io_reader.since_mark()(ret slice u8)",

	"io_reader.skip32?(n u32)()",
	"io_reader.skip64?(n u64)()",

	// ---- io_writer

	"io_writer.write_u8?(x u8)()",
	"io_writer.write_u16be?(x u16)()",
	"io_writer.write_u16le?(x u16)()",
	"io_writer.write_u32be?(x u32)()",
	"io_writer.write_u32le?(x u32)()",
	"io_writer.write_u64be?(x u64)()",
	"io_writer.write_u64le?(x u64)()",

	"io_writer.available()(ret u64)",
	"io_writer.set!(s slice u8)()",
	"io_writer.set_limit!(l u64)()",
	"io_writer.set_mark!()()",
	"io_writer.since_mark()(ret slice u8)",

	"io_writer.copy_from_history32!(distance u32, length u32)(ret u32)",
	"io_writer.copy_from_reader32!(r io_reader, length u32)(ret u32)",
	"io_writer.copy_from_slice!(s slice u8)(ret u64)",
	"io_writer.copy_from_slice32!(s slice u8, length u32)(ret u32)",

	// ---- status

	"status.is_error()(ret bool)",
	"status.is_ok()(ret bool)",
	"status.is_suspension()(ret bool)",
}

// The "T1" and "T2" types here are placeholders for generic "slice T" or
// "table T" types. After tokenizing (but before parsing) these XxxFunc strings
// (e.g. in the lang/check package), replace "T1" and "T2" with "†" or "‡"
// daggers, to avoid collision with a user-defined "T1" or "T2" type.

const (
	GenericOldName1 = t.IDT1
	GenericOldName2 = t.IDT2
	GenericNewName1 = t.IDDagger1
	GenericNewName2 = t.IDDagger2
)

var SliceFuncs = []string{
	"T1.copy_from_slice(s T1)(ret u64)",
	"T1.length()(ret u64)",
	"T1.prefix(up_to u64)(ret T1)",
	"T1.suffix(up_to u64)(ret T1)",
}

var TableFuncs = []string{
	"T2.height()(ret u64)",
	"T2.stride()(ret u64)",
	"T2.width()(ret u64)",

	"T2.linearize()(ret T1)",
}
