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

//go:generate go run gen.go

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
	{"BMP ", "Bitmap"},
	{"BRTL", "Brotli"},
	{"BZ2 ", "Bzip2"},
	{"CBOR", "Concise Binary Object Representation"},
	{"CSS ", "Cascading Style Sheets"},
	{"EPS ", "Encapsulated PostScript"},
	{"FLAC", "Free Lossless Audio Codec"},
	{"GIF ", "Graphics Interchange Format"},
	{"GZ  ", "GNU Zip"},
	{"HEIF", "High Efficiency Image File"},
	{"HTML", "Hypertext Markup Language"},
	{"ICCP", "International Color Consortium Profile"},
	{"ICO ", "Icon"},
	{"ICVG", "Icon Vector Graphics"},
	{"INI ", "Initialization"},
	{"JPEG", "Joint Photographic Experts Group"},
	{"JS  ", "JavaScript"},
	{"JSON", "JavaScript Object Notation"},
	{"JWCC", "JSON With Commas and Comments"},
	{"LZ4 ", "Lempel–Ziv 4"},
	{"MD  ", "Markdown"},
	{"MP3 ", "MPEG-1 Audio Layer III"},
	{"NIE ", "Naive Image"},
	{"OTF ", "Open Type Format"},
	{"PDF ", "Portable Document Format"},
	{"PNG ", "Portable Network Graphics"},
	{"PNM ", "Portable Anymap"},
	{"PS  ", "PostScript"},
	{"RAC ", "Random Access Compression"},
	{"RAW ", "Raw"},
	{"RIFF", "Resource Interchange File Format"},
	{"RIGL", "Riegeli Records"},
	{"SNPY", "Snappy"},
	{"SVG ", "Scalable Vector Graphics"},
	{"TAR ", "Tape Archive"},
	{"TIFF", "Tagged Image File Format"},
	{"TOML", "Tom's Obvious Minimal Language"},
	{"WAVE", "Waveform"},
	{"WBMP", "Wireless Bitmap"},
	{"WOFF", "Web Open Font Format"},
	{"WP8 ", "Web Picture (VP8)"},
	{"WP8L", "Web Picture (VP8 Lossless)"},
	{"XML ", "Extensible Markup Language"},
	{"XMP ", "Extensible Metadata Platform"},
	{"XZ  ", "Xz"},
	{"ZIP ", "Zip"},
	{"ZLIB", "Zlib"},
	{"ZSTD", "Zstandard"},
}

