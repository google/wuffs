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
	"fmt"
	"strings"

	"github.com/google/wuffs/lang/parse"

	a "github.com/google/wuffs/lang/ast"
	t "github.com/google/wuffs/lang/token"
)

var FourCCs = [...][2]string{
	{"ICCP", "International Color Consortium Profile"},
	{"JPEG", "Joint Photographic Experts Group"},
	{"PNG ", "Portable Network Graphics"},
	{"XMP ", "Extensible Metadata Platform"},
}

var Consts = [...]struct {
	Type  t.ID
	Value string
	Name  string
}{
	// ----

	{t.IDU32, "0x02000008", "PIXEL_FORMAT__A"},

	{t.IDU32, "0x20000008", "PIXEL_FORMAT__Y"},
	{t.IDU32, "0x21000008", "PIXEL_FORMAT__YA_NONPREMUL"},
	{t.IDU32, "0x22000008", "PIXEL_FORMAT__YA_PREMUL"},

	{t.IDU32, "0x40020888", "PIXEL_FORMAT__YCBCR"},
	{t.IDU32, "0x41038888", "PIXEL_FORMAT__YCBCRA_NONPREMUL"},
	{t.IDU32, "0x50038888", "PIXEL_FORMAT__YCBCRK"},

	{t.IDU32, "0x60020888", "PIXEL_FORMAT__YCOCG"},
	{t.IDU32, "0x61038888", "PIXEL_FORMAT__YCOCGA_NONPREMUL"},
	{t.IDU32, "0x70038888", "PIXEL_FORMAT__YCOCGK"},

	{t.IDU32, "0x81040008", "PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL"},
	{t.IDU32, "0x82040008", "PIXEL_FORMAT__INDEXED__BGRA_PREMUL"},
	{t.IDU32, "0x83040008", "PIXEL_FORMAT__INDEXED__BGRA_BINARY"},

	{t.IDU32, "0x80000565", "PIXEL_FORMAT__BGR_565"},
	{t.IDU32, "0x80000888", "PIXEL_FORMAT__BGR"},
	{t.IDU32, "0x81008888", "PIXEL_FORMAT__BGRA_NONPREMUL"},
	{t.IDU32, "0x82008888", "PIXEL_FORMAT__BGRA_PREMUL"},
	{t.IDU32, "0x83008888", "PIXEL_FORMAT__BGRA_BINARY"},
	{t.IDU32, "0x90008888", "PIXEL_FORMAT__BGRX"},

	{t.IDU32, "0xA0000888", "PIXEL_FORMAT__RGB"},
	{t.IDU32, "0xA1008888", "PIXEL_FORMAT__RGBA_NONPREMUL"},
	{t.IDU32, "0xA2008888", "PIXEL_FORMAT__RGBA_PREMUL"},
	{t.IDU32, "0xA3008888", "PIXEL_FORMAT__RGBA_BINARY"},
	{t.IDU32, "0xB0008888", "PIXEL_FORMAT__RGBX"},

	{t.IDU32, "0xC0020888", "PIXEL_FORMAT__CMY"},
	{t.IDU32, "0xD0038888", "PIXEL_FORMAT__CMYK"},

	// ----

	{t.IDU32, "0", "TOKEN__VBC__FILLER"},
	{t.IDU32, "1", "TOKEN__VBC__STRUCTURE"},
	{t.IDU32, "2", "TOKEN__VBC__STRING"},
	{t.IDU32, "3", "TOKEN__VBC__UNICODE_CODE_POINT"},
	{t.IDU32, "4", "TOKEN__VBC__LITERAL"},
	{t.IDU32, "5", "TOKEN__VBC__NUMBER"},

	// ----

	{t.IDU32, "0x00001", "TOKEN__VBD__FILLER__COMMENT_LINE"},
	{t.IDU32, "0x00002", "TOKEN__VBD__FILLER__COMMENT_BLOCK"},

	// ----

	{t.IDU32, "0x00001", "TOKEN__VBD__STRUCTURE__PUSH"},
	{t.IDU32, "0x00002", "TOKEN__VBD__STRUCTURE__POP"},
	{t.IDU32, "0x00010", "TOKEN__VBD__STRUCTURE__FROM_NONE"},
	{t.IDU32, "0x00020", "TOKEN__VBD__STRUCTURE__FROM_LIST"},
	{t.IDU32, "0x00040", "TOKEN__VBD__STRUCTURE__FROM_DICT"},
	{t.IDU32, "0x01000", "TOKEN__VBD__STRUCTURE__TO_NONE"},
	{t.IDU32, "0x02000", "TOKEN__VBD__STRUCTURE__TO_LIST"},
	{t.IDU32, "0x04000", "TOKEN__VBD__STRUCTURE__TO_DICT"},

	// ----

	{t.IDU32, "0x00001", "TOKEN__VBD__STRING__DEFINITELY_UTF_8"},
	{t.IDU32, "0x00002", "TOKEN__VBD__STRING__DEFINITELY_ASCII"},

	{t.IDU32, "0x00010", "TOKEN__VBD__STRING__CONVERT_0_DST_1_SRC_DROP"},
	{t.IDU32, "0x00020", "TOKEN__VBD__STRING__CONVERT_1_DST_1_SRC_COPY"},
	{t.IDU32, "0x00040", "TOKEN__VBD__STRING__CONVERT_1_DST_2_SRC_HEXADECIMAL"},
	{t.IDU32, "0x00080", "TOKEN__VBD__STRING__CONVERT_1_DST_4_SRC_BACKSLASH_X"},
	{t.IDU32, "0x00100", "TOKEN__VBD__STRING__CONVERT_3_DST_4_SRC_BASE_64_STD"},
	{t.IDU32, "0x00200", "TOKEN__VBD__STRING__CONVERT_3_DST_4_SRC_BASE_64_URL"},
	{t.IDU32, "0x00400", "TOKEN__VBD__STRING__CONVERT_4_DST_5_SRC_ASCII_85"},

	// ----

	{t.IDU32, "0x00001", "TOKEN__VBD__LITERAL__UNDEFINED"},
	{t.IDU32, "0x00002", "TOKEN__VBD__LITERAL__NULL"},
	{t.IDU32, "0x00004", "TOKEN__VBD__LITERAL__FALSE"},
	{t.IDU32, "0x00008", "TOKEN__VBD__LITERAL__TRUE"},

	// ----

	{t.IDU32, "0x00001", "TOKEN__VBD__NUMBER__CONTENT_FLOATING_POINT"},
	{t.IDU32, "0x00002", "TOKEN__VBD__NUMBER__CONTENT_INTEGER_SIGNED"},
	{t.IDU32, "0x00004", "TOKEN__VBD__NUMBER__CONTENT_INTEGER_UNSIGNED"},

	{t.IDU32, "0x00010", "TOKEN__VBD__NUMBER__CONTENT_NEG_INF"},
	{t.IDU32, "0x00020", "TOKEN__VBD__NUMBER__CONTENT_POS_INF"},
	{t.IDU32, "0x00040", "TOKEN__VBD__NUMBER__CONTENT_NEG_NAN"},
	{t.IDU32, "0x00080", "TOKEN__VBD__NUMBER__CONTENT_POS_NAN"},

	{t.IDU32, "0x00100", "TOKEN__VBD__NUMBER__FORMAT_BINARY_BIG_ENDIAN"},
	{t.IDU32, "0x00200", "TOKEN__VBD__NUMBER__FORMAT_BINARY_LITTLE_ENDIAN"},
	{t.IDU32, "0x00400", "TOKEN__VBD__NUMBER__FORMAT_TEXT"},

	// ----

	{t.IDU32, "0xFFFD", "UNICODE__REPLACEMENT_CHARACTER"},
}

