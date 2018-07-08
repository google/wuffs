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
	Value   int8
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
	{0, 0x00, "ok"},

	{t.IDError, -0x01, "bad wuffs version"},
	{t.IDError, -0x02, "bad sizeof receiver"},
	{t.IDError, -0x03, "bad receiver"},
	{t.IDError, -0x04, "bad argument"},
	{t.IDError, -0x04, "bad argument (length too short)"},

	{t.IDError, -0x10, "check_wuffs_version not called"},
	{t.IDError, -0x11, "check_wuffs_version called twice"},
	{t.IDError, -0x12, "invalid call sequence"},

	{t.IDError, -0x20, "cannot return a suspension"},

	{t.IDError, -0x30, "unexpected EOF"},    // Used if reading when closed == true.
	{t.IDError, -0x31, "closed for writes"}, // TODO: is this unused? Should callee or caller check closed-ness?

	{t.IDSuspension, +0x01, "end of data"},
	{t.IDSuspension, +0x02, "short read"}, // Used if reading when closed == false.
	{t.IDSuspension, +0x03, "short write"},
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
	"utility",

	"range_ii_u32",
	"range_ie_u32",
	"range_ii_u64",
	"range_ie_u64",
	"rect_ie_u32",
	"rect_ii_u32",

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

	// ---- utility

	"utility.make_range_ii_u32(min_incl u32, max_incl u32)(ret range_ii_u32)",
	"utility.make_range_ie_u32(min_incl u32, max_excl u32)(ret range_ie_u32)",
	"utility.make_range_ii_u64(min_incl u64, max_incl u64)(ret range_ii_u64)",
	"utility.make_range_ie_u64(min_incl u64, max_excl u64)(ret range_ie_u64)",
	"utility.make_rect_ii_u32(min_incl_x u32, min_incl_y u32, max_incl_x u32, max_incl_y u32)(ret rect_ii_u32)",
	"utility.make_rect_ie_u32(min_incl_x u32, min_incl_y u32, max_excl_x u32, max_excl_y u32)(ret rect_ie_u32)",

	// ---- image_buffer

	"image_buffer.plane(p u32[..3])(ret table u8)",
	// Duration's upper bound is the maximum possible i64 value.
	"image_buffer.update!(dirty_rect rect_ie_u32, duration u64[..0x7FFFFFFFFFFFFFFF], " +
		"blend bool, disposal u8, palette slice u8)()",

	// ---- image_config

	"image_config.initialize!(pixfmt u32, pixsub u32, width u32, height u32, " +
		"num_loops u32, first_frame_is_opaque bool)()",

	// ---- io_reader

	"io_reader.can_undo_byte()(ret bool)",
	"io_reader.undo_byte!()()",

	"io_reader.read_u8?()(ret u8)",
	"io_reader.read_u16be?()(ret u16)",
	"io_reader.read_u16le?()(ret u16)",
	"io_reader.read_u24be?()(ret u32[..0xFFFFFF])",
	"io_reader.read_u24le?()(ret u32[..0xFFFFFF])",
	"io_reader.read_u32be?()(ret u32)",
	"io_reader.read_u32le?()(ret u32)",
	"io_reader.read_u40be?()(ret u64[..0xFFFFFFFFFF])",
	"io_reader.read_u40le?()(ret u64[..0xFFFFFFFFFF])",
	"io_reader.read_u48be?()(ret u64[..0xFFFFFFFFFFFF])",
	"io_reader.read_u48le?()(ret u64[..0xFFFFFFFFFFFF])",
	"io_reader.read_u56be?()(ret u64[..0xFFFFFFFFFFFFFF])",
	"io_reader.read_u56le?()(ret u64[..0xFFFFFFFFFFFFFF])",
	"io_reader.read_u64be?()(ret u64)",
	"io_reader.read_u64le?()(ret u64)",

	// TODO: these should have an explicit pre-condition "available() >= N".
	// For now, that's implicitly checked (i.e. hard coded).
	//
	// The io_reader has peek_etc methods and skip_fast, not read_etc_fast,
	// because we sometimes advance the pointer by less than what's read. See
	// https://fgiesen.wordpress.com/2018/02/20/reading-bits-in-far-too-many-ways-part-2/
	"io_reader.peek_u8()(ret u8)",
	"io_reader.peek_u16be()(ret u16)",
	"io_reader.peek_u16le()(ret u16)",
	"io_reader.peek_u24be()(ret u32[..0xFFFFFF])",
	"io_reader.peek_u24le()(ret u32[..0xFFFFFF])",
	"io_reader.peek_u32be()(ret u32)",
	"io_reader.peek_u32le()(ret u32)",
	"io_reader.peek_u40be()(ret u64[..0xFFFFFFFFFF])",
	"io_reader.peek_u40le()(ret u64[..0xFFFFFFFFFF])",
	"io_reader.peek_u48be()(ret u64[..0xFFFFFFFFFFFF])",
	"io_reader.peek_u48le()(ret u64[..0xFFFFFFFFFFFF])",
	"io_reader.peek_u56be()(ret u64[..0xFFFFFFFFFFFFFF])",
	"io_reader.peek_u56le()(ret u64[..0xFFFFFFFFFFFFFF])",
	"io_reader.peek_u64be()(ret u64)",
	"io_reader.peek_u64le()(ret u64)",

	"io_reader.available()(ret u64)",
	"io_reader.set!(s slice u8, closed bool)()",
	"io_reader.set_limit!(l u64)()",
	"io_reader.set_mark!()()",
	"io_reader.since_mark()(ret slice u8)",

	"io_reader.skip?(n u32)()",

	// TODO: this should have explicit pre-conditions "actual <= worst_case"
	// and "worst_case <= available()". As an implementation restriction, we
	// also require that worst_case has a constant value. For now, that's all
	// implicitly checked (i.e. hard coded).
	"io_reader.skip_fast!(actual u32, worst_case u32)()",

	// ---- io_writer

	"io_writer.write_u8?(x u8)()",
	"io_writer.write_u16be?(x u16)()",
	"io_writer.write_u16le?(x u16)()",
	"io_writer.write_u24be?(x u32[..0xFFFFFF])()",
	"io_writer.write_u24le?(x u32[..0xFFFFFF])()",
	"io_writer.write_u32be?(x u32)()",
	"io_writer.write_u32le?(x u32)()",
	"io_writer.write_u40be?(x u64[..0xFFFFFFFFFF])()",
	"io_writer.write_u40le?(x u64[..0xFFFFFFFFFF])()",
	"io_writer.write_u48be?(x u64[..0xFFFFFFFFFFFF])()",
	"io_writer.write_u48le?(x u64[..0xFFFFFFFFFFFF])()",
	"io_writer.write_u56be?(x u64[..0xFFFFFFFFFFFFFF])()",
	"io_writer.write_u56le?(x u64[..0xFFFFFFFFFFFFFF])()",
	"io_writer.write_u64be?(x u64)()",
	"io_writer.write_u64le?(x u64)()",

	// TODO: these should have an explicit pre-condition "available() >= N".
	// For now, that's implicitly checked (i.e. hard coded).
	//
	// The io_writer has write_fast_etc methods, not poke_etc and skip_fast,
	// because skip_fast could leave uninitialized bytes in the io_buffer.
	"io_writer.write_fast_u8!(x u8)()",
	"io_writer.write_fast_u16be!(x u16)()",
	"io_writer.write_fast_u16le!(x u16)()",
	"io_writer.write_fast_u24be!(x u32[..0xFFFFFF])()",
	"io_writer.write_fast_u24le!(x u32[..0xFFFFFF])()",
	"io_writer.write_fast_u32be!(x u32)()",
	"io_writer.write_fast_u32le!(x u32)()",
	"io_writer.write_fast_u40be!(x u64[..0xFFFFFFFFFF])()",
	"io_writer.write_fast_u40le!(x u64[..0xFFFFFFFFFF])()",
	"io_writer.write_fast_u48be!(x u64[..0xFFFFFFFFFFFF])()",
	"io_writer.write_fast_u48le!(x u64[..0xFFFFFFFFFFFF])()",
	"io_writer.write_fast_u56be!(x u64[..0xFFFFFFFFFFFFFF])()",
	"io_writer.write_fast_u56le!(x u64[..0xFFFFFFFFFFFFFF])()",
	"io_writer.write_fast_u64be!(x u64)()",
	"io_writer.write_fast_u64le!(x u64)()",

	"io_writer.available()(ret u64)",
	"io_writer.set!(s slice u8)()",
	"io_writer.set_limit!(l u64)()",
	"io_writer.set_mark!()()",
	"io_writer.since_mark()(ret slice u8)",

	"io_writer.copy_from_slice!(s slice u8)(ret u64)",
	"io_writer.copy_n_from_history!(n u32, distance u32)(ret u32)",
	"io_writer.copy_n_from_reader!(n u32, r io_reader)(ret u32)",
	"io_writer.copy_n_from_slice!(n u32, s slice u8)(ret u32)",

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
	// TODO: should copy_from_slice be a ! method?
	"T1.copy_from_slice(s T1)(ret u64)",
	"T1.length()(ret u64)",
	"T1.prefix(up_to u64)(ret T1)",
	"T1.suffix(up_to u64)(ret T1)",
}

var TableFuncs = []string{
	"T2.height()(ret u64)",
	"T2.stride()(ret u64)",
	"T2.width()(ret u64)",

	"T2.row(y u32)(ret T1)",
}