var Consts = [...]struct {
	Type  t.ID
	Value string
	Name  string
}{
	// ----

	{t.IDU32, "0x02000008", "PIXEL_FORMAT__A"},

	{t.IDU32, "0x20000008", "PIXEL_FORMAT__Y"},
	{t.IDU32, "0x2000000B", "PIXEL_FORMAT__Y_16LE"},
	{t.IDU32, "0x2010000B", "PIXEL_FORMAT__Y_16BE"},
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
	{t.IDU32, "0x8100BBBB", "PIXEL_FORMAT__BGRA_NONPREMUL_4X16LE"},
	{t.IDU32, "0x82008888", "PIXEL_FORMAT__BGRA_PREMUL"},
	{t.IDU32, "0x8200BBBB", "PIXEL_FORMAT__BGRA_PREMUL_4X16LE"},
	{t.IDU32, "0x83008888", "PIXEL_FORMAT__BGRA_BINARY"},
	{t.IDU32, "0x90008888", "PIXEL_FORMAT__BGRX"},

	{t.IDU32, "0xA0000888", "PIXEL_FORMAT__RGB"},
	{t.IDU32, "0xA1008888", "PIXEL_FORMAT__RGBA_NONPREMUL"},
	{t.IDU32, "0xA100BBBB", "PIXEL_FORMAT__RGBA_NONPREMUL_4X16LE"},
	{t.IDU32, "0xA2008888", "PIXEL_FORMAT__RGBA_PREMUL"},
	{t.IDU32, "0xA200BBBB", "PIXEL_FORMAT__RGBA_PREMUL_4X16LE"},
	{t.IDU32, "0xA3008888", "PIXEL_FORMAT__RGBA_BINARY"},
	{t.IDU32, "0xB0008888", "PIXEL_FORMAT__RGBX"},

	{t.IDU32, "0xC0020888", "PIXEL_FORMAT__CMY"},
	{t.IDU32, "0xD0038888", "PIXEL_FORMAT__CMYK"},

	// ----

	{t.IDU32, "1", "QUIRK_IGNORE_CHECKSUM"},

	// ----

	{t.IDU32, "46", "TOKEN__VALUE_EXTENSION__NUM_BITS"},

	// ----

	{t.IDU32, "0", "TOKEN__VBC__FILLER"},
	{t.IDU32, "1", "TOKEN__VBC__STRUCTURE"},
	{t.IDU32, "2", "TOKEN__VBC__STRING"},
	{t.IDU32, "3", "TOKEN__VBC__UNICODE_CODE_POINT"},
	{t.IDU32, "4", "TOKEN__VBC__LITERAL"},
	{t.IDU32, "5", "TOKEN__VBC__NUMBER"},
	{t.IDU32, "6", "TOKEN__VBC__INLINE_INTEGER_SIGNED"},
	{t.IDU32, "7", "TOKEN__VBC__INLINE_INTEGER_UNSIGNED"},

	// ----

	{t.IDU32, "0x00001", "TOKEN__VBD__FILLER__PUNCTUATION"},
	{t.IDU32, "0x00002", "TOKEN__VBD__FILLER__COMMENT_BLOCK"},
	{t.IDU32, "0x00004", "TOKEN__VBD__FILLER__COMMENT_LINE"},
	{t.IDU32, "0x00006", "TOKEN__VBD__FILLER__COMMENT_ANY"},

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
	{t.IDU32, "0x00002", "TOKEN__VBD__STRING__CHAIN_MUST_BE_UTF_8"},
	{t.IDU32, "0x00004", "TOKEN__VBD__STRING__CHAIN_SHOULD_BE_UTF_8"},
	{t.IDU32, "0x00010", "TOKEN__VBD__STRING__DEFINITELY_ASCII"},
	{t.IDU32, "0x00020", "TOKEN__VBD__STRING__CHAIN_MUST_BE_ASCII"},
	{t.IDU32, "0x00040", "TOKEN__VBD__STRING__CHAIN_SHOULD_BE_ASCII"},

	{t.IDU32, "0x00100", "TOKEN__VBD__STRING__CONVERT_0_DST_1_SRC_DROP"},
	{t.IDU32, "0x00200", "TOKEN__VBD__STRING__CONVERT_1_DST_1_SRC_COPY"},
	{t.IDU32, "0x00400", "TOKEN__VBD__STRING__CONVERT_1_DST_2_SRC_HEXADECIMAL"},
	{t.IDU32, "0x00800", "TOKEN__VBD__STRING__CONVERT_1_DST_4_SRC_BACKSLASH_X"},
	{t.IDU32, "0x01000", "TOKEN__VBD__STRING__CONVERT_3_DST_4_SRC_BASE_64_STD"},
	{t.IDU32, "0x02000", "TOKEN__VBD__STRING__CONVERT_3_DST_4_SRC_BASE_64_URL"},
	{t.IDU32, "0x04000", "TOKEN__VBD__STRING__CONVERT_4_DST_5_SRC_ASCII_85"},
	{t.IDU32, "0x08000", "TOKEN__VBD__STRING__CONVERT_5_DST_8_SRC_BASE_32_HEX"},
	{t.IDU32, "0x10000", "TOKEN__VBD__STRING__CONVERT_5_DST_8_SRC_BASE_32_STD"},

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

	{t.IDU32, "0x01000", "TOKEN__VBD__NUMBER__FORMAT_IGNORE_FIRST_BYTE"},

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
	`"#bad data"`,
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

	// ----

	"arm_crc32_utility",
	"arm_crc32_u32",

	"arm_neon_utility",
	"arm_neon_u8x8",
	"arm_neon_u16x4",
	"arm_neon_u32x2",
	"arm_neon_u64x1",
	"arm_neon_u8x16",
	"arm_neon_u16x8",
	"arm_neon_u32x4",
	"arm_neon_u64x2",

	"x86_sse42_utility",
	"x86_m128i",
}

var Funcs = [][]string{
	funcsOther[:],   // Manually authored methods.
	funcsARMNeon[:], // Automatically generated methods (intrinsics).
}