var Statuses = [...]string{
	// Notes.
	`"@I/O redirect"`,
	`"@end of data"`,
	`"@metadata reported"`,

	// Suspensions.
	`"$even more information"`,
	`"$mispositioned read"`,
	`"$mispositioned write"`,
	`"$short read"`,
	`"$short write"`,

	// Errors.
	`"#bad I/O position"`,
	`"#bad argument (length too short)"`,
	`"#bad argument"`,
	`"#bad call sequence"`,
	`"#bad receiver"`,
	`"#bad restart"`,
	`"#bad sizeof receiver"`,
	`"#bad vtable"`,
	`"#bad workbuf length"`,
	`"#bad wuffs version"`,
	`"#cannot return a suspension"`,
	`"#disabled by previous error"`,
	`"#initialize falsely claimed already zeroed"`,
	`"#initialize not called"`,
	`"#interleaved coroutine calls"`,
	`"#no more information"`,
	`"#not enough data"`,
	`"#out of bounds"`,
	`"#unsupported method"`,
	`"#unsupported option"`,
	`"#unsupported pixel swizzler option"`,
	`"#too much data"`,
}

// TODO: a collection of forbidden variable names like and, or, not, as, false,
// true, in, out, this, u8, u16, etc?

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

	"more_information",

	"status",

	"io_reader",
	"io_writer",
	"token_reader",
	"token_writer",

	"frame_config",
	"image_config",
	"pixel_blend",
	"pixel_buffer",
	"pixel_config",
	"pixel_format",
	"pixel_swizzler",

	"decode_frame_options",
}

