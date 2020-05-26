// After editing this file, run "go generate" in the parent directory.

// Copyright 2020 The Wuffs Authors.
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

// ---------------- String Conversions

// wuffs_base__parse_number_i64 parses the ASCII integer in s. For example, if
// s contains the bytes "-123" then it will return the int64_t -123.
//
// It returns an error if s does not contain an integer or if the integer
// within would overflow an int64_t.
//
// It is similar to wuffs_base__parse_number_u64 but it returns a signed
// integer, not an unsigned integer. It also allows a leading '+' or '-'.
WUFFS_BASE__MAYBE_STATIC wuffs_base__result_i64  //
wuffs_base__parse_number_i64(wuffs_base__slice_u8 s);

// wuffs_base__parse_number_u64 parses the ASCII integer in s. For example, if
// s contains the bytes "123" then it will return the uint64_t 123.
//
// It returns an error if s does not contain an integer or if the integer
// within would overflow a uint64_t.
//
// It is similar to the C standard library's strtoull function, but:
//  - Errors are returned in-band (in a result type), not out-of-band (errno).
//  - It takes a slice (a pointer and length), not a NUL-terminated C string.
//  - It does not take an optional endptr argument. It does not allow a partial
//    parse: it returns an error unless all of s is consumed.
//  - It does not allow whitespace, leading or otherwise.
//  - It does not allow a leading '+' or '-'.
//  - It does not allow unnecessary leading zeroes ("0" is valid and its sole
//    zero is necessary). All of "00", "0644" and "007" are invalid.
//  - It does not take a base argument (e.g. base 10 vs base 16). Instead, it
//    always accepts both decimal (e.g "1234", "0d5678") and hexadecimal (e.g.
//    "0x9aBC"). The caller is responsible for prior filtering of e.g. hex
//    numbers if they are unwanted. For example, Wuffs' JSON decoder will only
//    produce a wuffs_base__token for decimal numbers, not hexadecimal.
//  - It is not affected by i18n / l10n settings such as environment variables.
//  - It does allow arbitrary underscores, except inside the optional 2-byte
//    opening "0d" or "0X" that denotes base-10 or base-16. For example,
//    "__0D_1_002" would successfully parse as "one thousand and two".
WUFFS_BASE__MAYBE_STATIC wuffs_base__result_u64  //
wuffs_base__parse_number_u64(wuffs_base__slice_u8 s);

// ---------------- IEEE 754 Floating Point

// wuffs_base__parse_number_f64 parses the floating point number in s. For
// example, if s contains the bytes "1.5" then it will return the double 1.5.
//
// It returns an error if s does not contain a floating point number.
//
// It does not necessarily return an error if the conversion is lossy, e.g. if
// s is "0.3", which double-precision floating point cannot represent exactly.
//
// Similarly, the returned value may be infinite (and no error returned) even
// if s was not "inf", when the input is nominally finite but sufficiently
// larger than DBL_MAX, about 1.8e+308.
//
// It is similar to the C standard library's strtod function, but:
//  - Errors are returned in-band (in a result type), not out-of-band (errno).
//  - It takes a slice (a pointer and length), not a NUL-terminated C string.
//  - It does not take an optional endptr argument. It does not allow a partial
//    parse: it returns an error unless all of s is consumed.
//  - It does not allow whitespace, leading or otherwise.
//  - It does not allow unnecessary leading zeroes ("0" is valid and its sole
//    zero is necessary). All of "00", "0644" and "00.7" are invalid.
//  - It is not affected by i18n / l10n settings such as environment variables.
//  - Conversely, it always accepts either ',' or '.' as a decimal separator.
//    In particular, "3,141,592" is always invalid but "3,141" is always valid
//    (and approximately π). The caller is responsible for e.g. previously
//    rejecting or filtering s if it contains a comma, if that is unacceptable
//    to the caller. For example, JSON numbers always use a dot '.' and never a
//    comma ',', regardless of the LOCALE environment variable.
//  - It does allow arbitrary underscores. For example, "_3.141_592" would
//    successfully parse, again approximately π.
//  - It does allow "inf", "+Infinity" and "-NAN", case insensitive, but it
//    does not permit "nan" to be followed by an integer mantissa.
//  - It does not allow hexadecimal floating point numbers.
//
// For modular builds that divide the base module into sub-modules, using this
// function requires the WUFFS_CONFIG__MODULE__BASE__F64CONV sub-module, not
// just WUFFS_CONFIG__MODULE__BASE__CORE.
WUFFS_BASE__MAYBE_STATIC wuffs_base__result_f64  //
wuffs_base__parse_number_f64(wuffs_base__slice_u8 s);