var funcsOther = [...]string{
	"u8.high_bits(n: u32[..= 7]) u8",
	"u8.low_bits(n: u32[..= 7]) u8",
	"u8.max(a: u8) u8",
	"u8.min(a: u8) u8",

	"u16.high_bits(n: u32[..= 15]) u16",
	"u16.low_bits(n: u32[..= 15]) u16",
	"u16.max(a: u16) u16",
	"u16.min(a: u16) u16",

	"u32.high_bits(n: u32[..= 31]) u32",
	"u32.low_bits(n: u32[..= 31]) u32",
	"u32.max(a: u32) u32",
	"u32.min(a: u32) u32",

	"u64.high_bits(n: u32[..= 63]) u64",
	"u64.low_bits(n: u32[..= 63]) u64",
	"u64.max(a: u64) u64",
	"u64.min(a: u64) u64",

	// ---- utility

	"utility.cpu_arch_is_32_bit() bool",
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

	"range_ie_u32.get_min_incl() u32",
	"range_ie_u32.get_max_excl() u32",
	"range_ie_u32.intersect(r: range_ie_u32) range_ie_u32",
	"range_ie_u32.unite(r: range_ie_u32) range_ie_u32",

	"range_ii_u32.get_min_incl() u32",
	"range_ii_u32.get_max_incl() u32",
	"range_ii_u32.intersect(r: range_ii_u32) range_ii_u32",
	"range_ii_u32.unite(r: range_ii_u32) range_ii_u32",

	"range_ie_u64.get_min_incl() u64",
	"range_ie_u64.get_max_excl() u64",
	"range_ie_u64.intersect(r: range_ie_u64) range_ie_u64",
	"range_ie_u64.unite(r: range_ie_u64) range_ie_u64",

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

	// TODO: these should have an explicit pre-condition "length() >= N". For
	// now, that's implicitly checked (i.e. hard coded).
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

	"io_reader.count_since(mark: u64) u64",
	"io_reader.is_closed() bool",
	"io_reader.length() u64",
	"io_reader.mark() u64",
	"io_reader.match7(a: u64) u32[..= 2]",
	"io_reader.position() u64",
	"io_reader.since(mark: u64) slice u8",
	"io_reader.valid_utf_8_length(up_to: u64) u64",

	"io_reader.limited_copy_u32_to_slice!(up_to: u32, s: slice u8) u32",

	"io_reader.skip?(n: u64)",
	"io_reader.skip_u32?(n: u32)",

	// TODO: this should have explicit pre-conditions "actual <= worst_case"
	// and "worst_case <= length()". For now, that's all implicitly checked
	// (i.e. hard coded).
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

	// TODO: these should have an explicit pre-condition "length() >= N". For
	// now, that's implicitly checked (i.e. hard coded).
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

	"io_writer.count_since(mark: u64) u64",
	"io_writer.history_length() u64",
	"io_writer.length() u64",
	"io_writer.mark() u64",
	"io_writer.position() u64",
	"io_writer.since(mark: u64) slice u8",

	"io_writer.copy_from_slice!(s: slice u8) u64",
	"io_writer.limited_copy_u32_from_history!(up_to: u32, distance: u32) u32",
	"io_writer.limited_copy_u32_from_reader!(up_to: u32, r: io_reader) u32",
	"io_writer.limited_copy_u32_from_slice!(up_to: u32, s: slice u8) u32",

	// TODO: this should have explicit pre-conditions:
	//  - up_to <= this.length()
	//  - distance >= 1
	//  - distance <= this.history_length()
	// For now, that's all implicitly checked (i.e. hard coded).
	"io_writer.limited_copy_u32_from_history_fast!(up_to: u32, distance: u32) u32",

	// TODO: this should have explicit pre-conditions:
	//  - (up_to + 8) <= this.length()
	//  - distance >= 8
	//  - distance <= this.history_length()
	// For now, that's all implicitly checked (i.e. hard coded).
	"io_writer.limited_copy_u32_from_history_8_byte_chunks_fast!(up_to: u32, distance: u32) u32",

	// ---- token_writer

	"token_writer.write_simple_token_fast!(" +
		"value_major: u32[..= 0x1F_FFFF], value_minor: u32[..= 0x1FF_FFFF]," +
		"continued: u32[..= 0x1], length: u32[..= 0xFFFF])",
	"token_writer.write_extended_token_fast!(" +
		"value_extension: u64[..= 0x3FFF_FFFF_FFFF]," +
		"continued: u32[..= 0x1], length: u32[..= 0xFFFF])",

	"token_writer.length() u64",

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
	"pixel_buffer.palette_or_else(fallback: slice u8) slice u8",
	"pixel_buffer.pixel_format() pixel_format",
	"pixel_buffer.plane(p: u32[..= 3]) table u8",

	// ---- pixel_format

	"pixel_format.bits_per_pixel() u32[..= 256]",

	// ---- pixel_swizzler

	"pixel_swizzler.prepare!(" +
		"dst_pixfmt: pixel_format, dst_palette: slice u8," +
		"src_pixfmt: pixel_format, src_palette: slice u8, blend: pixel_blend) status",

	"pixel_swizzler.limited_swizzle_u32_interleaved_from_reader!(" +
		"up_to_num_pixels: u32, dst: slice u8, dst_palette: slice u8, src: io_reader) u64",
	"pixel_swizzler.swizzle_interleaved_from_reader!(" +
		"dst: slice u8, dst_palette: slice u8, src: io_reader) u64",
	"pixel_swizzler.swizzle_interleaved_from_slice!(" +
		"dst: slice u8, dst_palette: slice u8, src: slice u8) u64",
	"pixel_swizzler.swizzle_interleaved_transparent_black!(" +
		"dst: slice u8, dst_palette: slice u8, num_pixels: u64) u64",

	// ---- arm_crc32_utility

	"arm_crc32_utility.make_u32(a: u32) arm_crc32_u32",

	// ---- arm_crc32_u32

	"arm_crc32_u32.crc32b(b: u8) arm_crc32_u32",
	"arm_crc32_u32.crc32h(b: u16) arm_crc32_u32",
	"arm_crc32_u32.crc32w(b: u32) arm_crc32_u32",
	"arm_crc32_u32.crc32d(b: u64) arm_crc32_u32",
	"arm_crc32_u32.value() u32",

	// ---- arm_neon_utility

	"arm_neon_utility.make_u8x8_multiple(" +
		"a00: u8, a01: u8, a02: u8, a03: u8," +
		"a04: u8, a05: u8, a06: u8, a07: u8) arm_neon_u8x8",
	"arm_neon_utility.make_u16x4_multiple(" +
		"a00: u16, a01: u16, a02: u16, a03: u16) arm_neon_u16x4",
	"arm_neon_utility.make_u32x2_multiple(" +
		"a00: u32, a01: u32) arm_neon_u32x2",
	"arm_neon_utility.make_u64x1_multiple(" +
		"a00: u64) arm_neon_u64x1",

	"arm_neon_utility.make_u8x16_multiple(" +
		"a00: u8, a01: u8, a02: u8, a03: u8," +
		"a04: u8, a05: u8, a06: u8, a07: u8," +
		"a08: u8, a09: u8, a10: u8, a11: u8," +
		"a12: u8, a13: u8, a14: u8, a15: u8) arm_neon_u8x16",
	"arm_neon_utility.make_u16x8_multiple(" +
		"a00: u16, a01: u16, a02: u16, a03: u16," +
		"a04: u16, a05: u16, a06: u16, a07: u16) arm_neon_u16x8",
	"arm_neon_utility.make_u32x4_multiple(" +
		"a00: u32, a01: u32, a02: u32, a03: u32) arm_neon_u32x4",
	"arm_neon_utility.make_u64x2_multiple(" +
		"a00: u64, a01: u64) arm_neon_u64x2",

	"arm_neon_utility.make_u8x8_repeat(a: u8) arm_neon_u8x8",
	"arm_neon_utility.make_u16x4_repeat(a: u16) arm_neon_u16x4",
	"arm_neon_utility.make_u32x2_repeat(a: u32) arm_neon_u32x2",
	"arm_neon_utility.make_u64x1_repeat(a: u64) arm_neon_u64x1",

	"arm_neon_utility.make_u8x16_repeat(a: u8) arm_neon_u8x16",
	"arm_neon_utility.make_u16x8_repeat(a: u16) arm_neon_u16x8",
	"arm_neon_utility.make_u32x4_repeat(a: u32) arm_neon_u32x4",
	"arm_neon_utility.make_u64x2_repeat(a: u64) arm_neon_u64x2",

	"arm_neon_utility.make_u8x8_slice64(a: slice base.u8) arm_neon_u8x8",
	"arm_neon_utility.make_u8x16_slice128(a: slice base.u8) arm_neon_u8x16",

	// ---- arm_neon_uAxB.as_uCxD

	"arm_neon_u8x8.as_u16x4() arm_neon_u16x4",
	"arm_neon_u8x8.as_u32x2() arm_neon_u32x2",
	"arm_neon_u8x8.as_u64x1() arm_neon_u64x1",

	"arm_neon_u16x4.as_u8x8() arm_neon_u8x8",
	"arm_neon_u32x2.as_u8x8() arm_neon_u8x8",
	"arm_neon_u64x1.as_u8x8() arm_neon_u8x8",

	"arm_neon_u16x8.as_u16x8() arm_neon_u16x8",
	"arm_neon_u16x8.as_u32x4() arm_neon_u32x4",
	"arm_neon_u16x8.as_u64x2() arm_neon_u64x2",

	"arm_neon_u16x8.as_u8x16() arm_neon_u8x16",
	"arm_neon_u32x4.as_u8x16() arm_neon_u8x16",
	"arm_neon_u64x2.as_u8x16() arm_neon_u8x16",

	// ---- x86_sse42_utility

	"x86_sse42_utility.make_m128i_multiple_u8(" +
		"a00: u8, a01: u8, a02: u8, a03: u8," +
		"a04: u8, a05: u8, a06: u8, a07: u8," +
		"a08: u8, a09: u8, a10: u8, a11: u8," +
		"a12: u8, a13: u8, a14: u8, a15: u8) x86_m128i",
	"x86_sse42_utility.make_m128i_multiple_u16(" +
		"a00: u16, a01: u16, a02: u16, a03: u16," +
		"a04: u16, a05: u16, a06: u16, a07: u16) x86_m128i",
	"x86_sse42_utility.make_m128i_multiple_u32(" +
		"a00: u32, a01: u32, a02: u32, a03: u32) x86_m128i",
	"x86_sse42_utility.make_m128i_multiple_u64(" +
		"a00: u64, a01: u64) x86_m128i",

	"x86_sse42_utility.make_m128i_repeat_u8(a: u8) x86_m128i",
	"x86_sse42_utility.make_m128i_repeat_u16(a: u16) x86_m128i",
	"x86_sse42_utility.make_m128i_repeat_u32(a: u32) x86_m128i",
	"x86_sse42_utility.make_m128i_repeat_u64(a: u64) x86_m128i",

	"x86_sse42_utility.make_m128i_single_u32(a: u32) x86_m128i",
	"x86_sse42_utility.make_m128i_single_u64(a: u64) x86_m128i",

	"x86_sse42_utility.make_m128i_slice128(a: slice base.u8) x86_m128i",

	"x86_sse42_utility.make_m128i_zeroes() x86_m128i",

	// ---- x86_m128i

	"x86_m128i.store_slice64!(a: slice base.u8)",
	"x86_m128i.store_slice128!(a: slice base.u8)",

	"x86_m128i.truncate_u32() u32",
	"x86_m128i.truncate_u64() u64",

	// TODO: generate these methods automatically?

	"x86_m128i._mm_abs_epi16() x86_m128i",
	"x86_m128i._mm_add_epi16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_add_epi32(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_add_epi64(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_add_epi8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_and_si128(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_avg_epu16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_avg_epu8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_blend_epi16(b: x86_m128i, imm8: u32) x86_m128i",
	"x86_m128i._mm_blendv_epi8(b: x86_m128i, mask: x86_m128i) x86_m128i",
	"x86_m128i._mm_clmulepi64_si128(b: x86_m128i, imm8: u32) x86_m128i",
	"x86_m128i._mm_cmpeq_epi16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_cmpeq_epi32(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_cmpeq_epi64(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_cmpeq_epi8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_extract_epi16(imm8: u32) u16",
	"x86_m128i._mm_extract_epi32(imm8: u32) u32",
	"x86_m128i._mm_extract_epi64(imm8: u32) u64",
	"x86_m128i._mm_extract_epi8(imm8: u32) u8",
	"x86_m128i._mm_madd_epi16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_maddubs_epi16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_max_epi16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_max_epi32(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_max_epi8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_max_epu16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_max_epu32(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_max_epu8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_min_epi16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_min_epi32(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_min_epi8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_min_epu16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_min_epu32(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_min_epu8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_packus_epi16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_sad_epu8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_shuffle_epi32(imm8: u32) x86_m128i",
	"x86_m128i._mm_shuffle_epi8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_slli_epi16(imm8: u32) x86_m128i",
	"x86_m128i._mm_slli_epi32(imm8: u32) x86_m128i",
	"x86_m128i._mm_slli_epi64(imm8: u32) x86_m128i",
	"x86_m128i._mm_slli_si128(imm8: u32) x86_m128i",
	"x86_m128i._mm_srli_epi16(imm8: u32) x86_m128i",
	"x86_m128i._mm_srli_epi32(imm8: u32) x86_m128i",
	"x86_m128i._mm_srli_epi64(imm8: u32) x86_m128i",
	"x86_m128i._mm_srli_si128(imm8: u32) x86_m128i",
	"x86_m128i._mm_sub_epi16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_sub_epi32(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_sub_epi64(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_sub_epi8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_unpackhi_epi16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_unpackhi_epi32(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_unpackhi_epi64(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_unpackhi_epi8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_unpacklo_epi16(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_unpacklo_epi32(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_unpacklo_epi64(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_unpacklo_epi8(b: x86_m128i) x86_m128i",
	"x86_m128i._mm_xor_si128(b: x86_m128i) x86_m128i",
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
	"GENERIC T1.uintptr_low_12_bits() u32[..= 4095]",
	"GENERIC T1.suffix(up_to: u64) T1",
}