var Funcs = []string{
	"u8.high_bits(n: u32[..= 8]) u8",
	"u8.low_bits(n: u32[..= 8]) u8",
	"u8.max(a: u8) u8",
	"u8.min(a: u8) u8",

	"u16.high_bits(n: u32[..= 16]) u16",
	"u16.low_bits(n: u32[..= 16]) u16",
	"u16.max(a: u16) u16",
	"u16.min(a: u16) u16",

	"u32.high_bits(n: u32[..= 32]) u32",
	"u32.low_bits(n: u32[..= 32]) u32",
	"u32.max(a: u32) u32",
	"u32.min(a: u32) u32",

	"u64.high_bits(n: u32[..= 64]) u64",
	"u64.low_bits(n: u32[..= 64]) u64",
	"u64.max(a: u64) u64",
	"u64.min(a: u64) u64",

	// ---- utility

	"utility.empty_io_reader() io_reader",
	"utility.empty_io_writer() io_writer",
	"utility.empty_range_ii_u32() range_ii_u32",
	"utility.empty_range_ie_u32() range_ie_u32",
	"utility.empty_range_ii_u64() range_ii_u64",
	"utility.empty_range_ie_u64() range_ie_u64",
	"utility.empty_rect_ii_u32() rect_ii_u32",
	"utility.empty_rect_ie_u32() rect_ie_u32",
	"utility.empty_slice_u8() slice u8",
	"utility.make_pixel_format(repr: u32) pixel_format",
	"utility.make_range_ii_u32(min_incl: u32, max_incl: u32) range_ii_u32",
	"utility.make_range_ie_u32(min_incl: u32, max_excl: u32) range_ie_u32",
	"utility.make_range_ii_u64(min_incl: u64, max_incl: u64) range_ii_u64",
	"utility.make_range_ie_u64(min_incl: u64, max_excl: u64) range_ie_u64",
	"utility.make_rect_ii_u32(" +
		"min_incl_x: u32, min_incl_y: u32, max_incl_x: u32, max_incl_y: u32) rect_ii_u32",
	"utility.make_rect_ie_u32(" +
		"min_incl_x: u32, min_incl_y: u32, max_excl_x: u32, max_excl_y: u32) rect_ie_u32",

	// ---- ranges

	"range_ie_u32.reset!()",
	"range_ie_u32.get_min_incl() u32",
	"range_ie_u32.get_max_excl() u32",
	"range_ie_u32.intersect(r: range_ie_u32) range_ie_u32",
	"range_ie_u32.unite(r: range_ie_u32) range_ie_u32",

	"range_ii_u32.reset!()",
	"range_ii_u32.get_min_incl() u32",
	"range_ii_u32.get_max_incl() u32",
	"range_ii_u32.intersect(r: range_ii_u32) range_ii_u32",
	"range_ii_u32.unite(r: range_ii_u32) range_ii_u32",

	"range_ie_u64.reset!()",
	"range_ie_u64.get_min_incl() u64",
	"range_ie_u64.get_max_excl() u64",
	"range_ie_u64.intersect(r: range_ie_u64) range_ie_u64",
	"range_ie_u64.unite(r: range_ie_u64) range_ie_u64",

	"range_ii_u64.reset!()",
	"range_ii_u64.get_min_incl() u64",
	"range_ii_u64.get_max_incl() u64",
	"range_ii_u64.intersect(r: range_ii_u64) range_ii_u64",
	"range_ii_u64.unite(r: range_ii_u64) range_ii_u64",

	// ---- more_information

	"more_information.set!(flavor: u32, w: u32, x: u64, y: u64, z: u64)",

	// ---- status

	// TODO: should we add is_complete?
	"status.is_error() bool",
	"status.is_note() bool",
	"status.is_ok() bool",
	"status.is_suspension() bool",

	// ---- io_reader

	"io_reader.can_undo_byte() bool",
	"io_reader.undo_byte!()",

	"io_reader.read_u8?() u8",

	"io_reader.read_u16be?() u16",
	"io_reader.read_u16le?() u16",

	"io_reader.read_u8_as_u32?() u32[..= 0xFF]",
	"io_reader.read_u16be_as_u32?() u32[..= 0xFFFF]",
	"io_reader.read_u16le_as_u32?() u32[..= 0xFFFF]",
	"io_reader.read_u24be_as_u32?() u32[..= 0xFF_FFFF]",
	"io_reader.read_u24le_as_u32?() u32[..= 0xFF_FFFF]",
	"io_reader.read_u32be?() u32",
	"io_reader.read_u32le?() u32",

	"io_reader.read_u8_as_u64?() u64[..= 0xFF]",
	"io_reader.read_u16be_as_u64?() u64[..= 0xFFFF]",
	"io_reader.read_u16le_as_u64?() u64[..= 0xFFFF]",
	"io_reader.read_u24be_as_u64?() u64[..= 0xFF_FFFF]",
	"io_reader.read_u24le_as_u64?() u64[..= 0xFF_FFFF]",
	"io_reader.read_u32be_as_u64?() u64[..= 0xFFFF_FFFF]",
	"io_reader.read_u32le_as_u64?() u64[..= 0xFFFF_FFFF]",
	"io_reader.read_u40be_as_u64?() u64[..= 0xFF_FFFF_FFFF]",
	"io_reader.read_u40le_as_u64?() u64[..= 0xFF_FFFF_FFFF]",
	"io_reader.read_u48be_as_u64?() u64[..= 0xFFFF_FFFF_FFFF]",
	"io_reader.read_u48le_as_u64?() u64[..= 0xFFFF_FFFF_FFFF]",
	"io_reader.read_u56be_as_u64?() u64[..= 0xFF_FFFF_FFFF_FFFF]",
	"io_reader.read_u56le_as_u64?() u64[..= 0xFF_FFFF_FFFF_FFFF]",
	"io_reader.read_u64be?() u64",
	"io_reader.read_u64le?() u64",

	// TODO: these should have an explicit pre-condition "available() >= N".
	// For now, that's implicitly checked (i.e. hard coded).
	//
	// io_reader has peek_etc methods and skip_u32_fast, not read_etc_fast,
	// because we sometimes advance the pointer by less than what's read. See
	// https://fgiesen.wordpress.com/2018/02/20/reading-bits-in-far-too-many-ways-part-2/

	"io_reader.peek_u8() u8",

	"io_reader.peek_u16be() u16",
	"io_reader.peek_u16le() u16",

	"io_reader.peek_u8_as_u32() u32[..= 0xFF]",
	"io_reader.peek_u16be_as_u32() u32[..= 0xFFFF]",
	"io_reader.peek_u16le_as_u32() u32[..= 0xFFFF]",
	"io_reader.peek_u24be_as_u32() u32[..= 0xFF_FFFF]",
	"io_reader.peek_u24le_as_u32() u32[..= 0xFF_FFFF]",
	"io_reader.peek_u32be() u32",
	"io_reader.peek_u32le() u32",

	"io_reader.peek_u8_as_u64() u64[..= 0xFF]",
	"io_reader.peek_u16be_as_u64() u64[..= 0xFFFF]",
	"io_reader.peek_u16le_as_u64() u64[..= 0xFFFF]",
	"io_reader.peek_u24be_as_u64() u64[..= 0xFF_FFFF]",
	"io_reader.peek_u24le_as_u64() u64[..= 0xFF_FFFF]",
	"io_reader.peek_u32be_as_u64() u64[..= 0xFFFF_FFFF]",
	"io_reader.peek_u32le_as_u64() u64[..= 0xFFFF_FFFF]",
	"io_reader.peek_u40be_as_u64() u64[..= 0xFF_FFFF_FFFF]",
	"io_reader.peek_u40le_as_u64() u64[..= 0xFF_FFFF_FFFF]",
	"io_reader.peek_u48be_as_u64() u64[..= 0xFFFF_FFFF_FFFF]",
	"io_reader.peek_u48le_as_u64() u64[..= 0xFFFF_FFFF_FFFF]",
	"io_reader.peek_u56be_as_u64() u64[..= 0xFF_FFFF_FFFF_FFFF]",
	"io_reader.peek_u56le_as_u64() u64[..= 0xFF_FFFF_FFFF_FFFF]",
	"io_reader.peek_u64be() u64",
	"io_reader.peek_u64le() u64",

	// As an implementation restriction, we require that offset has a constant
	// value. The (0x1_0000 - sizeof(u64)) limit is arbitrary, but high enough
	// in practice.
	"io_reader.peek_u64le_at(offset: u32[..= 0xFFF8]) u64",

	"io_reader.available() u64",
	"io_reader.count_since(mark: u64) u64",
	"io_reader.is_closed() bool",
	"io_reader.mark() u64",
	"io_reader.match7(a: u64) u32[..= 2]",
	"io_reader.position() u64",
	"io_reader.since(mark: u64) slice u8",

	"io_reader.limited_copy_u32_to_slice!(up_to: u32, s: slice u8) u32",

	"io_reader.skip?(n: u64)",
	"io_reader.skip_u32?(n: u32)",

	// TODO: this should have explicit pre-conditions "actual <= worst_case"
	// and "worst_case <= available()". As an implementation restriction, we
	// also require that worst_case has a constant value. For now, that's all
	// implicitly checked (i.e. hard coded).
	"io_reader.skip_u32_fast!(actual: u32, worst_case: u32)",

	// ---- io_writer

	"io_writer.write_u8?(a: u8)",
	"io_writer.write_u16be?(a: u16)",
	"io_writer.write_u16le?(a: u16)",
	"io_writer.write_u24be?(a: u32[..= 0xFF_FFFF])",
	"io_writer.write_u24le?(a: u32[..= 0xFF_FFFF])",
	"io_writer.write_u32be?(a: u32)",
	"io_writer.write_u32le?(a: u32)",
	"io_writer.write_u40be?(a: u64[..= 0xFF_FFFF_FFFF])",
	"io_writer.write_u40le?(a: u64[..= 0xFF_FFFF_FFFF])",
	"io_writer.write_u48be?(a: u64[..= 0xFFFF_FFFF_FFFF])",
	"io_writer.write_u48le?(a: u64[..= 0xFFFF_FFFF_FFFF])",
	"io_writer.write_u56be?(a: u64[..= 0xFF_FFFF_FFFF_FFFF])",
	"io_writer.write_u56le?(a: u64[..= 0xFF_FFFF_FFFF_FFFF])",
	"io_writer.write_u64be?(a: u64)",
	"io_writer.write_u64le?(a: u64)",

	// TODO: these should have an explicit pre-condition "available() >= N".
	// For now, that's implicitly checked (i.e. hard coded).
	//
	// io_writer has write_etc_fast methods, not poke_etc and skip_u32_fast,
	// because skip_u32_fast could leave uninitialized bytes in the io_buffer.
	"io_writer.write_u8_fast!(a: u8)",
	"io_writer.write_u16be_fast!(a: u16)",
	"io_writer.write_u16le_fast!(a: u16)",
	"io_writer.write_u24be_fast!(a: u32[..= 0xFF_FFFF])",
	"io_writer.write_u24le_fast!(a: u32[..= 0xFF_FFFF])",
	"io_writer.write_u32be_fast!(a: u32)",
	"io_writer.write_u32le_fast!(a: u32)",
	"io_writer.write_u40be_fast!(a: u64[..= 0xFF_FFFF_FFFF])",
	"io_writer.write_u40le_fast!(a: u64[..= 0xFF_FFFF_FFFF])",
	"io_writer.write_u48be_fast!(a: u64[..= 0xFFFF_FFFF_FFFF])",
	"io_writer.write_u48le_fast!(a: u64[..= 0xFFFF_FFFF_FFFF])",
	"io_writer.write_u56be_fast!(a: u64[..= 0xFF_FFFF_FFFF_FFFF])",
	"io_writer.write_u56le_fast!(a: u64[..= 0xFF_FFFF_FFFF_FFFF])",
	"io_writer.write_u64be_fast!(a: u64)",
	"io_writer.write_u64le_fast!(a: u64)",

	"io_writer.available() u64",
	"io_writer.count_since(mark: u64) u64",
	"io_writer.history_available() u64",
	"io_writer.mark() u64",
	"io_writer.position() u64",
	"io_writer.since(mark: u64) slice u8",

	"io_writer.copy_from_slice!(s: slice u8) u64",
	"io_writer.limited_copy_u32_from_history!(up_to: u32, distance: u32) u32",
	"io_writer.limited_copy_u32_from_reader!(up_to: u32, r: io_reader) u32",
	"io_writer.limited_copy_u32_from_slice!(up_to: u32, s: slice u8) u32",

	// TODO: this should have explicit pre-conditions:
	//  - up_to <= this.available()
	//  - distance > 0
	//  - distance <= this.since_mark().length()
	// For now, that's all implicitly checked (i.e. hard coded).
	"io_writer.limited_copy_u32_from_history_fast!(up_to: u32, distance: u32) u32",

	// ---- token_writer

	"token_writer.write_simple_token_fast!(" +
		"value_major: u32[..= 0x1F_FFFF], value_minor: u32[..= 0x1FF_FFFF]," +
		"continued: u32[..= 0x1], length: u32[..= 0xFFFF])",

	"token_writer.available() u64",

	// ---- frame_config

	"frame_config.blend() u8",
	"frame_config.disposal() u8",
	"frame_config.duration() u64[..= 0x7FFF_FFFF_FFFF_FFFF]",
	"frame_config.index() u64",
	"frame_config.io_position() u64",

	"frame_config.set!(bounds: rect_ie_u32, duration: u64[..= 0x7FFF_FFFF_FFFF_FFFF]," +
		"index: u64, io_position: u64, disposal: u8, opaque_within_bounds: bool," +
		"overwrite_instead_of_blend: bool, background_color: u32)",

	// ---- image_config

	"image_config.set!(pixfmt: u32, pixsub: u32, width: u32, height: u32," +
		"first_frame_io_position: u64, first_frame_is_opaque: bool)",

	// ---- pixel_buffer

	"pixel_buffer.palette() slice u8",
	"pixel_buffer.pixel_format() pixel_format",
	"pixel_buffer.plane(p: u32[..= 3]) table u8",

	// ---- pixel_format

	"pixel_format.bits_per_pixel() u32[..= 256]",

	// ---- pixel_swizzler

	"pixel_swizzler.prepare!(" +
		"dst_pixfmt: pixel_format, dst_palette: slice u8," +
		"src_pixfmt: pixel_format, src_palette: slice u8, blend: pixel_blend) status",
	"pixel_swizzler.swizzle_interleaved_from_reader!(" +
		"dst: slice u8, dst_palette: slice u8, src: io_reader) u64",
	"pixel_swizzler.swizzle_interleaved_from_slice!(" +
		"dst: slice u8, dst_palette: slice u8, src: slice u8) u64",
}