// wuffs_base__ieee_754_bit_representation__etc converts between a double
// precision numerical value and its IEEE 754 64-bit representation (1 sign
// bit, 11 exponent bits, 52 explicit significand bits).
//
// For example, it converts between:
//  - +1.0 and 0x3FF0_0000_0000_0000.
//  - +5.5 and 0x4016_0000_0000_0000.
//  - -inf and 0xFFF0_0000_0000_0000.
//
// See https://en.wikipedia.org/wiki/Double-precision_floating-point_format

static inline uint64_t  //
wuffs_base__ieee_754_bit_representation__from_f64(double f) {
  uint64_t u = 0;
  if (sizeof(uint64_t) == sizeof(double)) {
    memcpy(&u, &f, sizeof(uint64_t));
  }
  return u;
}

static inline double  //
wuffs_base__ieee_754_bit_representation__to_f64(uint64_t u) {
  double f = 0;
  if (sizeof(uint64_t) == sizeof(double)) {
    memcpy(&f, &u, sizeof(uint64_t));
  }
  return f;
}

// ---------------- Hexadecimal

// wuffs_base__hexadecimal__decode2 converts "6A6b" to "jk", where e.g. 'j' is
// U+006A. There are 2 source bytes for every destination byte.
//
// It returns the number of dst bytes written: the minimum of dst.len and
// (src.len / 2). Excess source bytes are ignored.
//
// It assumes that the src bytes are two hexadecimal digits (0-9, A-F, a-f),
// repeated. It may write nonsense bytes if not, although it will not read or
// write out of bounds.
WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__hexadecimal__decode2(wuffs_base__slice_u8 dst,
                                 wuffs_base__slice_u8 src);

// wuffs_base__hexadecimal__decode4 converts "\\x6A\\x6b" to "jk", where e.g.
// 'j' is U+006A. There are 4 source bytes for every destination byte.
//
// It returns the number of dst bytes written: the minimum of dst.len and
// (src.len / 4). Excess source bytes are ignored.
//
// It assumes that the src bytes are two ignored bytes and then two hexadecimal
// digits (0-9, A-F, a-f), repeated. It may write nonsense bytes if not,
// although it will not read or write out of bounds.
WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__hexadecimal__decode4(wuffs_base__slice_u8 dst,
                                 wuffs_base__slice_u8 src);

// ---------------- Unicode and UTF-8

#define WUFFS_BASE__UNICODE_CODE_POINT__MIN_INCL 0x00000000
#define WUFFS_BASE__UNICODE_CODE_POINT__MAX_INCL 0x0010FFFF

#define WUFFS_BASE__UNICODE_REPLACEMENT_CHARACTER 0x0000FFFD

#define WUFFS_BASE__UNICODE_SURROGATE__MIN_INCL 0x0000D800
#define WUFFS_BASE__UNICODE_SURROGATE__MAX_INCL 0x0000DFFF

#define WUFFS_BASE__ASCII__MIN_INCL 0x00
#define WUFFS_BASE__ASCII__MAX_INCL 0x7F

#define WUFFS_BASE__UTF_8__BYTE_LENGTH__MIN_INCL 1
#define WUFFS_BASE__UTF_8__BYTE_LENGTH__MAX_INCL 4

#define WUFFS_BASE__UTF_8__BYTE_LENGTH_1__CODE_POINT__MIN_INCL 0x00000000
#define WUFFS_BASE__UTF_8__BYTE_LENGTH_1__CODE_POINT__MAX_INCL 0x0000007F
#define WUFFS_BASE__UTF_8__BYTE_LENGTH_2__CODE_POINT__MIN_INCL 0x00000080
#define WUFFS_BASE__UTF_8__BYTE_LENGTH_2__CODE_POINT__MAX_INCL 0x000007FF
#define WUFFS_BASE__UTF_8__BYTE_LENGTH_3__CODE_POINT__MIN_INCL 0x00000800
#define WUFFS_BASE__UTF_8__BYTE_LENGTH_3__CODE_POINT__MAX_INCL 0x0000FFFF
#define WUFFS_BASE__UTF_8__BYTE_LENGTH_4__CODE_POINT__MIN_INCL 0x00010000
#define WUFFS_BASE__UTF_8__BYTE_LENGTH_4__CODE_POINT__MAX_INCL 0x0010FFFF