var SliceU8Funcs = []string{
	"GENERIC T1.peek_u8() u8",
	"GENERIC T1.peek_u16be() u16",
	"GENERIC T1.peek_u16le() u16",
	"GENERIC T1.peek_u24be_as_u32() u32",
	"GENERIC T1.peek_u24le_as_u32() u32",
	"GENERIC T1.peek_u32be() u32",
	"GENERIC T1.peek_u32le() u32",
	"GENERIC T1.peek_u40be_as_u64() u64",
	"GENERIC T1.peek_u40le_as_u64() u64",
	"GENERIC T1.peek_u48be_as_u64() u64",
	"GENERIC T1.peek_u48le_as_u64() u64",
	"GENERIC T1.peek_u56be_as_u64() u64",
	"GENERIC T1.peek_u56le_as_u64() u64",
	"GENERIC T1.peek_u64be() u64",
	"GENERIC T1.peek_u64le() u64",

	"GENERIC T1.poke_u8!(a: u8)",
	"GENERIC T1.poke_u16be!(a: u16)",
	"GENERIC T1.poke_u16le!(a: u16)",
	"GENERIC T1.poke_u24be!(a: u32)",
	"GENERIC T1.poke_u24le!(a: u32)",
	"GENERIC T1.poke_u32be!(a: u32)",
	"GENERIC T1.poke_u32le!(a: u32)",
	"GENERIC T1.poke_u40be!(a: u64)",
	"GENERIC T1.poke_u40le!(a: u64)",
	"GENERIC T1.poke_u48be!(a: u64)",
	"GENERIC T1.poke_u48le!(a: u64)",
	"GENERIC T1.poke_u56be!(a: u64)",
	"GENERIC T1.poke_u56le!(a: u64)",
	"GENERIC T1.poke_u64be!(a: u64)",
	"GENERIC T1.poke_u64le!(a: u64)",
}