var Interfaces = []string{
	"hasher_u32",
	"image_decoder",
	"io_transformer",
	"token_decoder",
}

var InterfacesMap = map[string]bool{
	"hasher_u32":     true,
	"image_decoder":  true,
	"io_transformer": true,
	"token_decoder":  true,
}

var InterfaceFuncs = []string{
	// ---- hasher_u32

	"hasher_u32.set_quirk_enabled!(quirk: u32, enabled: bool)",
	"hasher_u32.update_u32!(x: slice u8) u32",

	// ---- image_decoder

	"image_decoder.decode_frame?(" +
		"dst: ptr pixel_buffer, src: io_reader, blend: pixel_blend," +
		"workbuf: slice u8, opts: nptr decode_frame_options)",
	"image_decoder.decode_frame_config?(dst: nptr frame_config, src: io_reader)",
	"image_decoder.decode_image_config?(dst: nptr image_config, src: io_reader)",
	"image_decoder.frame_dirty_rect() rect_ie_u32",
	"image_decoder.num_animation_loops() u32",
	"image_decoder.num_decoded_frame_configs() u64",
	"image_decoder.num_decoded_frames() u64",
	"image_decoder.restart_frame!(index: u64, io_position: u64) status",
	"image_decoder.set_quirk_enabled!(quirk: u32, enabled: bool)",
	"image_decoder.set_report_metadata!(fourcc: u32, report: bool)",
	"image_decoder.tell_me_more?(dst: io_writer, minfo: nptr more_information, src: io_reader)",
	"image_decoder.workbuf_len() range_ii_u64",

	// ---- io_transformer

	"io_transformer.set_quirk_enabled!(quirk: u32, enabled: bool)",
	"io_transformer.transform_io?(dst: io_writer, src: io_reader, workbuf: slice u8)",
	"io_transformer.workbuf_len() range_ii_u64",

	// ---- token_decoder

	"token_decoder.decode_tokens?(dst: token_writer, src: io_reader, workbuf: slice u8)",
	"token_decoder.set_quirk_enabled!(quirk: u32, enabled: bool)",
	"token_decoder.workbuf_len() range_ii_u64",
}