// --------

// wuffs_base__utf_8__next__output is the type returned by
// wuffs_base__utf_8__next.
typedef struct {
  uint32_t code_point;
  uint32_t byte_length;

#ifdef __cplusplus
  inline bool is_valid() const;
#endif  // __cplusplus

} wuffs_base__utf_8__next__output;

static inline wuffs_base__utf_8__next__output  //
wuffs_base__make_utf_8__next__output(uint32_t code_point,
                                     uint32_t byte_length) {
  wuffs_base__utf_8__next__output ret;
  ret.code_point = code_point;
  ret.byte_length = byte_length;
  return ret;
}

static inline bool  //
wuffs_base__utf_8__next__output__is_valid(
    const wuffs_base__utf_8__next__output* o) {
  if (o) {
    uint32_t cp = o->code_point;
    switch (o->byte_length) {
      case 1:
        return (cp <= 0x7F);
      case 2:
        return (0x080 <= cp) && (cp <= 0x7FF);
      case 3:
        // Avoid the 0xD800 ..= 0xDFFF surrogate range.
        return ((0x0800 <= cp) && (cp <= 0xD7FF)) ||
               ((0xE000 <= cp) && (cp <= 0xFFFF));
      case 4:
        return (0x00010000 <= cp) && (cp <= 0x0010FFFF);
    }
  }
  return false;
}

#ifdef __cplusplus

inline bool  //
wuffs_base__utf_8__next__output::is_valid() const {
  return wuffs_base__utf_8__next__output__is_valid(this);
}

#endif  // __cplusplus

// --------

// wuffs_base__utf_8__encode writes the UTF-8 encoding of code_point to s and
// returns the number of bytes written. If code_point is invalid, or if s is
// shorter than the entire encoding, it returns 0 (and no bytes are written).
//
// s will never be too short if its length is at least 4, also known as
// WUFFS_BASE__UTF_8__BYTE_LENGTH__MAX_INCL.
WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__utf_8__encode(wuffs_base__slice_u8 dst, uint32_t code_point);

// wuffs_base__utf_8__next returns the next UTF-8 code point (and that code
// point's byte length) at the start of s.
//
// There are exactly two cases in which this function returns something where
// wuffs_base__utf_8__next__output__is_valid is false:
//  - If s is empty then it returns {.code_point=0, .byte_length=0}.
//  - If s is non-empty and starts with invalid UTF-8 then it returns
//    {.code_point=WUFFS_BASE__UNICODE_REPLACEMENT_CHARACTER, .byte_length=1}.
//
// Otherwise, it returns something where
// wuffs_base__utf_8__next__output__is_valid is true.
//
// In any case, it always returns an output that satisfies both of:
//  - (output.code_point  <= WUFFS_BASE__UNICODE_CODE_POINT__MAX_INCL).
//  - (output.byte_length <= s.len).
//
// If s is a sub-slice of a larger slice of valid UTF-8, but that sub-slice
// boundary occurs in the middle of a multi-byte UTF-8 encoding of a single
// code point, then this function may return something invalid. It is the
// caller's responsibility to split on or otherwise manage UTF-8 boundaries.
WUFFS_BASE__MAYBE_STATIC wuffs_base__utf_8__next__output  //
wuffs_base__utf_8__next(wuffs_base__slice_u8 s);

// wuffs_base__utf_8__longest_valid_prefix returns the largest n such that the
// sub-slice s[..n] is valid UTF-8.
//
// In particular, it returns s.len if and only if all of s is valid UTF-8.
//
// If s is a sub-slice of a larger slice of valid UTF-8, but that sub-slice
// boundary occurs in the middle of a multi-byte UTF-8 encoding of a single
// code point, then this function will return less than s.len. It is the
// caller's responsibility to split on or otherwise manage UTF-8 boundaries.
WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__utf_8__longest_valid_prefix(wuffs_base__slice_u8 s);

// wuffs_base__ascii__longest_valid_prefix returns the largest n such that the
// sub-slice s[..n] is valid ASCII.
//
// In particular, it returns s.len if and only if all of s is valid ASCII.
// Equivalently, when none of the bytes in s have the 0x80 high bit set.
WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__ascii__longest_valid_prefix(wuffs_base__slice_u8 s);