var TableFuncs = []string{
	"GENERIC T2.height() u64",
	"GENERIC T2.stride() u64",
	"GENERIC T2.width() u64",

	"GENERIC T2.row(y: u32) T1",
}

func ParseFuncs(tm *t.Map, ss []string, callback func(*a.Func) error) error {
	if len(ss) == 0 {
		return nil
	}
	const GENERIC = "GENERIC "
	generic := strings.HasPrefix(ss[0], GENERIC)

	buf := []byte(nil)
	for _, s := range ss {
		if generic && (len(s) >= len(GENERIC)) {
			s = s[len(GENERIC):]
		}
		buf = append(buf, "pub func "...)
		buf = append(buf, s...)
		buf = append(buf, "{}\n"...)
	}

	const filename = "builtin.wuffs"
	tokens, _, err := t.Tokenize(tm, filename, buf)
	if err != nil {
		return fmt.Errorf("could not tokenize built-in funcs: %v", err)
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
		return fmt.Errorf("could not parse built-in funcs: %v", err)
	}

	for _, tld := range file.TopLevelDecls() {
		if tld.Kind() != a.KFunc {
			continue
		}
		f := tld.AsFunc()
		f.AsNode().AsRaw().SetPackage(tm, t.IDBase)
		if callback != nil {
			if err := callback(f); err != nil {
				return err
			}
		}
	}
	return nil
}