// The "T1" and "T2" types here are placeholders for generic "slice T" or
// "table T" types. After tokenizing (but before parsing) these XxxFunc strings
// (e.g. in the lang/check package), replace "T1" and "T2" with "†" or "‡"
// daggers, to avoid collision with a user-defined "T1" or "T2" type.

const (
	genericOldName1 = t.IDT1
	genericOldName2 = t.IDT2
	genericNewName1 = t.IDDagger1
	genericNewName2 = t.IDDagger2
)

var SliceFuncs = []string{
	"GENERIC T1.copy_from_slice!(s: T1) u64",
	"GENERIC T1.length() u64",
	"GENERIC T1.prefix(up_to: u64) T1",
	"GENERIC T1.suffix(up_to: u64) T1",
}

var TableFuncs = []string{
	"GENERIC T2.height() u64",
	"GENERIC T2.stride() u64",
	"GENERIC T2.width() u64",

	"GENERIC T2.row(y: u32) T1",
}

func ParseFuncs(tm *t.Map, ss []string, callback func(*a.Func) error) error {
	const GENERIC = "GENERIC "
	buf := []byte(nil)
	for _, s := range ss {
		generic := strings.HasPrefix(s, GENERIC)
		if generic {
			s = s[len(GENERIC):]
		}

		buf = buf[:0]
		buf = append(buf, "pub func "...)
		buf = append(buf, s...)
		buf = append(buf, "{}\n"...)

		const filename = "builtin.wuffs"
		tokens, _, err := t.Tokenize(tm, filename, buf)
		if err != nil {
			return fmt.Errorf("parsing %q: could not tokenize built-in funcs: %v", s, err)
		}
		if generic {
			for i := range tokens {
				if id := tokens[i].ID; id == genericOldName1 {
					tokens[i].ID = genericNewName1
				} else if id == genericOldName2 {
					tokens[i].ID = genericNewName2
				}
			}
		}
		file, err := parse.Parse(tm, filename, tokens, &parse.Options{
			AllowBuiltInNames: true,
		})
		if err != nil {
			return fmt.Errorf("parsing %q: could not parse built-in funcs: %v", s, err)
		}

		tlds := file.TopLevelDecls()
		if len(tlds) != 1 || tlds[0].Kind() != a.KFunc {
			return fmt.Errorf("parsing %q: got %d top level decls, want %d", s, len(tlds), 1)
		}
		f := tlds[0].AsFunc()
		f.AsNode().AsRaw().SetPackage(tm, t.IDBase)

		if callback != nil {
			if err := callback(f); err != nil {
				return err
			}
		}
	}
	return nil
}
