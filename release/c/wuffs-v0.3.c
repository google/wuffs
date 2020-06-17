#ifndef WUFFS_INCLUDE_GUARD
#define WUFFS_INCLUDE_GUARD

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.

// Wuffs' C code is generated automatically, not hand-written. These warnings'
// costs outweigh the benefits.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Wunused-function"
#endif

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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Note that Clang also defines __GNUC__.
#ifdef __cplusplus
#if __cplusplus >= 201103L
#include <memory>
#elif defined(__GNUC__)
#warning "Wuffs' C++ code expects -std=c++11 or later"
#elif defined(_MSC_VER)
#pragma message("Wuffs' C++ code expects C++11 or later")
#endif

extern "C" {
#endif

// ---------------- Fundamentals

// WUFFS_VERSION is the major.minor.patch version, as per https://semver.org/,
// as a uint64_t. The major number is the high 32 bits. The minor number is the
// middle 16 bits. The patch number is the low 16 bits. The pre-release label
// and build metadata are part of the string representation (such as
// "1.2.3-beta+456.20181231") but not the uint64_t representation.
//
// WUFFS_VERSION_PRE_RELEASE_LABEL (such as "", "beta" or "rc.1") being
// non-empty denotes a developer preview, not a release version, and has no
// backwards or forwards compatibility guarantees.
//
// WUFFS_VERSION_BUILD_METADATA_XXX, if non-zero, are the number of commits and
// the last commit date in the repository used to build this library. Within
// each major.minor branch, the commit count should increase monotonically.
//
// WUFFS_VERSION was overridden by "wuffs gen -version" based on revision
// d377dcaea826f12b47492cced1b75450a440f6c1 committed on 2020-06-17.
#define WUFFS_VERSION 0x000030000
#define WUFFS_VERSION_MAJOR 0
#define WUFFS_VERSION_MINOR 3
#define WUFFS_VERSION_PATCH 0
#define WUFFS_VERSION_PRE_RELEASE_LABEL "alpha.4"
#define WUFFS_VERSION_BUILD_METADATA_COMMIT_COUNT 2514
#define WUFFS_VERSION_BUILD_METADATA_COMMIT_DATE 20200617
#define WUFFS_VERSION_STRING "0.3.0-alpha.4+2514.20200617"

// Define WUFFS_CONFIG__STATIC_FUNCTIONS to make all of Wuffs' functions have
// static storage. The motivation is discussed in the "ALLOW STATIC
// IMPLEMENTATION" section of
// https://raw.githubusercontent.com/nothings/stb/master/docs/stb_howto.txt
#ifdef WUFFS_CONFIG__STATIC_FUNCTIONS
#define WUFFS_BASE__MAYBE_STATIC static
#else
#define WUFFS_BASE__MAYBE_STATIC
#endif

// --------

// Wuffs assumes that:
//  - converting a uint32_t to a size_t will never overflow.
//  - converting a size_t to a uint64_t will never overflow.
#ifdef __WORDSIZE
#if (__WORDSIZE != 32) && (__WORDSIZE != 64)
#error "Wuffs requires a word size of either 32 or 64 bits"
#endif
#endif

#if defined(__clang__)
#define WUFFS_BASE__POTENTIALLY_UNUSED_FIELD __attribute__((unused))
#else
#define WUFFS_BASE__POTENTIALLY_UNUSED_FIELD
#endif

// Clang also defines "__GNUC__".
#if defined(__GNUC__)
#define WUFFS_BASE__POTENTIALLY_UNUSED __attribute__((unused))
#define WUFFS_BASE__WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WUFFS_BASE__POTENTIALLY_UNUSED
#define WUFFS_BASE__WARN_UNUSED_RESULT
#endif

// Flags for wuffs_foo__bar__initialize functions.

#define WUFFS_INITIALIZE__DEFAULT_OPTIONS ((uint32_t)0x00000000)

// WUFFS_INITIALIZE__ALREADY_ZEROED means that the "self" receiver struct value
// has already been set to all zeroes.
#define WUFFS_INITIALIZE__ALREADY_ZEROED ((uint32_t)0x00000001)

// WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED means that, absent
// WUFFS_INITIALIZE__ALREADY_ZEROED, only some of the "self" receiver struct
// value will be set to all zeroes. Internal buffers, which tend to be a large
// proportion of the struct's size, will be left uninitialized. Internal means
// that the buffer is contained by the receiver struct, as opposed to being
// passed as a separately allocated "work buffer".
//
// For more detail, see:
// https://github.com/google/wuffs/blob/master/doc/note/initialization.md
#define WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED \
  ((uint32_t)0x00000002)

// --------

// wuffs_base__empty_struct is used when a Wuffs function returns an empty
// struct. In C, if a function f returns void, you can't say "x = f()", but in
// Wuffs, if a function g returns empty, you can say "y = g()".
typedef struct {
  // private_impl is a placeholder field. It isn't explicitly used, except that
  // without it, the sizeof a struct with no fields can differ across C/C++
  // compilers, and it is undefined behavior in C99. For example, gcc says that
  // the sizeof an empty struct is 0, and g++ says that it is 1. This leads to
  // ABI incompatibility if a Wuffs .c file is processed by one compiler and
  // its .h file with another compiler.
  //
  // Instead, we explicitly insert an otherwise unused field, so that the
  // sizeof this struct is always 1.
  uint8_t private_impl;
} wuffs_base__empty_struct;

static inline wuffs_base__empty_struct  //
wuffs_base__make_empty_struct() {
  wuffs_base__empty_struct ret;
  ret.private_impl = 0;
  return ret;
}

// wuffs_base__utility is a placeholder receiver type. It enables what Java
// calls static methods, as opposed to regular methods.
typedef struct {
  // private_impl is a placeholder field. It isn't explicitly used, except that
  // without it, the sizeof a struct with no fields can differ across C/C++
  // compilers, and it is undefined behavior in C99. For example, gcc says that
  // the sizeof an empty struct is 0, and g++ says that it is 1. This leads to
  // ABI incompatibility if a Wuffs .c file is processed by one compiler and
  // its .h file with another compiler.
  //
  // Instead, we explicitly insert an otherwise unused field, so that the
  // sizeof this struct is always 1.
  uint8_t private_impl;
} wuffs_base__utility;

typedef struct {
  const char* vtable_name;
  const void* function_pointers;
} wuffs_base__vtable;

// --------

// See https://github.com/google/wuffs/blob/master/doc/note/statuses.md
typedef struct {
  const char* repr;

#ifdef __cplusplus
  inline bool is_complete() const;
  inline bool is_error() const;
  inline bool is_note() const;
  inline bool is_ok() const;
  inline bool is_suspension() const;
  inline const char* message() const;
#endif  // __cplusplus

} wuffs_base__status;

extern const char* wuffs_base__note__i_o_redirect;
extern const char* wuffs_base__note__end_of_data;
extern const char* wuffs_base__note__metadata_reported;
extern const char* wuffs_base__suspension__even_more_information;
extern const char* wuffs_base__suspension__mispositioned_read;
extern const char* wuffs_base__suspension__mispositioned_write;
extern const char* wuffs_base__suspension__short_read;
extern const char* wuffs_base__suspension__short_write;
extern const char* wuffs_base__error__bad_i_o_position;
extern const char* wuffs_base__error__bad_argument_length_too_short;
extern const char* wuffs_base__error__bad_argument;
extern const char* wuffs_base__error__bad_call_sequence;
extern const char* wuffs_base__error__bad_receiver;
extern const char* wuffs_base__error__bad_restart;
extern const char* wuffs_base__error__bad_sizeof_receiver;
extern const char* wuffs_base__error__bad_vtable;
extern const char* wuffs_base__error__bad_workbuf_length;
extern const char* wuffs_base__error__bad_wuffs_version;
extern const char* wuffs_base__error__cannot_return_a_suspension;
extern const char* wuffs_base__error__disabled_by_previous_error;
extern const char* wuffs_base__error__initialize_falsely_claimed_already_zeroed;
extern const char* wuffs_base__error__initialize_not_called;
extern const char* wuffs_base__error__interleaved_coroutine_calls;
extern const char* wuffs_base__error__no_more_information;
extern const char* wuffs_base__error__not_enough_data;
extern const char* wuffs_base__error__out_of_bounds;
extern const char* wuffs_base__error__unsupported_method;
extern const char* wuffs_base__error__unsupported_option;
extern const char* wuffs_base__error__unsupported_pixel_swizzler_option;
extern const char* wuffs_base__error__too_much_data;

static inline wuffs_base__status  //
wuffs_base__make_status(const char* repr) {
  wuffs_base__status z;
  z.repr = repr;
  return z;
}

static inline bool  //
wuffs_base__status__is_complete(const wuffs_base__status* z) {
  return (z->repr == NULL) || ((*z->repr != '$') && (*z->repr != '#'));
}

static inline bool  //
wuffs_base__status__is_error(const wuffs_base__status* z) {
  return z->repr && (*z->repr == '#');
}

static inline bool  //
wuffs_base__status__is_note(const wuffs_base__status* z) {
  return z->repr && (*z->repr != '$') && (*z->repr != '#');
}

static inline bool  //
wuffs_base__status__is_ok(const wuffs_base__status* z) {
  return z->repr == NULL;
}

static inline bool  //
wuffs_base__status__is_suspension(const wuffs_base__status* z) {
  return z->repr && (*z->repr == '$');
}

// wuffs_base__status__message strips the leading '$', '#' or '@'.
static inline const char*  //
wuffs_base__status__message(const wuffs_base__status* z) {
  if (z->repr) {
    if ((*z->repr == '$') || (*z->repr == '#') || (*z->repr == '@')) {
      return z->repr + 1;
    }
  }
  return z->repr;
}

#ifdef __cplusplus

inline bool  //
wuffs_base__status::is_complete() const {
  return wuffs_base__status__is_complete(this);
}

inline bool  //
wuffs_base__status::is_error() const {
  return wuffs_base__status__is_error(this);
}

inline bool  //
wuffs_base__status::is_note() const {
  return wuffs_base__status__is_note(this);
}

inline bool  //
wuffs_base__status::is_ok() const {
  return wuffs_base__status__is_ok(this);
}

inline bool  //
wuffs_base__status::is_suspension() const {
  return wuffs_base__status__is_suspension(this);
}

inline const char*  //
wuffs_base__status::message() const {
  return wuffs_base__status__message(this);
}

#endif  // __cplusplus

// --------

// WUFFS_BASE__RESULT is a result type: either a status (an error) or a value.
//
// A result with all fields NULL or zero is as valid as a zero-valued T.
#define WUFFS_BASE__RESULT(T)  \
  struct {                     \
    wuffs_base__status status; \
    T value;                   \
  }

typedef WUFFS_BASE__RESULT(double) wuffs_base__result_f64;
typedef WUFFS_BASE__RESULT(int64_t) wuffs_base__result_i64;
typedef WUFFS_BASE__RESULT(uint64_t) wuffs_base__result_u64;

// --------

// FourCC constants.

// International Color Consortium Profile.
#define WUFFS_BASE__FOURCC__ICCP 0x49434350

// Joint Photographic Experts Group.
#define WUFFS_BASE__FOURCC__JPEG 0x4A504547

// Portable Network Graphics.
#define WUFFS_BASE__FOURCC__PNG 0x504E4720

// Extensible Metadata Platform.
#define WUFFS_BASE__FOURCC__XMP 0x584D5020

// --------

// Flicks are a unit of time. One flick (frame-tick) is 1 / 705_600_000 of a
// second. See https://github.com/OculusVR/Flicks
typedef int64_t wuffs_base__flicks;

#define WUFFS_BASE__FLICKS_PER_SECOND ((uint64_t)705600000)
#define WUFFS_BASE__FLICKS_PER_MILLISECOND ((uint64_t)705600)

// ---------------- Numeric Types

// The helpers below are functions, instead of macros, because their arguments
// can be an expression that we shouldn't evaluate more than once.
//
// They are static, so that linking multiple wuffs .o files won't complain about
// duplicate function definitions.
//
// They are explicitly marked inline, even if modern compilers don't use the
// inline attribute to guide optimizations such as inlining, to avoid the
// -Wunused-function warning, and we like to compile with -Wall -Werror.

static inline uint8_t  //
wuffs_base__u8__min(uint8_t x, uint8_t y) {
  return x < y ? x : y;
}

static inline uint8_t  //
wuffs_base__u8__max(uint8_t x, uint8_t y) {
  return x > y ? x : y;
}

static inline uint16_t  //
wuffs_base__u16__min(uint16_t x, uint16_t y) {
  return x < y ? x : y;
}

static inline uint16_t  //
wuffs_base__u16__max(uint16_t x, uint16_t y) {
  return x > y ? x : y;
}

static inline uint32_t  //
wuffs_base__u32__min(uint32_t x, uint32_t y) {
  return x < y ? x : y;
}

static inline uint32_t  //
wuffs_base__u32__max(uint32_t x, uint32_t y) {
  return x > y ? x : y;
}

static inline uint64_t  //
wuffs_base__u64__min(uint64_t x, uint64_t y) {
  return x < y ? x : y;
}

static inline uint64_t  //
wuffs_base__u64__max(uint64_t x, uint64_t y) {
  return x > y ? x : y;
}

// --------

// Saturating arithmetic (sat_add, sat_sub) branchless bit-twiddling algorithms
// are per https://locklessinc.com/articles/sat_arithmetic/
//
// It is important that the underlying types are unsigned integers, as signed
// integer arithmetic overflow is undefined behavior in C.

static inline uint8_t  //
wuffs_base__u8__sat_add(uint8_t x, uint8_t y) {
  uint8_t res = (uint8_t)(x + y);
  res |= (uint8_t)(-(res < x));
  return res;
}

static inline uint8_t  //
wuffs_base__u8__sat_sub(uint8_t x, uint8_t y) {
  uint8_t res = (uint8_t)(x - y);
  res &= (uint8_t)(-(res <= x));
  return res;
}

static inline uint16_t  //
wuffs_base__u16__sat_add(uint16_t x, uint16_t y) {
  uint16_t res = (uint16_t)(x + y);
  res |= (uint16_t)(-(res < x));
  return res;
}

static inline uint16_t  //
wuffs_base__u16__sat_sub(uint16_t x, uint16_t y) {
  uint16_t res = (uint16_t)(x - y);
  res &= (uint16_t)(-(res <= x));
  return res;
}

static inline uint32_t  //
wuffs_base__u32__sat_add(uint32_t x, uint32_t y) {
  uint32_t res = (uint32_t)(x + y);
  res |= (uint32_t)(-(res < x));
  return res;
}

static inline uint32_t  //
wuffs_base__u32__sat_sub(uint32_t x, uint32_t y) {
  uint32_t res = (uint32_t)(x - y);
  res &= (uint32_t)(-(res <= x));
  return res;
}

static inline uint64_t  //
wuffs_base__u64__sat_add(uint64_t x, uint64_t y) {
  uint64_t res = (uint64_t)(x + y);
  res |= (uint64_t)(-(res < x));
  return res;
}

static inline uint64_t  //
wuffs_base__u64__sat_sub(uint64_t x, uint64_t y) {
  uint64_t res = (uint64_t)(x - y);
  res &= (uint64_t)(-(res <= x));
  return res;
}

// --------

typedef struct {
  uint64_t hi;
  uint64_t lo;
} wuffs_base__multiply_u64__output;

// wuffs_base__multiply_u64 returns x*y as a 128-bit value.
//
// The maximum inclusive output hi_lo is 0xFFFFFFFFFFFFFFFE_0000000000000001.
static inline wuffs_base__multiply_u64__output  //
wuffs_base__multiply_u64(uint64_t x, uint64_t y) {
  uint64_t x0 = x & 0xFFFFFFFF;
  uint64_t x1 = x >> 32;
  uint64_t y0 = y & 0xFFFFFFFF;
  uint64_t y1 = y >> 32;
  uint64_t w0 = x0 * y0;
  uint64_t t = (x1 * y0) + (w0 >> 32);
  uint64_t w1 = t & 0xFFFFFFFF;
  uint64_t w2 = t >> 32;
  w1 += x0 * y1;
  wuffs_base__multiply_u64__output o;
  o.hi = (x1 * y1) + w2 + (w1 >> 32);
  o.lo = x * y;
  return o;
}

// --------

#if defined(__GNUC__) && (__SIZEOF_LONG__ == 8)

static inline uint32_t  //
wuffs_base__count_leading_zeroes_u64(uint64_t u) {
  return u ? ((uint32_t)(__builtin_clzl(u))) : 64u;
}

#else

static inline uint32_t  //
wuffs_base__count_leading_zeroes_u64(uint64_t u) {
  if (u == 0) {
    return 64;
  }

  uint32_t n = 0;
  if ((u >> 32) == 0) {
    n |= 32;
    u <<= 32;
  }
  if ((u >> 48) == 0) {
    n |= 16;
    u <<= 16;
  }
  if ((u >> 56) == 0) {
    n |= 8;
    u <<= 8;
  }
  if ((u >> 60) == 0) {
    n |= 4;
    u <<= 4;
  }
  if ((u >> 62) == 0) {
    n |= 2;
    u <<= 2;
  }
  if ((u >> 63) == 0) {
    n |= 1;
    u <<= 1;
  }
  return n;
}

#endif  // defined(__GNUC__) && (__SIZEOF_LONG__ == 8)

// --------

#define wuffs_base__load_u8be__no_bounds_check \
  wuffs_base__load_u8__no_bounds_check
#define wuffs_base__load_u8le__no_bounds_check \
  wuffs_base__load_u8__no_bounds_check

static inline uint8_t  //
wuffs_base__load_u8__no_bounds_check(const uint8_t* p) {
  return p[0];
}

static inline uint16_t  //
wuffs_base__load_u16be__no_bounds_check(const uint8_t* p) {
  return (uint16_t)(((uint16_t)(p[0]) << 8) | ((uint16_t)(p[1]) << 0));
}

static inline uint16_t  //
wuffs_base__load_u16le__no_bounds_check(const uint8_t* p) {
  return (uint16_t)(((uint16_t)(p[0]) << 0) | ((uint16_t)(p[1]) << 8));
}

static inline uint32_t  //
wuffs_base__load_u24be__no_bounds_check(const uint8_t* p) {
  return ((uint32_t)(p[0]) << 16) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 0);
}

static inline uint32_t  //
wuffs_base__load_u24le__no_bounds_check(const uint8_t* p) {
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16);
}

static inline uint32_t  //
wuffs_base__load_u32be__no_bounds_check(const uint8_t* p) {
  return ((uint32_t)(p[0]) << 24) | ((uint32_t)(p[1]) << 16) |
         ((uint32_t)(p[2]) << 8) | ((uint32_t)(p[3]) << 0);
}

static inline uint32_t  //
wuffs_base__load_u32le__no_bounds_check(const uint8_t* p) {
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16) | ((uint32_t)(p[3]) << 24);
}

static inline uint64_t  //
wuffs_base__load_u40be__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 32) | ((uint64_t)(p[1]) << 24) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 8) |
         ((uint64_t)(p[4]) << 0);
}

static inline uint64_t  //
wuffs_base__load_u40le__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32);
}

static inline uint64_t  //
wuffs_base__load_u48be__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 40) | ((uint64_t)(p[1]) << 32) |
         ((uint64_t)(p[2]) << 24) | ((uint64_t)(p[3]) << 16) |
         ((uint64_t)(p[4]) << 8) | ((uint64_t)(p[5]) << 0);
}

static inline uint64_t  //
wuffs_base__load_u48le__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32) | ((uint64_t)(p[5]) << 40);
}

static inline uint64_t  //
wuffs_base__load_u56be__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 48) | ((uint64_t)(p[1]) << 40) |
         ((uint64_t)(p[2]) << 32) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 16) | ((uint64_t)(p[5]) << 8) |
         ((uint64_t)(p[6]) << 0);
}

static inline uint64_t  //
wuffs_base__load_u56le__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32) | ((uint64_t)(p[5]) << 40) |
         ((uint64_t)(p[6]) << 48);
}

static inline uint64_t  //
wuffs_base__load_u64be__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 56) | ((uint64_t)(p[1]) << 48) |
         ((uint64_t)(p[2]) << 40) | ((uint64_t)(p[3]) << 32) |
         ((uint64_t)(p[4]) << 24) | ((uint64_t)(p[5]) << 16) |
         ((uint64_t)(p[6]) << 8) | ((uint64_t)(p[7]) << 0);
}

static inline uint64_t  //
wuffs_base__load_u64le__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32) | ((uint64_t)(p[5]) << 40) |
         ((uint64_t)(p[6]) << 48) | ((uint64_t)(p[7]) << 56);
}

// --------

#define wuffs_base__store_u8be__no_bounds_check \
  wuffs_base__store_u8__no_bounds_check
#define wuffs_base__store_u8le__no_bounds_check \
  wuffs_base__store_u8__no_bounds_check

static inline void  //
wuffs_base__store_u8__no_bounds_check(uint8_t* p, uint8_t x) {
  p[0] = x;
}

static inline void  //
wuffs_base__store_u16be__no_bounds_check(uint8_t* p, uint16_t x) {
  p[0] = (uint8_t)(x >> 8);
  p[1] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u16le__no_bounds_check(uint8_t* p, uint16_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
}

static inline void  //
wuffs_base__store_u24be__no_bounds_check(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 16);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u24le__no_bounds_check(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
}

static inline void  //
wuffs_base__store_u32be__no_bounds_check(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 24);
  p[1] = (uint8_t)(x >> 16);
  p[2] = (uint8_t)(x >> 8);
  p[3] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u32le__no_bounds_check(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
}

static inline void  //
wuffs_base__store_u40be__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 32);
  p[1] = (uint8_t)(x >> 24);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 8);
  p[4] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u40le__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
}

static inline void  //
wuffs_base__store_u48be__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 40);
  p[1] = (uint8_t)(x >> 32);
  p[2] = (uint8_t)(x >> 24);
  p[3] = (uint8_t)(x >> 16);
  p[4] = (uint8_t)(x >> 8);
  p[5] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u48le__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
  p[5] = (uint8_t)(x >> 40);
}

static inline void  //
wuffs_base__store_u56be__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 48);
  p[1] = (uint8_t)(x >> 40);
  p[2] = (uint8_t)(x >> 32);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 16);
  p[5] = (uint8_t)(x >> 8);
  p[6] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u56le__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
  p[5] = (uint8_t)(x >> 40);
  p[6] = (uint8_t)(x >> 48);
}

static inline void  //
wuffs_base__store_u64be__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 56);
  p[1] = (uint8_t)(x >> 48);
  p[2] = (uint8_t)(x >> 40);
  p[3] = (uint8_t)(x >> 32);
  p[4] = (uint8_t)(x >> 24);
  p[5] = (uint8_t)(x >> 16);
  p[6] = (uint8_t)(x >> 8);
  p[7] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u64le__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
  p[5] = (uint8_t)(x >> 40);
  p[6] = (uint8_t)(x >> 48);
  p[7] = (uint8_t)(x >> 56);
}

// ---------------- Slices and Tables

// WUFFS_BASE__SLICE is a 1-dimensional buffer.
//
// len measures a number of elements, not necessarily a size in bytes.
//
// A value with all fields NULL or zero is a valid, empty slice.
#define WUFFS_BASE__SLICE(T) \
  struct {                   \
    T* ptr;                  \
    size_t len;              \
  }

// WUFFS_BASE__TABLE is a 2-dimensional buffer.
//
// width height, and stride measure a number of elements, not necessarily a
// size in bytes.
//
// A value with all fields NULL or zero is a valid, empty table.
#define WUFFS_BASE__TABLE(T) \
  struct {                   \
    T* ptr;                  \
    size_t width;            \
    size_t height;           \
    size_t stride;           \
  }

typedef WUFFS_BASE__SLICE(uint8_t) wuffs_base__slice_u8;
typedef WUFFS_BASE__SLICE(uint16_t) wuffs_base__slice_u16;
typedef WUFFS_BASE__SLICE(uint32_t) wuffs_base__slice_u32;
typedef WUFFS_BASE__SLICE(uint64_t) wuffs_base__slice_u64;

typedef WUFFS_BASE__TABLE(uint8_t) wuffs_base__table_u8;
typedef WUFFS_BASE__TABLE(uint16_t) wuffs_base__table_u16;
typedef WUFFS_BASE__TABLE(uint32_t) wuffs_base__table_u32;
typedef WUFFS_BASE__TABLE(uint64_t) wuffs_base__table_u64;

static inline wuffs_base__slice_u8  //
wuffs_base__make_slice_u8(uint8_t* ptr, size_t len) {
  wuffs_base__slice_u8 ret;
  ret.ptr = ptr;
  ret.len = len;
  return ret;
}

static inline wuffs_base__slice_u16  //
wuffs_base__make_slice_u16(uint16_t* ptr, size_t len) {
  wuffs_base__slice_u16 ret;
  ret.ptr = ptr;
  ret.len = len;
  return ret;
}

static inline wuffs_base__slice_u32  //
wuffs_base__make_slice_u32(uint32_t* ptr, size_t len) {
  wuffs_base__slice_u32 ret;
  ret.ptr = ptr;
  ret.len = len;
  return ret;
}

static inline wuffs_base__slice_u64  //
wuffs_base__make_slice_u64(uint64_t* ptr, size_t len) {
  wuffs_base__slice_u64 ret;
  ret.ptr = ptr;
  ret.len = len;
  return ret;
}

static inline wuffs_base__slice_u8  //
wuffs_base__empty_slice_u8() {
  wuffs_base__slice_u8 ret;
  ret.ptr = NULL;
  ret.len = 0;
  return ret;
}

static inline wuffs_base__table_u8  //
wuffs_base__empty_table_u8() {
  wuffs_base__table_u8 ret;
  ret.ptr = NULL;
  ret.width = 0;
  ret.height = 0;
  ret.stride = 0;
  return ret;
}

// wuffs_base__slice_u8__subslice_i returns s[i:].
//
// It returns an empty slice if i is out of bounds.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__subslice_i(wuffs_base__slice_u8 s, uint64_t i) {
  if ((i <= SIZE_MAX) && (i <= s.len)) {
    return wuffs_base__make_slice_u8(s.ptr + i, s.len - i);
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

// wuffs_base__slice_u8__subslice_j returns s[:j].
//
// It returns an empty slice if j is out of bounds.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__subslice_j(wuffs_base__slice_u8 s, uint64_t j) {
  if ((j <= SIZE_MAX) && (j <= s.len)) {
    return wuffs_base__make_slice_u8(s.ptr, j);
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

// wuffs_base__slice_u8__subslice_ij returns s[i:j].
//
// It returns an empty slice if i or j is out of bounds.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__subslice_ij(wuffs_base__slice_u8 s,
                                  uint64_t i,
                                  uint64_t j) {
  if ((i <= j) && (j <= SIZE_MAX) && (j <= s.len)) {
    return wuffs_base__make_slice_u8(s.ptr + i, j - i);
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

// ---------------- Ranges and Rects

// See https://github.com/google/wuffs/blob/master/doc/note/ranges-and-rects.md

typedef struct wuffs_base__range_ii_u32__struct {
  uint32_t min_incl;
  uint32_t max_incl;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__range_ii_u32__struct s) const;
  inline wuffs_base__range_ii_u32__struct intersect(
      wuffs_base__range_ii_u32__struct s) const;
  inline wuffs_base__range_ii_u32__struct unite(
      wuffs_base__range_ii_u32__struct s) const;
  inline bool contains(uint32_t x) const;
  inline bool contains_range(wuffs_base__range_ii_u32__struct s) const;
#endif  // __cplusplus

} wuffs_base__range_ii_u32;

static inline wuffs_base__range_ii_u32  //
wuffs_base__empty_range_ii_u32() {
  wuffs_base__range_ii_u32 ret;
  ret.min_incl = 0;
  ret.max_incl = 0;
  return ret;
}

static inline wuffs_base__range_ii_u32  //
wuffs_base__make_range_ii_u32(uint32_t min_incl, uint32_t max_incl) {
  wuffs_base__range_ii_u32 ret;
  ret.min_incl = min_incl;
  ret.max_incl = max_incl;
  return ret;
}

static inline bool  //
wuffs_base__range_ii_u32__is_empty(const wuffs_base__range_ii_u32* r) {
  return r->min_incl > r->max_incl;
}

static inline bool  //
wuffs_base__range_ii_u32__equals(const wuffs_base__range_ii_u32* r,
                                 wuffs_base__range_ii_u32 s) {
  return (r->min_incl == s.min_incl && r->max_incl == s.max_incl) ||
         (wuffs_base__range_ii_u32__is_empty(r) &&
          wuffs_base__range_ii_u32__is_empty(&s));
}

static inline wuffs_base__range_ii_u32  //
wuffs_base__range_ii_u32__intersect(const wuffs_base__range_ii_u32* r,
                                    wuffs_base__range_ii_u32 s) {
  wuffs_base__range_ii_u32 t;
  t.min_incl = wuffs_base__u32__max(r->min_incl, s.min_incl);
  t.max_incl = wuffs_base__u32__min(r->max_incl, s.max_incl);
  return t;
}

static inline wuffs_base__range_ii_u32  //
wuffs_base__range_ii_u32__unite(const wuffs_base__range_ii_u32* r,
                                wuffs_base__range_ii_u32 s) {
  if (wuffs_base__range_ii_u32__is_empty(r)) {
    return s;
  }
  if (wuffs_base__range_ii_u32__is_empty(&s)) {
    return *r;
  }
  wuffs_base__range_ii_u32 t;
  t.min_incl = wuffs_base__u32__min(r->min_incl, s.min_incl);
  t.max_incl = wuffs_base__u32__max(r->max_incl, s.max_incl);
  return t;
}

static inline bool  //
wuffs_base__range_ii_u32__contains(const wuffs_base__range_ii_u32* r,
                                   uint32_t x) {
  return (r->min_incl <= x) && (x <= r->max_incl);
}

static inline bool  //
wuffs_base__range_ii_u32__contains_range(const wuffs_base__range_ii_u32* r,
                                         wuffs_base__range_ii_u32 s) {
  return wuffs_base__range_ii_u32__equals(
      &s, wuffs_base__range_ii_u32__intersect(r, s));
}

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ii_u32::is_empty() const {
  return wuffs_base__range_ii_u32__is_empty(this);
}

inline bool  //
wuffs_base__range_ii_u32::equals(wuffs_base__range_ii_u32 s) const {
  return wuffs_base__range_ii_u32__equals(this, s);
}

inline wuffs_base__range_ii_u32  //
wuffs_base__range_ii_u32::intersect(wuffs_base__range_ii_u32 s) const {
  return wuffs_base__range_ii_u32__intersect(this, s);
}

inline wuffs_base__range_ii_u32  //
wuffs_base__range_ii_u32::unite(wuffs_base__range_ii_u32 s) const {
  return wuffs_base__range_ii_u32__unite(this, s);
}

inline bool  //
wuffs_base__range_ii_u32::contains(uint32_t x) const {
  return wuffs_base__range_ii_u32__contains(this, x);
}

inline bool  //
wuffs_base__range_ii_u32::contains_range(wuffs_base__range_ii_u32 s) const {
  return wuffs_base__range_ii_u32__contains_range(this, s);
}

#endif  // __cplusplus

// --------

typedef struct wuffs_base__range_ie_u32__struct {
  uint32_t min_incl;
  uint32_t max_excl;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__range_ie_u32__struct s) const;
  inline wuffs_base__range_ie_u32__struct intersect(
      wuffs_base__range_ie_u32__struct s) const;
  inline wuffs_base__range_ie_u32__struct unite(
      wuffs_base__range_ie_u32__struct s) const;
  inline bool contains(uint32_t x) const;
  inline bool contains_range(wuffs_base__range_ie_u32__struct s) const;
  inline uint32_t length() const;
#endif  // __cplusplus

} wuffs_base__range_ie_u32;

static inline wuffs_base__range_ie_u32  //
wuffs_base__empty_range_ie_u32() {
  wuffs_base__range_ie_u32 ret;
  ret.min_incl = 0;
  ret.max_excl = 0;
  return ret;
}

static inline wuffs_base__range_ie_u32  //
wuffs_base__make_range_ie_u32(uint32_t min_incl, uint32_t max_excl) {
  wuffs_base__range_ie_u32 ret;
  ret.min_incl = min_incl;
  ret.max_excl = max_excl;
  return ret;
}

static inline bool  //
wuffs_base__range_ie_u32__is_empty(const wuffs_base__range_ie_u32* r) {
  return r->min_incl >= r->max_excl;
}

static inline bool  //
wuffs_base__range_ie_u32__equals(const wuffs_base__range_ie_u32* r,
                                 wuffs_base__range_ie_u32 s) {
  return (r->min_incl == s.min_incl && r->max_excl == s.max_excl) ||
         (wuffs_base__range_ie_u32__is_empty(r) &&
          wuffs_base__range_ie_u32__is_empty(&s));
}

static inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32__intersect(const wuffs_base__range_ie_u32* r,
                                    wuffs_base__range_ie_u32 s) {
  wuffs_base__range_ie_u32 t;
  t.min_incl = wuffs_base__u32__max(r->min_incl, s.min_incl);
  t.max_excl = wuffs_base__u32__min(r->max_excl, s.max_excl);
  return t;
}

static inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32__unite(const wuffs_base__range_ie_u32* r,
                                wuffs_base__range_ie_u32 s) {
  if (wuffs_base__range_ie_u32__is_empty(r)) {
    return s;
  }
  if (wuffs_base__range_ie_u32__is_empty(&s)) {
    return *r;
  }
  wuffs_base__range_ie_u32 t;
  t.min_incl = wuffs_base__u32__min(r->min_incl, s.min_incl);
  t.max_excl = wuffs_base__u32__max(r->max_excl, s.max_excl);
  return t;
}

static inline bool  //
wuffs_base__range_ie_u32__contains(const wuffs_base__range_ie_u32* r,
                                   uint32_t x) {
  return (r->min_incl <= x) && (x < r->max_excl);
}

static inline bool  //
wuffs_base__range_ie_u32__contains_range(const wuffs_base__range_ie_u32* r,
                                         wuffs_base__range_ie_u32 s) {
  return wuffs_base__range_ie_u32__equals(
      &s, wuffs_base__range_ie_u32__intersect(r, s));
}

static inline uint32_t  //
wuffs_base__range_ie_u32__length(const wuffs_base__range_ie_u32* r) {
  return wuffs_base__u32__sat_sub(r->max_excl, r->min_incl);
}

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ie_u32::is_empty() const {
  return wuffs_base__range_ie_u32__is_empty(this);
}

inline bool  //
wuffs_base__range_ie_u32::equals(wuffs_base__range_ie_u32 s) const {
  return wuffs_base__range_ie_u32__equals(this, s);
}

inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32::intersect(wuffs_base__range_ie_u32 s) const {
  return wuffs_base__range_ie_u32__intersect(this, s);
}

inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32::unite(wuffs_base__range_ie_u32 s) const {
  return wuffs_base__range_ie_u32__unite(this, s);
}

inline bool  //
wuffs_base__range_ie_u32::contains(uint32_t x) const {
  return wuffs_base__range_ie_u32__contains(this, x);
}

inline bool  //
wuffs_base__range_ie_u32::contains_range(wuffs_base__range_ie_u32 s) const {
  return wuffs_base__range_ie_u32__contains_range(this, s);
}

inline uint32_t  //
wuffs_base__range_ie_u32::length() const {
  return wuffs_base__range_ie_u32__length(this);
}

#endif  // __cplusplus

// --------

typedef struct wuffs_base__range_ii_u64__struct {
  uint64_t min_incl;
  uint64_t max_incl;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__range_ii_u64__struct s) const;
  inline wuffs_base__range_ii_u64__struct intersect(
      wuffs_base__range_ii_u64__struct s) const;
  inline wuffs_base__range_ii_u64__struct unite(
      wuffs_base__range_ii_u64__struct s) const;
  inline bool contains(uint64_t x) const;
  inline bool contains_range(wuffs_base__range_ii_u64__struct s) const;
#endif  // __cplusplus

} wuffs_base__range_ii_u64;

static inline wuffs_base__range_ii_u64  //
wuffs_base__empty_range_ii_u64() {
  wuffs_base__range_ii_u64 ret;
  ret.min_incl = 0;
  ret.max_incl = 0;
  return ret;
}

static inline wuffs_base__range_ii_u64  //
wuffs_base__make_range_ii_u64(uint64_t min_incl, uint64_t max_incl) {
  wuffs_base__range_ii_u64 ret;
  ret.min_incl = min_incl;
  ret.max_incl = max_incl;
  return ret;
}

static inline bool  //
wuffs_base__range_ii_u64__is_empty(const wuffs_base__range_ii_u64* r) {
  return r->min_incl > r->max_incl;
}

static inline bool  //
wuffs_base__range_ii_u64__equals(const wuffs_base__range_ii_u64* r,
                                 wuffs_base__range_ii_u64 s) {
  return (r->min_incl == s.min_incl && r->max_incl == s.max_incl) ||
         (wuffs_base__range_ii_u64__is_empty(r) &&
          wuffs_base__range_ii_u64__is_empty(&s));
}

static inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64__intersect(const wuffs_base__range_ii_u64* r,
                                    wuffs_base__range_ii_u64 s) {
  wuffs_base__range_ii_u64 t;
  t.min_incl = wuffs_base__u64__max(r->min_incl, s.min_incl);
  t.max_incl = wuffs_base__u64__min(r->max_incl, s.max_incl);
  return t;
}

static inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64__unite(const wuffs_base__range_ii_u64* r,
                                wuffs_base__range_ii_u64 s) {
  if (wuffs_base__range_ii_u64__is_empty(r)) {
    return s;
  }
  if (wuffs_base__range_ii_u64__is_empty(&s)) {
    return *r;
  }
  wuffs_base__range_ii_u64 t;
  t.min_incl = wuffs_base__u64__min(r->min_incl, s.min_incl);
  t.max_incl = wuffs_base__u64__max(r->max_incl, s.max_incl);
  return t;
}

static inline bool  //
wuffs_base__range_ii_u64__contains(const wuffs_base__range_ii_u64* r,
                                   uint64_t x) {
  return (r->min_incl <= x) && (x <= r->max_incl);
}

static inline bool  //
wuffs_base__range_ii_u64__contains_range(const wuffs_base__range_ii_u64* r,
                                         wuffs_base__range_ii_u64 s) {
  return wuffs_base__range_ii_u64__equals(
      &s, wuffs_base__range_ii_u64__intersect(r, s));
}

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ii_u64::is_empty() const {
  return wuffs_base__range_ii_u64__is_empty(this);
}

inline bool  //
wuffs_base__range_ii_u64::equals(wuffs_base__range_ii_u64 s) const {
  return wuffs_base__range_ii_u64__equals(this, s);
}

inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64::intersect(wuffs_base__range_ii_u64 s) const {
  return wuffs_base__range_ii_u64__intersect(this, s);
}

inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64::unite(wuffs_base__range_ii_u64 s) const {
  return wuffs_base__range_ii_u64__unite(this, s);
}

inline bool  //
wuffs_base__range_ii_u64::contains(uint64_t x) const {
  return wuffs_base__range_ii_u64__contains(this, x);
}

inline bool  //
wuffs_base__range_ii_u64::contains_range(wuffs_base__range_ii_u64 s) const {
  return wuffs_base__range_ii_u64__contains_range(this, s);
}

#endif  // __cplusplus

// --------

typedef struct wuffs_base__range_ie_u64__struct {
  uint64_t min_incl;
  uint64_t max_excl;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__range_ie_u64__struct s) const;
  inline wuffs_base__range_ie_u64__struct intersect(
      wuffs_base__range_ie_u64__struct s) const;
  inline wuffs_base__range_ie_u64__struct unite(
      wuffs_base__range_ie_u64__struct s) const;
  inline bool contains(uint64_t x) const;
  inline bool contains_range(wuffs_base__range_ie_u64__struct s) const;
  inline uint64_t length() const;
#endif  // __cplusplus

} wuffs_base__range_ie_u64;

static inline wuffs_base__range_ie_u64  //
wuffs_base__empty_range_ie_u64() {
  wuffs_base__range_ie_u64 ret;
  ret.min_incl = 0;
  ret.max_excl = 0;
  return ret;
}

static inline wuffs_base__range_ie_u64  //
wuffs_base__make_range_ie_u64(uint64_t min_incl, uint64_t max_excl) {
  wuffs_base__range_ie_u64 ret;
  ret.min_incl = min_incl;
  ret.max_excl = max_excl;
  return ret;
}

static inline bool  //
wuffs_base__range_ie_u64__is_empty(const wuffs_base__range_ie_u64* r) {
  return r->min_incl >= r->max_excl;
}

static inline bool  //
wuffs_base__range_ie_u64__equals(const wuffs_base__range_ie_u64* r,
                                 wuffs_base__range_ie_u64 s) {
  return (r->min_incl == s.min_incl && r->max_excl == s.max_excl) ||
         (wuffs_base__range_ie_u64__is_empty(r) &&
          wuffs_base__range_ie_u64__is_empty(&s));
}

static inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64__intersect(const wuffs_base__range_ie_u64* r,
                                    wuffs_base__range_ie_u64 s) {
  wuffs_base__range_ie_u64 t;
  t.min_incl = wuffs_base__u64__max(r->min_incl, s.min_incl);
  t.max_excl = wuffs_base__u64__min(r->max_excl, s.max_excl);
  return t;
}

static inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64__unite(const wuffs_base__range_ie_u64* r,
                                wuffs_base__range_ie_u64 s) {
  if (wuffs_base__range_ie_u64__is_empty(r)) {
    return s;
  }
  if (wuffs_base__range_ie_u64__is_empty(&s)) {
    return *r;
  }
  wuffs_base__range_ie_u64 t;
  t.min_incl = wuffs_base__u64__min(r->min_incl, s.min_incl);
  t.max_excl = wuffs_base__u64__max(r->max_excl, s.max_excl);
  return t;
}

static inline bool  //
wuffs_base__range_ie_u64__contains(const wuffs_base__range_ie_u64* r,
                                   uint64_t x) {
  return (r->min_incl <= x) && (x < r->max_excl);
}

static inline bool  //
wuffs_base__range_ie_u64__contains_range(const wuffs_base__range_ie_u64* r,
                                         wuffs_base__range_ie_u64 s) {
  return wuffs_base__range_ie_u64__equals(
      &s, wuffs_base__range_ie_u64__intersect(r, s));
}

static inline uint64_t  //
wuffs_base__range_ie_u64__length(const wuffs_base__range_ie_u64* r) {
  return wuffs_base__u64__sat_sub(r->max_excl, r->min_incl);
}

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ie_u64::is_empty() const {
  return wuffs_base__range_ie_u64__is_empty(this);
}

inline bool  //
wuffs_base__range_ie_u64::equals(wuffs_base__range_ie_u64 s) const {
  return wuffs_base__range_ie_u64__equals(this, s);
}

inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64::intersect(wuffs_base__range_ie_u64 s) const {
  return wuffs_base__range_ie_u64__intersect(this, s);
}

inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64::unite(wuffs_base__range_ie_u64 s) const {
  return wuffs_base__range_ie_u64__unite(this, s);
}

inline bool  //
wuffs_base__range_ie_u64::contains(uint64_t x) const {
  return wuffs_base__range_ie_u64__contains(this, x);
}

inline bool  //
wuffs_base__range_ie_u64::contains_range(wuffs_base__range_ie_u64 s) const {
  return wuffs_base__range_ie_u64__contains_range(this, s);
}

inline uint64_t  //
wuffs_base__range_ie_u64::length() const {
  return wuffs_base__range_ie_u64__length(this);
}

#endif  // __cplusplus

// --------

typedef struct wuffs_base__rect_ii_u32__struct {
  uint32_t min_incl_x;
  uint32_t min_incl_y;
  uint32_t max_incl_x;
  uint32_t max_incl_y;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__rect_ii_u32__struct s) const;
  inline wuffs_base__rect_ii_u32__struct intersect(
      wuffs_base__rect_ii_u32__struct s) const;
  inline wuffs_base__rect_ii_u32__struct unite(
      wuffs_base__rect_ii_u32__struct s) const;
  inline bool contains(uint32_t x, uint32_t y) const;
  inline bool contains_rect(wuffs_base__rect_ii_u32__struct s) const;
#endif  // __cplusplus

} wuffs_base__rect_ii_u32;

static inline wuffs_base__rect_ii_u32  //
wuffs_base__empty_rect_ii_u32() {
  wuffs_base__rect_ii_u32 ret;
  ret.min_incl_x = 0;
  ret.min_incl_y = 0;
  ret.max_incl_x = 0;
  ret.max_incl_y = 0;
  return ret;
}

static inline wuffs_base__rect_ii_u32  //
wuffs_base__make_rect_ii_u32(uint32_t min_incl_x,
                             uint32_t min_incl_y,
                             uint32_t max_incl_x,
                             uint32_t max_incl_y) {
  wuffs_base__rect_ii_u32 ret;
  ret.min_incl_x = min_incl_x;
  ret.min_incl_y = min_incl_y;
  ret.max_incl_x = max_incl_x;
  ret.max_incl_y = max_incl_y;
  return ret;
}

static inline bool  //
wuffs_base__rect_ii_u32__is_empty(const wuffs_base__rect_ii_u32* r) {
  return (r->min_incl_x > r->max_incl_x) || (r->min_incl_y > r->max_incl_y);
}

static inline bool  //
wuffs_base__rect_ii_u32__equals(const wuffs_base__rect_ii_u32* r,
                                wuffs_base__rect_ii_u32 s) {
  return (r->min_incl_x == s.min_incl_x && r->min_incl_y == s.min_incl_y &&
          r->max_incl_x == s.max_incl_x && r->max_incl_y == s.max_incl_y) ||
         (wuffs_base__rect_ii_u32__is_empty(r) &&
          wuffs_base__rect_ii_u32__is_empty(&s));
}

static inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32__intersect(const wuffs_base__rect_ii_u32* r,
                                   wuffs_base__rect_ii_u32 s) {
  wuffs_base__rect_ii_u32 t;
  t.min_incl_x = wuffs_base__u32__max(r->min_incl_x, s.min_incl_x);
  t.min_incl_y = wuffs_base__u32__max(r->min_incl_y, s.min_incl_y);
  t.max_incl_x = wuffs_base__u32__min(r->max_incl_x, s.max_incl_x);
  t.max_incl_y = wuffs_base__u32__min(r->max_incl_y, s.max_incl_y);
  return t;
}

static inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32__unite(const wuffs_base__rect_ii_u32* r,
                               wuffs_base__rect_ii_u32 s) {
  if (wuffs_base__rect_ii_u32__is_empty(r)) {
    return s;
  }
  if (wuffs_base__rect_ii_u32__is_empty(&s)) {
    return *r;
  }
  wuffs_base__rect_ii_u32 t;
  t.min_incl_x = wuffs_base__u32__min(r->min_incl_x, s.min_incl_x);
  t.min_incl_y = wuffs_base__u32__min(r->min_incl_y, s.min_incl_y);
  t.max_incl_x = wuffs_base__u32__max(r->max_incl_x, s.max_incl_x);
  t.max_incl_y = wuffs_base__u32__max(r->max_incl_y, s.max_incl_y);
  return t;
}

static inline bool  //
wuffs_base__rect_ii_u32__contains(const wuffs_base__rect_ii_u32* r,
                                  uint32_t x,
                                  uint32_t y) {
  return (r->min_incl_x <= x) && (x <= r->max_incl_x) && (r->min_incl_y <= y) &&
         (y <= r->max_incl_y);
}

static inline bool  //
wuffs_base__rect_ii_u32__contains_rect(const wuffs_base__rect_ii_u32* r,
                                       wuffs_base__rect_ii_u32 s) {
  return wuffs_base__rect_ii_u32__equals(
      &s, wuffs_base__rect_ii_u32__intersect(r, s));
}

#ifdef __cplusplus

inline bool  //
wuffs_base__rect_ii_u32::is_empty() const {
  return wuffs_base__rect_ii_u32__is_empty(this);
}

inline bool  //
wuffs_base__rect_ii_u32::equals(wuffs_base__rect_ii_u32 s) const {
  return wuffs_base__rect_ii_u32__equals(this, s);
}

inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32::intersect(wuffs_base__rect_ii_u32 s) const {
  return wuffs_base__rect_ii_u32__intersect(this, s);
}

inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32::unite(wuffs_base__rect_ii_u32 s) const {
  return wuffs_base__rect_ii_u32__unite(this, s);
}

inline bool  //
wuffs_base__rect_ii_u32::contains(uint32_t x, uint32_t y) const {
  return wuffs_base__rect_ii_u32__contains(this, x, y);
}

inline bool  //
wuffs_base__rect_ii_u32::contains_rect(wuffs_base__rect_ii_u32 s) const {
  return wuffs_base__rect_ii_u32__contains_rect(this, s);
}

#endif  // __cplusplus

// --------

typedef struct wuffs_base__rect_ie_u32__struct {
  uint32_t min_incl_x;
  uint32_t min_incl_y;
  uint32_t max_excl_x;
  uint32_t max_excl_y;

#ifdef __cplusplus
  inline bool is_empty() const;
  inline bool equals(wuffs_base__rect_ie_u32__struct s) const;
  inline wuffs_base__rect_ie_u32__struct intersect(
      wuffs_base__rect_ie_u32__struct s) const;
  inline wuffs_base__rect_ie_u32__struct unite(
      wuffs_base__rect_ie_u32__struct s) const;
  inline bool contains(uint32_t x, uint32_t y) const;
  inline bool contains_rect(wuffs_base__rect_ie_u32__struct s) const;
  inline uint32_t width() const;
  inline uint32_t height() const;
#endif  // __cplusplus

} wuffs_base__rect_ie_u32;

static inline wuffs_base__rect_ie_u32  //
wuffs_base__empty_rect_ie_u32() {
  wuffs_base__rect_ie_u32 ret;
  ret.min_incl_x = 0;
  ret.min_incl_y = 0;
  ret.max_excl_x = 0;
  ret.max_excl_y = 0;
  return ret;
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__make_rect_ie_u32(uint32_t min_incl_x,
                             uint32_t min_incl_y,
                             uint32_t max_excl_x,
                             uint32_t max_excl_y) {
  wuffs_base__rect_ie_u32 ret;
  ret.min_incl_x = min_incl_x;
  ret.min_incl_y = min_incl_y;
  ret.max_excl_x = max_excl_x;
  ret.max_excl_y = max_excl_y;
  return ret;
}

static inline bool  //
wuffs_base__rect_ie_u32__is_empty(const wuffs_base__rect_ie_u32* r) {
  return (r->min_incl_x >= r->max_excl_x) || (r->min_incl_y >= r->max_excl_y);
}

static inline bool  //
wuffs_base__rect_ie_u32__equals(const wuffs_base__rect_ie_u32* r,
                                wuffs_base__rect_ie_u32 s) {
  return (r->min_incl_x == s.min_incl_x && r->min_incl_y == s.min_incl_y &&
          r->max_excl_x == s.max_excl_x && r->max_excl_y == s.max_excl_y) ||
         (wuffs_base__rect_ie_u32__is_empty(r) &&
          wuffs_base__rect_ie_u32__is_empty(&s));
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32__intersect(const wuffs_base__rect_ie_u32* r,
                                   wuffs_base__rect_ie_u32 s) {
  wuffs_base__rect_ie_u32 t;
  t.min_incl_x = wuffs_base__u32__max(r->min_incl_x, s.min_incl_x);
  t.min_incl_y = wuffs_base__u32__max(r->min_incl_y, s.min_incl_y);
  t.max_excl_x = wuffs_base__u32__min(r->max_excl_x, s.max_excl_x);
  t.max_excl_y = wuffs_base__u32__min(r->max_excl_y, s.max_excl_y);
  return t;
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32__unite(const wuffs_base__rect_ie_u32* r,
                               wuffs_base__rect_ie_u32 s) {
  if (wuffs_base__rect_ie_u32__is_empty(r)) {
    return s;
  }
  if (wuffs_base__rect_ie_u32__is_empty(&s)) {
    return *r;
  }
  wuffs_base__rect_ie_u32 t;
  t.min_incl_x = wuffs_base__u32__min(r->min_incl_x, s.min_incl_x);
  t.min_incl_y = wuffs_base__u32__min(r->min_incl_y, s.min_incl_y);
  t.max_excl_x = wuffs_base__u32__max(r->max_excl_x, s.max_excl_x);
  t.max_excl_y = wuffs_base__u32__max(r->max_excl_y, s.max_excl_y);
  return t;
}

static inline bool  //
wuffs_base__rect_ie_u32__contains(const wuffs_base__rect_ie_u32* r,
                                  uint32_t x,
                                  uint32_t y) {
  return (r->min_incl_x <= x) && (x < r->max_excl_x) && (r->min_incl_y <= y) &&
         (y < r->max_excl_y);
}

static inline bool  //
wuffs_base__rect_ie_u32__contains_rect(const wuffs_base__rect_ie_u32* r,
                                       wuffs_base__rect_ie_u32 s) {
  return wuffs_base__rect_ie_u32__equals(
      &s, wuffs_base__rect_ie_u32__intersect(r, s));
}

static inline uint32_t  //
wuffs_base__rect_ie_u32__width(const wuffs_base__rect_ie_u32* r) {
  return wuffs_base__u32__sat_sub(r->max_excl_x, r->min_incl_x);
}

static inline uint32_t  //
wuffs_base__rect_ie_u32__height(const wuffs_base__rect_ie_u32* r) {
  return wuffs_base__u32__sat_sub(r->max_excl_y, r->min_incl_y);
}

#ifdef __cplusplus

inline bool  //
wuffs_base__rect_ie_u32::is_empty() const {
  return wuffs_base__rect_ie_u32__is_empty(this);
}

inline bool  //
wuffs_base__rect_ie_u32::equals(wuffs_base__rect_ie_u32 s) const {
  return wuffs_base__rect_ie_u32__equals(this, s);
}

inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32::intersect(wuffs_base__rect_ie_u32 s) const {
  return wuffs_base__rect_ie_u32__intersect(this, s);
}

inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32::unite(wuffs_base__rect_ie_u32 s) const {
  return wuffs_base__rect_ie_u32__unite(this, s);
}

inline bool  //
wuffs_base__rect_ie_u32::contains(uint32_t x, uint32_t y) const {
  return wuffs_base__rect_ie_u32__contains(this, x, y);
}

inline bool  //
wuffs_base__rect_ie_u32::contains_rect(wuffs_base__rect_ie_u32 s) const {
  return wuffs_base__rect_ie_u32__contains_rect(this, s);
}

inline uint32_t  //
wuffs_base__rect_ie_u32::width() const {
  return wuffs_base__rect_ie_u32__width(this);
}

inline uint32_t  //
wuffs_base__rect_ie_u32::height() const {
  return wuffs_base__rect_ie_u32__height(this);
}

#endif  // __cplusplus

// ---------------- More Information

// wuffs_base__more_information holds additional fields, typically when a Wuffs
// method returns a [note status](/doc/note/statuses.md).
//
// The flavor field follows the base38 namespace
// convention](/doc/note/base38-and-fourcc.md). The other fields' semantics
// depends on the flavor.
typedef struct {
  uint32_t flavor;
  uint32_t w;
  uint64_t x;
  uint64_t y;
  uint64_t z;

#ifdef __cplusplus
  inline void set(uint32_t flavor_arg,
                  uint32_t w_arg,
                  uint64_t x_arg,
                  uint64_t y_arg,
                  uint64_t z_arg);
  inline uint32_t io_redirect__fourcc() const;
  inline wuffs_base__range_ie_u64 io_redirect__range() const;
  inline uint64_t io_seek__position() const;
  inline uint32_t metadata__fourcc() const;
  inline wuffs_base__range_ie_u64 metadata__range() const;
#endif  // __cplusplus

} wuffs_base__more_information;

#define WUFFS_BASE__MORE_INFORMATION__FLAVOR__IO_REDIRECT 1
#define WUFFS_BASE__MORE_INFORMATION__FLAVOR__IO_SEEK 2
#define WUFFS_BASE__MORE_INFORMATION__FLAVOR__METADATA 3

static inline wuffs_base__more_information  //
wuffs_base__empty_more_information() {
  wuffs_base__more_information ret;
  ret.flavor = 0;
  ret.w = 0;
  ret.x = 0;
  ret.y = 0;
  ret.z = 0;
  return ret;
}

static inline void  //
wuffs_base__more_information__set(wuffs_base__more_information* m,
                                  uint32_t flavor,
                                  uint32_t w,
                                  uint64_t x,
                                  uint64_t y,
                                  uint64_t z) {
  if (!m) {
    return;
  }
  m->flavor = flavor;
  m->w = w;
  m->x = x;
  m->y = y;
  m->z = z;
}

static inline uint32_t  //
wuffs_base__more_information__io_redirect__fourcc(
    const wuffs_base__more_information* m) {
  return m->w;
}

static inline wuffs_base__range_ie_u64  //
wuffs_base__more_information__io_redirect__range(
    const wuffs_base__more_information* m) {
  wuffs_base__range_ie_u64 ret;
  ret.min_incl = m->y;
  ret.max_excl = m->z;
  return ret;
}

static inline uint64_t  //
wuffs_base__more_information__io_seek__position(
    const wuffs_base__more_information* m) {
  return m->x;
}

static inline uint32_t  //
wuffs_base__more_information__metadata__fourcc(
    const wuffs_base__more_information* m) {
  return m->w;
}

static inline wuffs_base__range_ie_u64  //
wuffs_base__more_information__metadata__range(
    const wuffs_base__more_information* m) {
  wuffs_base__range_ie_u64 ret;
  ret.min_incl = m->y;
  ret.max_excl = m->z;
  return ret;
}

#ifdef __cplusplus

inline void  //
wuffs_base__more_information::set(uint32_t flavor_arg,
                                  uint32_t w_arg,
                                  uint64_t x_arg,
                                  uint64_t y_arg,
                                  uint64_t z_arg) {
  wuffs_base__more_information__set(this, flavor_arg, w_arg, x_arg, y_arg,
                                    z_arg);
}

inline uint32_t  //
wuffs_base__more_information::io_redirect__fourcc() const {
  return wuffs_base__more_information__io_redirect__fourcc(this);
}

inline wuffs_base__range_ie_u64  //
wuffs_base__more_information::io_redirect__range() const {
  return wuffs_base__more_information__io_redirect__range(this);
}

inline uint64_t  //
wuffs_base__more_information::io_seek__position() const {
  return wuffs_base__more_information__io_seek__position(this);
}

inline uint32_t  //
wuffs_base__more_information::metadata__fourcc() const {
  return wuffs_base__more_information__metadata__fourcc(this);
}

inline wuffs_base__range_ie_u64  //
wuffs_base__more_information::metadata__range() const {
  return wuffs_base__more_information__metadata__range(this);
}

#endif  // __cplusplus

// ---------------- I/O
//
// See (/doc/note/io-input-output.md).

// wuffs_base__io_buffer_meta is the metadata for a wuffs_base__io_buffer's
// data.
typedef struct {
  size_t wi;     // Write index. Invariant: wi <= len.
  size_t ri;     // Read  index. Invariant: ri <= wi.
  uint64_t pos;  // Position of the buffer start relative to the stream start.
  bool closed;   // No further writes are expected.
} wuffs_base__io_buffer_meta;

// wuffs_base__io_buffer is a 1-dimensional buffer (a pointer and length) plus
// additional metadata.
//
// A value with all fields zero is a valid, empty buffer.
typedef struct {
  wuffs_base__slice_u8 data;
  wuffs_base__io_buffer_meta meta;

#ifdef __cplusplus
  inline bool is_valid() const;
  inline void compact();
  inline uint64_t reader_available() const;
  inline uint64_t reader_io_position() const;
  inline uint64_t writer_available() const;
  inline uint64_t writer_io_position() const;
#endif  // __cplusplus

} wuffs_base__io_buffer;

static inline wuffs_base__io_buffer  //
wuffs_base__make_io_buffer(wuffs_base__slice_u8 data,
                           wuffs_base__io_buffer_meta meta) {
  wuffs_base__io_buffer ret;
  ret.data = data;
  ret.meta = meta;
  return ret;
}

static inline wuffs_base__io_buffer_meta  //
wuffs_base__make_io_buffer_meta(size_t wi,
                                size_t ri,
                                uint64_t pos,
                                bool closed) {
  wuffs_base__io_buffer_meta ret;
  ret.wi = wi;
  ret.ri = ri;
  ret.pos = pos;
  ret.closed = closed;
  return ret;
}

static inline wuffs_base__io_buffer  //
wuffs_base__ptr_u8__reader(uint8_t* ptr, size_t len, bool closed) {
  wuffs_base__io_buffer ret;
  ret.data.ptr = ptr;
  ret.data.len = len;
  ret.meta.wi = len;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = closed;
  return ret;
}

static inline wuffs_base__io_buffer  //
wuffs_base__ptr_u8__writer(uint8_t* ptr, size_t len) {
  wuffs_base__io_buffer ret;
  ret.data.ptr = ptr;
  ret.data.len = len;
  ret.meta.wi = 0;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = false;
  return ret;
}

static inline wuffs_base__io_buffer  //
wuffs_base__slice_u8__reader(wuffs_base__slice_u8 s, bool closed) {
  wuffs_base__io_buffer ret;
  ret.data.ptr = s.ptr;
  ret.data.len = s.len;
  ret.meta.wi = s.len;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = closed;
  return ret;
}

static inline wuffs_base__io_buffer  //
wuffs_base__slice_u8__writer(wuffs_base__slice_u8 s) {
  wuffs_base__io_buffer ret;
  ret.data.ptr = s.ptr;
  ret.data.len = s.len;
  ret.meta.wi = 0;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = false;
  return ret;
}

static inline wuffs_base__io_buffer  //
wuffs_base__empty_io_buffer() {
  wuffs_base__io_buffer ret;
  ret.data.ptr = NULL;
  ret.data.len = 0;
  ret.meta.wi = 0;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = false;
  return ret;
}

static inline wuffs_base__io_buffer_meta  //
wuffs_base__empty_io_buffer_meta() {
  wuffs_base__io_buffer_meta ret;
  ret.wi = 0;
  ret.ri = 0;
  ret.pos = 0;
  ret.closed = false;
  return ret;
}

static inline bool  //
wuffs_base__io_buffer__is_valid(const wuffs_base__io_buffer* buf) {
  if (buf) {
    if (buf->data.ptr) {
      return (buf->meta.ri <= buf->meta.wi) && (buf->meta.wi <= buf->data.len);
    } else {
      return (buf->meta.ri == 0) && (buf->meta.wi == 0) && (buf->data.len == 0);
    }
  }
  return false;
}

// wuffs_base__io_buffer__compact moves any written but unread bytes to the
// start of the buffer.
static inline void  //
wuffs_base__io_buffer__compact(wuffs_base__io_buffer* buf) {
  if (!buf || (buf->meta.ri == 0)) {
    return;
  }
  buf->meta.pos = wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.ri);
  size_t n = buf->meta.wi - buf->meta.ri;
  if (n != 0) {
    memmove(buf->data.ptr, buf->data.ptr + buf->meta.ri, n);
  }
  buf->meta.wi = n;
  buf->meta.ri = 0;
}

static inline uint64_t  //
wuffs_base__io_buffer__reader_available(const wuffs_base__io_buffer* buf) {
  return buf ? buf->meta.wi - buf->meta.ri : 0;
}

static inline uint64_t  //
wuffs_base__io_buffer__reader_io_position(const wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.ri) : 0;
}

static inline uint64_t  //
wuffs_base__io_buffer__writer_available(const wuffs_base__io_buffer* buf) {
  return buf ? buf->data.len - buf->meta.wi : 0;
}

static inline uint64_t  //
wuffs_base__io_buffer__writer_io_position(const wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.wi) : 0;
}

#ifdef __cplusplus

inline bool  //
wuffs_base__io_buffer::is_valid() const {
  return wuffs_base__io_buffer__is_valid(this);
}

inline void  //
wuffs_base__io_buffer::compact() {
  wuffs_base__io_buffer__compact(this);
}

inline uint64_t  //
wuffs_base__io_buffer::reader_available() const {
  return wuffs_base__io_buffer__reader_available(this);
}

inline uint64_t  //
wuffs_base__io_buffer::reader_io_position() const {
  return wuffs_base__io_buffer__reader_io_position(this);
}

inline uint64_t  //
wuffs_base__io_buffer::writer_available() const {
  return wuffs_base__io_buffer__writer_available(this);
}

inline uint64_t  //
wuffs_base__io_buffer::writer_io_position() const {
  return wuffs_base__io_buffer__writer_io_position(this);
}

#endif  // __cplusplus

// ---------------- Tokens

// wuffs_base__token is an element of a byte stream's tokenization.
//
// See https://github.com/google/wuffs/blob/master/doc/note/tokens.md
typedef struct {
  uint64_t repr;

#ifdef __cplusplus
  inline int64_t value() const;
  inline int64_t value_extension() const;
  inline int64_t value_major() const;
  inline int64_t value_base_category() const;
  inline uint64_t value_minor() const;
  inline uint64_t value_base_detail() const;
  inline bool continued() const;
  inline uint64_t length() const;
#endif  // __cplusplus

} wuffs_base__token;

static inline wuffs_base__token  //
wuffs_base__make_token(uint64_t repr) {
  wuffs_base__token ret;
  ret.repr = repr;
  return ret;
}

// --------

#define WUFFS_BASE__TOKEN__LENGTH__MAX_INCL 0xFFFF

#define WUFFS_BASE__TOKEN__VALUE__SHIFT 17
#define WUFFS_BASE__TOKEN__VALUE_EXTENSION__SHIFT 17
#define WUFFS_BASE__TOKEN__VALUE_MAJOR__SHIFT 42
#define WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT 17
#define WUFFS_BASE__TOKEN__VALUE_BASE_CATEGORY__SHIFT 38
#define WUFFS_BASE__TOKEN__VALUE_BASE_DETAIL__SHIFT 17
#define WUFFS_BASE__TOKEN__CONTINUED__SHIFT 16
#define WUFFS_BASE__TOKEN__LENGTH__SHIFT 0

// --------

#define WUFFS_BASE__TOKEN__VBC__FILLER 0
#define WUFFS_BASE__TOKEN__VBC__STRUCTURE 1
#define WUFFS_BASE__TOKEN__VBC__STRING 2
#define WUFFS_BASE__TOKEN__VBC__UNICODE_CODE_POINT 3
#define WUFFS_BASE__TOKEN__VBC__LITERAL 4
#define WUFFS_BASE__TOKEN__VBC__NUMBER 5

// --------

#define WUFFS_BASE__TOKEN__VBD__FILLER__COMMENT_LINE 0x00001
#define WUFFS_BASE__TOKEN__VBD__FILLER__COMMENT_BLOCK 0x00002

// --------

#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__PUSH 0x00001
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__POP 0x00002
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_NONE 0x00010
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_LIST 0x00020
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_DICT 0x00040
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_NONE 0x01000
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST 0x02000
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_DICT 0x04000

// --------

// "DEFINITELY_FOO" means that the destination bytes (and also the source
// bytes, for 1_DST_1_SRC_COPY) are in the FOO format. Definitely means that
// the lack of the bit is conservative: it is valid for all-ASCII strings to
// have neither DEFINITELY_UTF_8 or DEFINITELY_ASCII bits set.
#define WUFFS_BASE__TOKEN__VBD__STRING__DEFINITELY_UTF_8 0x00001
#define WUFFS_BASE__TOKEN__VBD__STRING__DEFINITELY_ASCII 0x00002

// "CONVERT_D_DST_S_SRC" means that multiples of S source bytes (possibly
// padded) produces multiples of D destination bytes. For example,
// CONVERT_1_DST_4_SRC_BACKSLASH_X means a source like "\\x23\\x67\\xAB", where
// 12 src bytes encode 3 dst bytes.
//
// Post-processing may further transform those D destination bytes (e.g. treat
// "\\xFF" as the Unicode code point U+00FF instead of the byte 0xFF), but that
// is out of scope of this VBD's semantics.
//
// When src is the empty string, multiple conversion algorithms are applicable
// (so these bits are not necessarily mutually exclusive), all producing the
// same empty dst string.
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_0_DST_1_SRC_DROP 0x00010
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_1_DST_1_SRC_COPY 0x00020
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_1_DST_2_SRC_HEXADECIMAL 0x00040
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_1_DST_4_SRC_BACKSLASH_X 0x00080
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_3_DST_4_SRC_BASE_64_STD 0x00100
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_3_DST_4_SRC_BASE_64_URL 0x00200
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_4_DST_5_SRC_ASCII_85 0x00400

// --------

#define WUFFS_BASE__TOKEN__VBD__LITERAL__UNDEFINED 0x00001
#define WUFFS_BASE__TOKEN__VBD__LITERAL__NULL 0x00002
#define WUFFS_BASE__TOKEN__VBD__LITERAL__FALSE 0x00004
#define WUFFS_BASE__TOKEN__VBD__LITERAL__TRUE 0x00008

// --------

// For a source string of "123" or "0x9A", it is valid for a tokenizer to
// return any one of:
//  - WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_FLOATING_POINT.
//  - WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_INTEGER_SIGNED.
//  - WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_INTEGER_UNSIGNED.
//
// For a source string of "+123" or "-0x9A", only the first two are valid.
//
// For a source string of "123.", only the first one is valid.
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_FLOATING_POINT 0x00001
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_INTEGER_SIGNED 0x00002
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_INTEGER_UNSIGNED 0x00004

#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_NEG_INF 0x00010
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_POS_INF 0x00020
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_NEG_NAN 0x00040
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_POS_NAN 0x00080

// The number 300 might be represented as "\x01\x2C", "\x2C\x01\x00\x00" or
// "300", which are big-endian, little-endian or text. For binary formats, the
// token length discriminates e.g. u16 little-endian vs u32 little-endian.
#define WUFFS_BASE__TOKEN__VBD__NUMBER__FORMAT_BINARY_BIG_ENDIAN 0x00100
#define WUFFS_BASE__TOKEN__VBD__NUMBER__FORMAT_BINARY_LITTLE_ENDIAN 0x00200
#define WUFFS_BASE__TOKEN__VBD__NUMBER__FORMAT_TEXT 0x00400

// --------

// wuffs_base__token__value returns the token's high 46 bits, sign-extended. A
// negative value means an extended token, non-negative means a simple token.
static inline int64_t  //
wuffs_base__token__value(const wuffs_base__token* t) {
  return ((int64_t)(t->repr)) >> WUFFS_BASE__TOKEN__VALUE__SHIFT;
}

// wuffs_base__token__value_extension returns a negative value if the token was
// not an extended token.
static inline int64_t  //
wuffs_base__token__value_extension(const wuffs_base__token* t) {
  return (~(int64_t)(t->repr)) >> WUFFS_BASE__TOKEN__VALUE_EXTENSION__SHIFT;
}

// wuffs_base__token__value_major returns a negative value if the token was not
// a simple token.
static inline int64_t  //
wuffs_base__token__value_major(const wuffs_base__token* t) {
  return ((int64_t)(t->repr)) >> WUFFS_BASE__TOKEN__VALUE_MAJOR__SHIFT;
}

// wuffs_base__token__value_base_category returns a negative value if the token
// was not a simple token.
static inline int64_t  //
wuffs_base__token__value_base_category(const wuffs_base__token* t) {
  return ((int64_t)(t->repr)) >> WUFFS_BASE__TOKEN__VALUE_BASE_CATEGORY__SHIFT;
}

static inline uint64_t  //
wuffs_base__token__value_minor(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) & 0x1FFFFFF;
}

static inline uint64_t  //
wuffs_base__token__value_base_detail(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__VALUE_BASE_DETAIL__SHIFT) & 0x1FFFFF;
}

static inline bool  //
wuffs_base__token__continued(const wuffs_base__token* t) {
  return t->repr & 0x10000;
}

static inline uint64_t  //
wuffs_base__token__length(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__LENGTH__SHIFT) & 0xFFFF;
}

#ifdef __cplusplus

inline int64_t  //
wuffs_base__token::value() const {
  return wuffs_base__token__value(this);
}

inline int64_t  //
wuffs_base__token::value_extension() const {
  return wuffs_base__token__value_extension(this);
}

inline int64_t  //
wuffs_base__token::value_major() const {
  return wuffs_base__token__value_major(this);
}

inline int64_t  //
wuffs_base__token::value_base_category() const {
  return wuffs_base__token__value_base_category(this);
}

inline uint64_t  //
wuffs_base__token::value_minor() const {
  return wuffs_base__token__value_minor(this);
}

inline uint64_t  //
wuffs_base__token::value_base_detail() const {
  return wuffs_base__token__value_base_detail(this);
}

inline bool  //
wuffs_base__token::continued() const {
  return wuffs_base__token__continued(this);
}

inline uint64_t  //
wuffs_base__token::length() const {
  return wuffs_base__token__length(this);
}

#endif  // __cplusplus

// --------

typedef WUFFS_BASE__SLICE(wuffs_base__token) wuffs_base__slice_token;

static inline wuffs_base__slice_token  //
wuffs_base__make_slice_token(wuffs_base__token* ptr, size_t len) {
  wuffs_base__slice_token ret;
  ret.ptr = ptr;
  ret.len = len;
  return ret;
}

// --------

// wuffs_base__token_buffer_meta is the metadata for a
// wuffs_base__token_buffer's data.
typedef struct {
  size_t wi;     // Write index. Invariant: wi <= len.
  size_t ri;     // Read  index. Invariant: ri <= wi.
  uint64_t pos;  // Position of the buffer start relative to the stream start.
  bool closed;   // No further writes are expected.
} wuffs_base__token_buffer_meta;

// wuffs_base__token_buffer is a 1-dimensional buffer (a pointer and length)
// plus additional metadata.
//
// A value with all fields zero is a valid, empty buffer.
typedef struct {
  wuffs_base__slice_token data;
  wuffs_base__token_buffer_meta meta;

#ifdef __cplusplus
  inline bool is_valid() const;
  inline void compact();
  inline uint64_t reader_available() const;
  inline uint64_t reader_token_position() const;
  inline uint64_t writer_available() const;
  inline uint64_t writer_token_position() const;
#endif  // __cplusplus

} wuffs_base__token_buffer;

static inline wuffs_base__token_buffer  //
wuffs_base__make_token_buffer(wuffs_base__slice_token data,
                              wuffs_base__token_buffer_meta meta) {
  wuffs_base__token_buffer ret;
  ret.data = data;
  ret.meta = meta;
  return ret;
}

static inline wuffs_base__token_buffer_meta  //
wuffs_base__make_token_buffer_meta(size_t wi,
                                   size_t ri,
                                   uint64_t pos,
                                   bool closed) {
  wuffs_base__token_buffer_meta ret;
  ret.wi = wi;
  ret.ri = ri;
  ret.pos = pos;
  ret.closed = closed;
  return ret;
}

static inline wuffs_base__token_buffer  //
wuffs_base__slice_token__reader(wuffs_base__slice_token s, bool closed) {
  wuffs_base__token_buffer ret;
  ret.data.ptr = s.ptr;
  ret.data.len = s.len;
  ret.meta.wi = s.len;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = closed;
  return ret;
}

static inline wuffs_base__token_buffer  //
wuffs_base__slice_token__writer(wuffs_base__slice_token s) {
  wuffs_base__token_buffer ret;
  ret.data.ptr = s.ptr;
  ret.data.len = s.len;
  ret.meta.wi = 0;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = false;
  return ret;
}

static inline wuffs_base__token_buffer  //
wuffs_base__empty_token_buffer() {
  wuffs_base__token_buffer ret;
  ret.data.ptr = NULL;
  ret.data.len = 0;
  ret.meta.wi = 0;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = false;
  return ret;
}

static inline wuffs_base__token_buffer_meta  //
wuffs_base__empty_token_buffer_meta() {
  wuffs_base__token_buffer_meta ret;
  ret.wi = 0;
  ret.ri = 0;
  ret.pos = 0;
  ret.closed = false;
  return ret;
}

static inline bool  //
wuffs_base__token_buffer__is_valid(const wuffs_base__token_buffer* buf) {
  if (buf) {
    if (buf->data.ptr) {
      return (buf->meta.ri <= buf->meta.wi) && (buf->meta.wi <= buf->data.len);
    } else {
      return (buf->meta.ri == 0) && (buf->meta.wi == 0) && (buf->data.len == 0);
    }
  }
  return false;
}

// wuffs_base__token_buffer__compact moves any written but unread tokens to the
// start of the buffer.
static inline void  //
wuffs_base__token_buffer__compact(wuffs_base__token_buffer* buf) {
  if (!buf || (buf->meta.ri == 0)) {
    return;
  }
  buf->meta.pos = wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.ri);
  size_t n = buf->meta.wi - buf->meta.ri;
  if (n != 0) {
    memmove(buf->data.ptr, buf->data.ptr + buf->meta.ri,
            n * sizeof(wuffs_base__token));
  }
  buf->meta.wi = n;
  buf->meta.ri = 0;
}

static inline uint64_t  //
wuffs_base__token_buffer__reader_available(
    const wuffs_base__token_buffer* buf) {
  return buf ? buf->meta.wi - buf->meta.ri : 0;
}

static inline uint64_t  //
wuffs_base__token_buffer__reader_token_position(
    const wuffs_base__token_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.ri) : 0;
}

static inline uint64_t  //
wuffs_base__token_buffer__writer_available(
    const wuffs_base__token_buffer* buf) {
  return buf ? buf->data.len - buf->meta.wi : 0;
}

static inline uint64_t  //
wuffs_base__token_buffer__writer_token_position(
    const wuffs_base__token_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.wi) : 0;
}

#ifdef __cplusplus

inline bool  //
wuffs_base__token_buffer::is_valid() const {
  return wuffs_base__token_buffer__is_valid(this);
}

inline void  //
wuffs_base__token_buffer::compact() {
  wuffs_base__token_buffer__compact(this);
}

inline uint64_t  //
wuffs_base__token_buffer::reader_available() const {
  return wuffs_base__token_buffer__reader_available(this);
}

inline uint64_t  //
wuffs_base__token_buffer::reader_token_position() const {
  return wuffs_base__token_buffer__reader_token_position(this);
}

inline uint64_t  //
wuffs_base__token_buffer::writer_available() const {
  return wuffs_base__token_buffer__writer_available(this);
}

inline uint64_t  //
wuffs_base__token_buffer::writer_token_position() const {
  return wuffs_base__token_buffer__writer_token_position(this);
}

#endif  // __cplusplus

// ---------------- Memory Allocation

// The memory allocation related functions in this section aren't used by Wuffs
// per se, but they may be helpful to the code that uses Wuffs.

// wuffs_base__malloc_slice_uxx wraps calling a malloc-like function, except
// that it takes a uint64_t number of elements instead of a size_t size in
// bytes, and it returns a slice (a pointer and a length) instead of just a
// pointer.
//
// You can pass the C stdlib's malloc as the malloc_func.
//
// It returns an empty slice (containing a NULL ptr field) if (num_uxx *
// sizeof(uintxx_t)) would overflow SIZE_MAX.

static inline wuffs_base__slice_u8  //
wuffs_base__malloc_slice_u8(void* (*malloc_func)(size_t), uint64_t num_u8) {
  if (malloc_func && (num_u8 <= (SIZE_MAX / sizeof(uint8_t)))) {
    void* p = (*malloc_func)(num_u8 * sizeof(uint8_t));
    if (p) {
      return wuffs_base__make_slice_u8((uint8_t*)(p), num_u8);
    }
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

static inline wuffs_base__slice_u16  //
wuffs_base__malloc_slice_u16(void* (*malloc_func)(size_t), uint64_t num_u16) {
  if (malloc_func && (num_u16 <= (SIZE_MAX / sizeof(uint16_t)))) {
    void* p = (*malloc_func)(num_u16 * sizeof(uint16_t));
    if (p) {
      return wuffs_base__make_slice_u16((uint16_t*)(p), num_u16);
    }
  }
  return wuffs_base__make_slice_u16(NULL, 0);
}

static inline wuffs_base__slice_u32  //
wuffs_base__malloc_slice_u32(void* (*malloc_func)(size_t), uint64_t num_u32) {
  if (malloc_func && (num_u32 <= (SIZE_MAX / sizeof(uint32_t)))) {
    void* p = (*malloc_func)(num_u32 * sizeof(uint32_t));
    if (p) {
      return wuffs_base__make_slice_u32((uint32_t*)(p), num_u32);
    }
  }
  return wuffs_base__make_slice_u32(NULL, 0);
}

static inline wuffs_base__slice_u64  //
wuffs_base__malloc_slice_u64(void* (*malloc_func)(size_t), uint64_t num_u64) {
  if (malloc_func && (num_u64 <= (SIZE_MAX / sizeof(uint64_t)))) {
    void* p = (*malloc_func)(num_u64 * sizeof(uint64_t));
    if (p) {
      return wuffs_base__make_slice_u64((uint64_t*)(p), num_u64);
    }
  }
  return wuffs_base__make_slice_u64(NULL, 0);
}

// ---------------- Images

// wuffs_base__color_u32_argb_premul is an 8 bit per channel premultiplied
// Alpha, Red, Green, Blue color, as a uint32_t value. Its value is always
// 0xAARRGGBB (Alpha most significant, Blue least), regardless of endianness.
typedef uint32_t wuffs_base__color_u32_argb_premul;

static inline uint16_t  //
wuffs_base__color_u32_argb_premul__as__color_u16_rgb_565(
    wuffs_base__color_u32_argb_premul c) {
  uint32_t r5 = 0xF800 & (c >> 8);
  uint32_t g6 = 0x07E0 & (c >> 5);
  uint32_t b5 = 0x001F & (c >> 3);
  return (uint16_t)(r5 | g6 | b5);
}

static inline wuffs_base__color_u32_argb_premul  //
wuffs_base__color_u16_rgb_565__as__color_u32_argb_premul(uint16_t rgb_565) {
  uint32_t b5 = 0x1F & (rgb_565 >> 0);
  uint32_t b = (b5 << 3) | (b5 >> 2);
  uint32_t g6 = 0x3F & (rgb_565 >> 5);
  uint32_t g = (g6 << 2) | (g6 >> 4);
  uint32_t r5 = 0x1F & (rgb_565 >> 11);
  uint32_t r = (r5 << 3) | (r5 >> 2);
  return 0xFF000000 | (r << 16) | (g << 8) | (b << 0);
}

static inline uint8_t  //
wuffs_base__color_u32_argb_premul__as__color_u8_gray(
    wuffs_base__color_u32_argb_premul c) {
  // Work in 16-bit color.
  uint32_t cr = 0x101 * (0xFF & (c >> 16));
  uint32_t cg = 0x101 * (0xFF & (c >> 8));
  uint32_t cb = 0x101 * (0xFF & (c >> 0));

  // These coefficients (the fractions 0.299, 0.587 and 0.114) are the same
  // as those given by the JFIF specification.
  //
  // Note that 19595 + 38470 + 7471 equals 65536, also known as (1 << 16). We
  // shift by 24, not just by 16, because the return value is 8-bit color, not
  // 16-bit color.
  uint32_t weighted_average = (19595 * cr) + (38470 * cg) + (7471 * cb) + 32768;
  return (uint8_t)(weighted_average >> 24);
}

// wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul converts
// from non-premultiplied alpha to premultiplied alpha.
static inline wuffs_base__color_u32_argb_premul  //
wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
    uint32_t argb_nonpremul) {
  // Multiplying by 0x101 (twice, once for alpha and once for color) converts
  // from 8-bit to 16-bit color. Shifting right by 8 undoes that.
  //
  // Working in the higher bit depth can produce slightly different (and
  // arguably slightly more accurate) results. For example, given 8-bit blue
  // and alpha of 0x80 and 0x81:
  //
  //  - ((0x80   * 0x81  ) / 0xFF  )      = 0x40        = 0x40
  //  - ((0x8080 * 0x8181) / 0xFFFF) >> 8 = 0x4101 >> 8 = 0x41
  uint32_t a = 0xFF & (argb_nonpremul >> 24);
  uint32_t a16 = a * (0x101 * 0x101);

  uint32_t r = 0xFF & (argb_nonpremul >> 16);
  r = ((r * a16) / 0xFFFF) >> 8;
  uint32_t g = 0xFF & (argb_nonpremul >> 8);
  g = ((g * a16) / 0xFFFF) >> 8;
  uint32_t b = 0xFF & (argb_nonpremul >> 0);
  b = ((b * a16) / 0xFFFF) >> 8;

  return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}

// wuffs_base__color_u32_argb_premul__as__color_u32_argb_nonpremul converts
// from premultiplied alpha to non-premultiplied alpha.
static inline uint32_t  //
wuffs_base__color_u32_argb_premul__as__color_u32_argb_nonpremul(
    wuffs_base__color_u32_argb_premul c) {
  uint32_t a = 0xFF & (c >> 24);
  if (a == 0xFF) {
    return c;
  } else if (a == 0) {
    return 0;
  }
  uint32_t a16 = a * 0x101;

  uint32_t r = 0xFF & (c >> 16);
  r = ((r * (0x101 * 0xFFFF)) / a16) >> 8;
  uint32_t g = 0xFF & (c >> 8);
  g = ((g * (0x101 * 0xFFFF)) / a16) >> 8;
  uint32_t b = 0xFF & (c >> 0);
  b = ((b * (0x101 * 0xFFFF)) / a16) >> 8;

  return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}

// --------

typedef uint8_t wuffs_base__pixel_blend;

// wuffs_base__pixel_blend encodes how to blend source and destination pixels,
// accounting for transparency. It encompasses the Porter-Duff compositing
// operators as well as the other blending modes defined by PDF.
//
// TODO: implement the other modes.
#define WUFFS_BASE__PIXEL_BLEND__SRC ((wuffs_base__pixel_blend)0)
#define WUFFS_BASE__PIXEL_BLEND__SRC_OVER ((wuffs_base__pixel_blend)1)

// --------

// wuffs_base__pixel_alpha_transparency is a pixel format's alpha channel
// model. It is a property of the pixel format in general, not of a specific
// pixel. An RGBA pixel format (with alpha) can still have fully opaque pixels.
typedef uint32_t wuffs_base__pixel_alpha_transparency;

#define WUFFS_BASE__PIXEL_ALPHA_TRANSPARENCY__OPAQUE 0
#define WUFFS_BASE__PIXEL_ALPHA_TRANSPARENCY__NON_PREMULTIPLIED_ALPHA 1
#define WUFFS_BASE__PIXEL_ALPHA_TRANSPARENCY__PREMULTIPLIED_ALPHA 2
#define WUFFS_BASE__PIXEL_ALPHA_TRANSPARENCY__BINARY_ALPHA 3

// --------

#define WUFFS_BASE__PIXEL_FORMAT__NUM_PLANES_MAX 4

#define WUFFS_BASE__PIXEL_FORMAT__INDEXED__INDEX_PLANE 0
#define WUFFS_BASE__PIXEL_FORMAT__INDEXED__COLOR_PLANE 3

// wuffs_base__pixel_format encodes the format of the bytes that constitute an
// image frame's pixel data.
//
// See https://github.com/google/wuffs/blob/master/doc/note/pixel-formats.md
//
// Do not manipulate its bits directly; they are private implementation
// details. Use methods such as wuffs_base__pixel_format__num_planes instead.
typedef struct {
  uint32_t repr;

#ifdef __cplusplus
  inline bool is_valid() const;
  inline uint32_t bits_per_pixel() const;
  inline bool is_direct() const;
  inline bool is_indexed() const;
  inline bool is_interleaved() const;
  inline bool is_planar() const;
  inline uint32_t num_planes() const;
  inline wuffs_base__pixel_alpha_transparency transparency() const;
#endif  // __cplusplus

} wuffs_base__pixel_format;

static inline wuffs_base__pixel_format  //
wuffs_base__make_pixel_format(uint32_t repr) {
  wuffs_base__pixel_format f;
  f.repr = repr;
  return f;
}

// Common 8-bit-depth pixel formats. This list is not exhaustive; not all valid
// wuffs_base__pixel_format values are present.

#define WUFFS_BASE__PIXEL_FORMAT__INVALID 0x00000000

#define WUFFS_BASE__PIXEL_FORMAT__A 0x02000008

#define WUFFS_BASE__PIXEL_FORMAT__Y 0x20000008
#define WUFFS_BASE__PIXEL_FORMAT__YA_NONPREMUL 0x21000008
#define WUFFS_BASE__PIXEL_FORMAT__YA_PREMUL 0x22000008

#define WUFFS_BASE__PIXEL_FORMAT__YCBCR 0x40020888
#define WUFFS_BASE__PIXEL_FORMAT__YCBCRA_NONPREMUL 0x41038888
#define WUFFS_BASE__PIXEL_FORMAT__YCBCRK 0x50038888

#define WUFFS_BASE__PIXEL_FORMAT__YCOCG 0x60020888
#define WUFFS_BASE__PIXEL_FORMAT__YCOCGA_NONPREMUL 0x61038888
#define WUFFS_BASE__PIXEL_FORMAT__YCOCGK 0x70038888

#define WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL 0x81040008
#define WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL 0x82040008
#define WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY 0x83040008

#define WUFFS_BASE__PIXEL_FORMAT__BGR_565 0x80000565
#define WUFFS_BASE__PIXEL_FORMAT__BGR 0x80000888
#define WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL 0x81008888
#define WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL 0x82008888
#define WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY 0x83008888
#define WUFFS_BASE__PIXEL_FORMAT__BGRX 0x90008888

#define WUFFS_BASE__PIXEL_FORMAT__RGB 0xA0000888
#define WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL 0xA1008888
#define WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL 0xA2008888
#define WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY 0xA3008888
#define WUFFS_BASE__PIXEL_FORMAT__RGBX 0xB0008888

#define WUFFS_BASE__PIXEL_FORMAT__CMY 0xC0020888
#define WUFFS_BASE__PIXEL_FORMAT__CMYK 0xD0038888

extern const uint32_t wuffs_base__pixel_format__bits_per_channel[16];

static inline bool  //
wuffs_base__pixel_format__is_valid(const wuffs_base__pixel_format* f) {
  return f->repr != 0;
}

// wuffs_base__pixel_format__bits_per_pixel returns the number of bits per
// pixel for interleaved pixel formats, and returns 0 for planar pixel formats.
static inline uint32_t  //
wuffs_base__pixel_format__bits_per_pixel(const wuffs_base__pixel_format* f) {
  if (((f->repr >> 16) & 0x03) != 0) {
    return 0;
  }
  return wuffs_base__pixel_format__bits_per_channel[0x0F & (f->repr >> 0)] +
         wuffs_base__pixel_format__bits_per_channel[0x0F & (f->repr >> 4)] +
         wuffs_base__pixel_format__bits_per_channel[0x0F & (f->repr >> 8)] +
         wuffs_base__pixel_format__bits_per_channel[0x0F & (f->repr >> 12)];
}

static inline bool  //
wuffs_base__pixel_format__is_direct(const wuffs_base__pixel_format* f) {
  return ((f->repr >> 18) & 0x01) == 0;
}

static inline bool  //
wuffs_base__pixel_format__is_indexed(const wuffs_base__pixel_format* f) {
  return ((f->repr >> 18) & 0x01) != 0;
}

static inline bool  //
wuffs_base__pixel_format__is_interleaved(const wuffs_base__pixel_format* f) {
  return ((f->repr >> 16) & 0x03) == 0;
}

static inline bool  //
wuffs_base__pixel_format__is_planar(const wuffs_base__pixel_format* f) {
  return ((f->repr >> 16) & 0x03) != 0;
}

static inline uint32_t  //
wuffs_base__pixel_format__num_planes(const wuffs_base__pixel_format* f) {
  return ((f->repr >> 16) & 0x03) + 1;
}

static inline wuffs_base__pixel_alpha_transparency  //
wuffs_base__pixel_format__transparency(const wuffs_base__pixel_format* f) {
  return (wuffs_base__pixel_alpha_transparency)((f->repr >> 24) & 0x03);
}

#ifdef __cplusplus

inline bool  //
wuffs_base__pixel_format::is_valid() const {
  return wuffs_base__pixel_format__is_valid(this);
}

inline uint32_t  //
wuffs_base__pixel_format::bits_per_pixel() const {
  return wuffs_base__pixel_format__bits_per_pixel(this);
}

inline bool  //
wuffs_base__pixel_format::is_direct() const {
  return wuffs_base__pixel_format__is_direct(this);
}

inline bool  //
wuffs_base__pixel_format::is_indexed() const {
  return wuffs_base__pixel_format__is_indexed(this);
}

inline bool  //
wuffs_base__pixel_format::is_interleaved() const {
  return wuffs_base__pixel_format__is_interleaved(this);
}

inline bool  //
wuffs_base__pixel_format::is_planar() const {
  return wuffs_base__pixel_format__is_planar(this);
}

inline uint32_t  //
wuffs_base__pixel_format::num_planes() const {
  return wuffs_base__pixel_format__num_planes(this);
}

inline wuffs_base__pixel_alpha_transparency  //
wuffs_base__pixel_format::transparency() const {
  return wuffs_base__pixel_format__transparency(this);
}

#endif  // __cplusplus

// --------

// wuffs_base__pixel_subsampling encodes whether sample values cover one pixel
// or cover multiple pixels.
//
// See https://github.com/google/wuffs/blob/master/doc/note/pixel-subsampling.md
//
// Do not manipulate its bits directly; they are private implementation
// details. Use methods such as wuffs_base__pixel_subsampling__bias_x instead.
typedef struct {
  uint32_t repr;

#ifdef __cplusplus
  inline uint32_t bias_x(uint32_t plane) const;
  inline uint32_t denominator_x(uint32_t plane) const;
  inline uint32_t bias_y(uint32_t plane) const;
  inline uint32_t denominator_y(uint32_t plane) const;
#endif  // __cplusplus

} wuffs_base__pixel_subsampling;

static inline wuffs_base__pixel_subsampling  //
wuffs_base__make_pixel_subsampling(uint32_t repr) {
  wuffs_base__pixel_subsampling s;
  s.repr = repr;
  return s;
}

#define WUFFS_BASE__PIXEL_SUBSAMPLING__NONE 0x00000000

#define WUFFS_BASE__PIXEL_SUBSAMPLING__444 0x000000
#define WUFFS_BASE__PIXEL_SUBSAMPLING__440 0x010100
#define WUFFS_BASE__PIXEL_SUBSAMPLING__422 0x101000
#define WUFFS_BASE__PIXEL_SUBSAMPLING__420 0x111100
#define WUFFS_BASE__PIXEL_SUBSAMPLING__411 0x303000
#define WUFFS_BASE__PIXEL_SUBSAMPLING__410 0x313100

static inline uint32_t  //
wuffs_base__pixel_subsampling__bias_x(const wuffs_base__pixel_subsampling* s,
                                      uint32_t plane) {
  uint32_t shift = ((plane & 0x03) * 8) + 6;
  return (s->repr >> shift) & 0x03;
}

static inline uint32_t  //
wuffs_base__pixel_subsampling__denominator_x(
    const wuffs_base__pixel_subsampling* s,
    uint32_t plane) {
  uint32_t shift = ((plane & 0x03) * 8) + 4;
  return ((s->repr >> shift) & 0x03) + 1;
}

static inline uint32_t  //
wuffs_base__pixel_subsampling__bias_y(const wuffs_base__pixel_subsampling* s,
                                      uint32_t plane) {
  uint32_t shift = ((plane & 0x03) * 8) + 2;
  return (s->repr >> shift) & 0x03;
}

static inline uint32_t  //
wuffs_base__pixel_subsampling__denominator_y(
    const wuffs_base__pixel_subsampling* s,
    uint32_t plane) {
  uint32_t shift = ((plane & 0x03) * 8) + 0;
  return ((s->repr >> shift) & 0x03) + 1;
}

#ifdef __cplusplus

inline uint32_t  //
wuffs_base__pixel_subsampling::bias_x(uint32_t plane) const {
  return wuffs_base__pixel_subsampling__bias_x(this, plane);
}

inline uint32_t  //
wuffs_base__pixel_subsampling::denominator_x(uint32_t plane) const {
  return wuffs_base__pixel_subsampling__denominator_x(this, plane);
}

inline uint32_t  //
wuffs_base__pixel_subsampling::bias_y(uint32_t plane) const {
  return wuffs_base__pixel_subsampling__bias_y(this, plane);
}

inline uint32_t  //
wuffs_base__pixel_subsampling::denominator_y(uint32_t plane) const {
  return wuffs_base__pixel_subsampling__denominator_y(this, plane);
}

#endif  // __cplusplus

// --------

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    wuffs_base__pixel_format pixfmt;
    wuffs_base__pixel_subsampling pixsub;
    uint32_t width;
    uint32_t height;
  } private_impl;

#ifdef __cplusplus
  inline void set(uint32_t pixfmt_repr,
                  uint32_t pixsub_repr,
                  uint32_t width,
                  uint32_t height);
  inline void invalidate();
  inline bool is_valid() const;
  inline wuffs_base__pixel_format pixel_format() const;
  inline wuffs_base__pixel_subsampling pixel_subsampling() const;
  inline wuffs_base__rect_ie_u32 bounds() const;
  inline uint32_t width() const;
  inline uint32_t height() const;
  inline uint64_t pixbuf_len() const;
#endif  // __cplusplus

} wuffs_base__pixel_config;

static inline wuffs_base__pixel_config  //
wuffs_base__null_pixel_config() {
  wuffs_base__pixel_config ret;
  ret.private_impl.pixfmt.repr = 0;
  ret.private_impl.pixsub.repr = 0;
  ret.private_impl.width = 0;
  ret.private_impl.height = 0;
  return ret;
}

// TODO: Should this function return bool? An error type?
static inline void  //
wuffs_base__pixel_config__set(wuffs_base__pixel_config* c,
                              uint32_t pixfmt_repr,
                              uint32_t pixsub_repr,
                              uint32_t width,
                              uint32_t height) {
  if (!c) {
    return;
  }
  if (pixfmt_repr) {
    uint64_t wh = ((uint64_t)width) * ((uint64_t)height);
    // TODO: handle things other than 1 byte per pixel.
    if (wh <= ((uint64_t)SIZE_MAX)) {
      c->private_impl.pixfmt.repr = pixfmt_repr;
      c->private_impl.pixsub.repr = pixsub_repr;
      c->private_impl.width = width;
      c->private_impl.height = height;
      return;
    }
  }

  c->private_impl.pixfmt.repr = 0;
  c->private_impl.pixsub.repr = 0;
  c->private_impl.width = 0;
  c->private_impl.height = 0;
}

static inline void  //
wuffs_base__pixel_config__invalidate(wuffs_base__pixel_config* c) {
  if (c) {
    c->private_impl.pixfmt.repr = 0;
    c->private_impl.pixsub.repr = 0;
    c->private_impl.width = 0;
    c->private_impl.height = 0;
  }
}

static inline bool  //
wuffs_base__pixel_config__is_valid(const wuffs_base__pixel_config* c) {
  return c && c->private_impl.pixfmt.repr;
}

static inline wuffs_base__pixel_format  //
wuffs_base__pixel_config__pixel_format(const wuffs_base__pixel_config* c) {
  return c ? c->private_impl.pixfmt : wuffs_base__make_pixel_format(0);
}

static inline wuffs_base__pixel_subsampling  //
wuffs_base__pixel_config__pixel_subsampling(const wuffs_base__pixel_config* c) {
  return c ? c->private_impl.pixsub : wuffs_base__make_pixel_subsampling(0);
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__pixel_config__bounds(const wuffs_base__pixel_config* c) {
  if (c) {
    wuffs_base__rect_ie_u32 ret;
    ret.min_incl_x = 0;
    ret.min_incl_y = 0;
    ret.max_excl_x = c->private_impl.width;
    ret.max_excl_y = c->private_impl.height;
    return ret;
  }

  wuffs_base__rect_ie_u32 ret;
  ret.min_incl_x = 0;
  ret.min_incl_y = 0;
  ret.max_excl_x = 0;
  ret.max_excl_y = 0;
  return ret;
}

static inline uint32_t  //
wuffs_base__pixel_config__width(const wuffs_base__pixel_config* c) {
  return c ? c->private_impl.width : 0;
}

static inline uint32_t  //
wuffs_base__pixel_config__height(const wuffs_base__pixel_config* c) {
  return c ? c->private_impl.height : 0;
}

// TODO: this is the right API for planar (not interleaved) pixbufs? Should it
// allow decoding into a color model different from the format's intrinsic one?
// For example, decoding a JPEG image straight to RGBA instead of to YCbCr?
static inline uint64_t  //
wuffs_base__pixel_config__pixbuf_len(const wuffs_base__pixel_config* c) {
  if (!c) {
    return 0;
  }
  if (wuffs_base__pixel_format__is_planar(&c->private_impl.pixfmt)) {
    // TODO: support planar pixel formats, concious of pixel subsampling.
    return 0;
  }
  uint32_t bits_per_pixel =
      wuffs_base__pixel_format__bits_per_pixel(&c->private_impl.pixfmt);
  if ((bits_per_pixel == 0) || ((bits_per_pixel % 8) != 0)) {
    // TODO: support fraction-of-byte pixels, e.g. 1 bit per pixel?
    return 0;
  }
  uint64_t bytes_per_pixel = bits_per_pixel / 8;

  uint64_t n =
      ((uint64_t)c->private_impl.width) * ((uint64_t)c->private_impl.height);
  if (n > (UINT64_MAX / bytes_per_pixel)) {
    return 0;
  }
  n *= bytes_per_pixel;

  if (wuffs_base__pixel_format__is_indexed(&c->private_impl.pixfmt)) {
    if (n > (UINT64_MAX - 1024)) {
      return 0;
    }
    n += 1024;
  }

  return n;
}

#ifdef __cplusplus

inline void  //
wuffs_base__pixel_config::set(uint32_t pixfmt_repr,
                              uint32_t pixsub_repr,
                              uint32_t width,
                              uint32_t height) {
  wuffs_base__pixel_config__set(this, pixfmt_repr, pixsub_repr, width, height);
}

inline void  //
wuffs_base__pixel_config::invalidate() {
  wuffs_base__pixel_config__invalidate(this);
}

inline bool  //
wuffs_base__pixel_config::is_valid() const {
  return wuffs_base__pixel_config__is_valid(this);
}

inline wuffs_base__pixel_format  //
wuffs_base__pixel_config::pixel_format() const {
  return wuffs_base__pixel_config__pixel_format(this);
}

inline wuffs_base__pixel_subsampling  //
wuffs_base__pixel_config::pixel_subsampling() const {
  return wuffs_base__pixel_config__pixel_subsampling(this);
}

inline wuffs_base__rect_ie_u32  //
wuffs_base__pixel_config::bounds() const {
  return wuffs_base__pixel_config__bounds(this);
}

inline uint32_t  //
wuffs_base__pixel_config::width() const {
  return wuffs_base__pixel_config__width(this);
}

inline uint32_t  //
wuffs_base__pixel_config::height() const {
  return wuffs_base__pixel_config__height(this);
}

inline uint64_t  //
wuffs_base__pixel_config::pixbuf_len() const {
  return wuffs_base__pixel_config__pixbuf_len(this);
}

#endif  // __cplusplus

// --------

typedef struct {
  wuffs_base__pixel_config pixcfg;

  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    uint64_t first_frame_io_position;
    bool first_frame_is_opaque;
  } private_impl;

#ifdef __cplusplus
  inline void set(uint32_t pixfmt_repr,
                  uint32_t pixsub_repr,
                  uint32_t width,
                  uint32_t height,
                  uint64_t first_frame_io_position,
                  bool first_frame_is_opaque);
  inline void invalidate();
  inline bool is_valid() const;
  inline uint64_t first_frame_io_position() const;
  inline bool first_frame_is_opaque() const;
#endif  // __cplusplus

} wuffs_base__image_config;

static inline wuffs_base__image_config  //
wuffs_base__null_image_config() {
  wuffs_base__image_config ret;
  ret.pixcfg = wuffs_base__null_pixel_config();
  ret.private_impl.first_frame_io_position = 0;
  ret.private_impl.first_frame_is_opaque = false;
  return ret;
}

// TODO: Should this function return bool? An error type?
static inline void  //
wuffs_base__image_config__set(wuffs_base__image_config* c,
                              uint32_t pixfmt_repr,
                              uint32_t pixsub_repr,
                              uint32_t width,
                              uint32_t height,
                              uint64_t first_frame_io_position,
                              bool first_frame_is_opaque) {
  if (!c) {
    return;
  }
  if (pixfmt_repr) {
    c->pixcfg.private_impl.pixfmt.repr = pixfmt_repr;
    c->pixcfg.private_impl.pixsub.repr = pixsub_repr;
    c->pixcfg.private_impl.width = width;
    c->pixcfg.private_impl.height = height;
    c->private_impl.first_frame_io_position = first_frame_io_position;
    c->private_impl.first_frame_is_opaque = first_frame_is_opaque;
    return;
  }

  c->pixcfg.private_impl.pixfmt.repr = 0;
  c->pixcfg.private_impl.pixsub.repr = 0;
  c->pixcfg.private_impl.width = 0;
  c->pixcfg.private_impl.height = 0;
  c->private_impl.first_frame_io_position = 0;
  c->private_impl.first_frame_is_opaque = 0;
}

static inline void  //
wuffs_base__image_config__invalidate(wuffs_base__image_config* c) {
  if (c) {
    c->pixcfg.private_impl.pixfmt.repr = 0;
    c->pixcfg.private_impl.pixsub.repr = 0;
    c->pixcfg.private_impl.width = 0;
    c->pixcfg.private_impl.height = 0;
    c->private_impl.first_frame_io_position = 0;
    c->private_impl.first_frame_is_opaque = 0;
  }
}

static inline bool  //
wuffs_base__image_config__is_valid(const wuffs_base__image_config* c) {
  return c && wuffs_base__pixel_config__is_valid(&(c->pixcfg));
}

static inline uint64_t  //
wuffs_base__image_config__first_frame_io_position(
    const wuffs_base__image_config* c) {
  return c ? c->private_impl.first_frame_io_position : 0;
}

static inline bool  //
wuffs_base__image_config__first_frame_is_opaque(
    const wuffs_base__image_config* c) {
  return c ? c->private_impl.first_frame_is_opaque : false;
}

#ifdef __cplusplus

inline void  //
wuffs_base__image_config::set(uint32_t pixfmt_repr,
                              uint32_t pixsub_repr,
                              uint32_t width,
                              uint32_t height,
                              uint64_t first_frame_io_position,
                              bool first_frame_is_opaque) {
  wuffs_base__image_config__set(this, pixfmt_repr, pixsub_repr, width, height,
                                first_frame_io_position, first_frame_is_opaque);
}

inline void  //
wuffs_base__image_config::invalidate() {
  wuffs_base__image_config__invalidate(this);
}

inline bool  //
wuffs_base__image_config::is_valid() const {
  return wuffs_base__image_config__is_valid(this);
}

inline uint64_t  //
wuffs_base__image_config::first_frame_io_position() const {
  return wuffs_base__image_config__first_frame_io_position(this);
}

inline bool  //
wuffs_base__image_config::first_frame_is_opaque() const {
  return wuffs_base__image_config__first_frame_is_opaque(this);
}

#endif  // __cplusplus

// --------

// Deprecated: use wuffs_base__pixel_blend instead.
//
// wuffs_base__animation_blend encodes, for an animated image, how to blend the
// transparent pixels of this frame with the existing canvas. In Porter-Duff
// compositing operator terminology:
//  - 0 means the frame may be transparent, and should be blended "src over
//    dst", also known as just "over".
//  - 1 means the frame may be transparent, and should be blended "src".
//  - 2 means the frame is completely opaque, so that "src over dst" and "src"
//    are equivalent.
//
// These semantics are conservative. It is valid for a completely opaque frame
// to have a blend value other than 2.
typedef uint8_t wuffs_base__animation_blend;

#define WUFFS_BASE__ANIMATION_BLEND__SRC_OVER_DST \
  ((wuffs_base__animation_blend)0)
#define WUFFS_BASE__ANIMATION_BLEND__SRC ((wuffs_base__animation_blend)1)
#define WUFFS_BASE__ANIMATION_BLEND__OPAQUE ((wuffs_base__animation_blend)2)

// --------

// wuffs_base__animation_disposal encodes, for an animated image, how to
// dispose of a frame after displaying it:
//  - None means to draw the next frame on top of this one.
//  - Restore Background means to clear the frame's dirty rectangle to "the
//    background color" (in practice, this means transparent black) before
//    drawing the next frame.
//  - Restore Previous means to undo the current frame, so that the next frame
//    is drawn on top of the previous one.
typedef uint8_t wuffs_base__animation_disposal;

#define WUFFS_BASE__ANIMATION_DISPOSAL__NONE ((wuffs_base__animation_disposal)0)
#define WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_BACKGROUND \
  ((wuffs_base__animation_disposal)1)
#define WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS \
  ((wuffs_base__animation_disposal)2)

// --------

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    wuffs_base__rect_ie_u32 bounds;
    wuffs_base__flicks duration;
    uint64_t index;
    uint64_t io_position;
    wuffs_base__animation_disposal disposal;
    bool opaque_within_bounds;
    bool overwrite_instead_of_blend;
    wuffs_base__color_u32_argb_premul background_color;
  } private_impl;

#ifdef __cplusplus
  inline void set(wuffs_base__rect_ie_u32 bounds,
                  wuffs_base__flicks duration,
                  uint64_t index,
                  uint64_t io_position,
                  wuffs_base__animation_disposal disposal,
                  bool opaque_within_bounds,
                  bool overwrite_instead_of_blend,
                  wuffs_base__color_u32_argb_premul background_color);
  inline wuffs_base__rect_ie_u32 bounds() const;
  inline uint32_t width() const;
  inline uint32_t height() const;
  inline wuffs_base__flicks duration() const;
  inline uint64_t index() const;
  inline uint64_t io_position() const;
  inline wuffs_base__animation_disposal disposal() const;
  inline bool opaque_within_bounds() const;
  inline bool overwrite_instead_of_blend() const;
  inline wuffs_base__color_u32_argb_premul background_color() const;
#endif  // __cplusplus

} wuffs_base__frame_config;

static inline wuffs_base__frame_config  //
wuffs_base__null_frame_config() {
  wuffs_base__frame_config ret;
  ret.private_impl.bounds = wuffs_base__make_rect_ie_u32(0, 0, 0, 0);
  ret.private_impl.duration = 0;
  ret.private_impl.index = 0;
  ret.private_impl.io_position = 0;
  ret.private_impl.disposal = 0;
  ret.private_impl.opaque_within_bounds = false;
  ret.private_impl.overwrite_instead_of_blend = false;
  return ret;
}

static inline void  //
wuffs_base__frame_config__set(
    wuffs_base__frame_config* c,
    wuffs_base__rect_ie_u32 bounds,
    wuffs_base__flicks duration,
    uint64_t index,
    uint64_t io_position,
    wuffs_base__animation_disposal disposal,
    bool opaque_within_bounds,
    bool overwrite_instead_of_blend,
    wuffs_base__color_u32_argb_premul background_color) {
  if (!c) {
    return;
  }

  c->private_impl.bounds = bounds;
  c->private_impl.duration = duration;
  c->private_impl.index = index;
  c->private_impl.io_position = io_position;
  c->private_impl.disposal = disposal;
  c->private_impl.opaque_within_bounds = opaque_within_bounds;
  c->private_impl.overwrite_instead_of_blend = overwrite_instead_of_blend;
  c->private_impl.background_color = background_color;
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__frame_config__bounds(const wuffs_base__frame_config* c) {
  if (c) {
    return c->private_impl.bounds;
  }

  wuffs_base__rect_ie_u32 ret;
  ret.min_incl_x = 0;
  ret.min_incl_y = 0;
  ret.max_excl_x = 0;
  ret.max_excl_y = 0;
  return ret;
}

static inline uint32_t  //
wuffs_base__frame_config__width(const wuffs_base__frame_config* c) {
  return c ? wuffs_base__rect_ie_u32__width(&c->private_impl.bounds) : 0;
}

static inline uint32_t  //
wuffs_base__frame_config__height(const wuffs_base__frame_config* c) {
  return c ? wuffs_base__rect_ie_u32__height(&c->private_impl.bounds) : 0;
}

// wuffs_base__frame_config__duration returns the amount of time to display
// this frame. Zero means to display forever - a still (non-animated) image.
static inline wuffs_base__flicks  //
wuffs_base__frame_config__duration(const wuffs_base__frame_config* c) {
  return c ? c->private_impl.duration : 0;
}

// wuffs_base__frame_config__index returns the index of this frame. The first
// frame in an image has index 0, the second frame has index 1, and so on.
static inline uint64_t  //
wuffs_base__frame_config__index(const wuffs_base__frame_config* c) {
  return c ? c->private_impl.index : 0;
}

// wuffs_base__frame_config__io_position returns the I/O stream position before
// the frame config.
static inline uint64_t  //
wuffs_base__frame_config__io_position(const wuffs_base__frame_config* c) {
  return c ? c->private_impl.io_position : 0;
}

// wuffs_base__frame_config__disposal returns, for an animated image, how to
// dispose of this frame after displaying it.
static inline wuffs_base__animation_disposal  //
wuffs_base__frame_config__disposal(const wuffs_base__frame_config* c) {
  return c ? c->private_impl.disposal : 0;
}

// wuffs_base__frame_config__opaque_within_bounds returns whether all pixels
// within the frame's bounds are fully opaque. It makes no claim about pixels
// outside the frame bounds but still inside the overall image. The two
// bounding rectangles can differ for animated images.
//
// Its semantics are conservative. It is valid for a fully opaque frame to have
// this value be false: a false negative.
//
// If true, drawing the frame with WUFFS_BASE__PIXEL_BLEND__SRC and
// WUFFS_BASE__PIXEL_BLEND__SRC_OVER should be equivalent, in terms of
// resultant pixels, but the former may be faster.
static inline bool  //
wuffs_base__frame_config__opaque_within_bounds(
    const wuffs_base__frame_config* c) {
  return c && c->private_impl.opaque_within_bounds;
}

// wuffs_base__frame_config__overwrite_instead_of_blend returns, for an
// animated image, whether to ignore the previous image state (within the frame
// bounds) when drawing this incremental frame. Equivalently, whether to use
// WUFFS_BASE__PIXEL_BLEND__SRC instead of WUFFS_BASE__PIXEL_BLEND__SRC_OVER.
//
// The WebP spec (https://developers.google.com/speed/webp/docs/riff_container)
// calls this the "Blending method" bit. WebP's "Do not blend" corresponds to
// Wuffs' "overwrite_instead_of_blend".
static inline bool  //
wuffs_base__frame_config__overwrite_instead_of_blend(
    const wuffs_base__frame_config* c) {
  return c && c->private_impl.overwrite_instead_of_blend;
}

static inline wuffs_base__color_u32_argb_premul  //
wuffs_base__frame_config__background_color(const wuffs_base__frame_config* c) {
  return c ? c->private_impl.background_color : 0;
}

#ifdef __cplusplus

inline void  //
wuffs_base__frame_config::set(
    wuffs_base__rect_ie_u32 bounds,
    wuffs_base__flicks duration,
    uint64_t index,
    uint64_t io_position,
    wuffs_base__animation_disposal disposal,
    bool opaque_within_bounds,
    bool overwrite_instead_of_blend,
    wuffs_base__color_u32_argb_premul background_color) {
  wuffs_base__frame_config__set(this, bounds, duration, index, io_position,
                                disposal, opaque_within_bounds,
                                overwrite_instead_of_blend, background_color);
}

inline wuffs_base__rect_ie_u32  //
wuffs_base__frame_config::bounds() const {
  return wuffs_base__frame_config__bounds(this);
}

inline uint32_t  //
wuffs_base__frame_config::width() const {
  return wuffs_base__frame_config__width(this);
}

inline uint32_t  //
wuffs_base__frame_config::height() const {
  return wuffs_base__frame_config__height(this);
}

inline wuffs_base__flicks  //
wuffs_base__frame_config::duration() const {
  return wuffs_base__frame_config__duration(this);
}

inline uint64_t  //
wuffs_base__frame_config::index() const {
  return wuffs_base__frame_config__index(this);
}

inline uint64_t  //
wuffs_base__frame_config::io_position() const {
  return wuffs_base__frame_config__io_position(this);
}

inline wuffs_base__animation_disposal  //
wuffs_base__frame_config::disposal() const {
  return wuffs_base__frame_config__disposal(this);
}

inline bool  //
wuffs_base__frame_config::opaque_within_bounds() const {
  return wuffs_base__frame_config__opaque_within_bounds(this);
}

inline bool  //
wuffs_base__frame_config::overwrite_instead_of_blend() const {
  return wuffs_base__frame_config__overwrite_instead_of_blend(this);
}

inline wuffs_base__color_u32_argb_premul  //
wuffs_base__frame_config::background_color() const {
  return wuffs_base__frame_config__background_color(this);
}

#endif  // __cplusplus

// --------

typedef struct {
  wuffs_base__pixel_config pixcfg;

  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    wuffs_base__table_u8 planes[WUFFS_BASE__PIXEL_FORMAT__NUM_PLANES_MAX];
    // TODO: color spaces.
  } private_impl;

#ifdef __cplusplus
  inline wuffs_base__status set_from_slice(
      const wuffs_base__pixel_config* pixcfg,
      wuffs_base__slice_u8 pixbuf_memory);
  inline wuffs_base__status set_from_table(
      const wuffs_base__pixel_config* pixcfg,
      wuffs_base__table_u8 pixbuf_memory);
  inline wuffs_base__slice_u8 palette();
  inline wuffs_base__pixel_format pixel_format() const;
  inline wuffs_base__table_u8 plane(uint32_t p);
  inline wuffs_base__color_u32_argb_premul color_u32_at(uint32_t x,
                                                        uint32_t y) const;
  inline wuffs_base__status set_color_u32_at(
      uint32_t x,
      uint32_t y,
      wuffs_base__color_u32_argb_premul color);
#endif  // __cplusplus

} wuffs_base__pixel_buffer;

static inline wuffs_base__pixel_buffer  //
wuffs_base__null_pixel_buffer() {
  wuffs_base__pixel_buffer ret;
  ret.pixcfg = wuffs_base__null_pixel_config();
  ret.private_impl.planes[0] = wuffs_base__empty_table_u8();
  ret.private_impl.planes[1] = wuffs_base__empty_table_u8();
  ret.private_impl.planes[2] = wuffs_base__empty_table_u8();
  ret.private_impl.planes[3] = wuffs_base__empty_table_u8();
  return ret;
}

static inline wuffs_base__status  //
wuffs_base__pixel_buffer__set_from_slice(wuffs_base__pixel_buffer* pb,
                                         const wuffs_base__pixel_config* pixcfg,
                                         wuffs_base__slice_u8 pixbuf_memory) {
  if (!pb) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  memset(pb, 0, sizeof(*pb));
  if (!pixcfg) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if (wuffs_base__pixel_format__is_planar(&pixcfg->private_impl.pixfmt)) {
    // TODO: support planar pixel formats, concious of pixel subsampling.
    return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }
  uint32_t bits_per_pixel =
      wuffs_base__pixel_format__bits_per_pixel(&pixcfg->private_impl.pixfmt);
  if ((bits_per_pixel == 0) || ((bits_per_pixel % 8) != 0)) {
    // TODO: support fraction-of-byte pixels, e.g. 1 bit per pixel?
    return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }
  uint64_t bytes_per_pixel = bits_per_pixel / 8;

  uint8_t* ptr = pixbuf_memory.ptr;
  uint64_t len = pixbuf_memory.len;
  if (wuffs_base__pixel_format__is_indexed(&pixcfg->private_impl.pixfmt)) {
    // Split a 1024 byte chunk (256 palette entries × 4 bytes per entry) from
    // the start of pixbuf_memory. We split from the start, not the end, so
    // that the both chunks' pointers have the same alignment as the original
    // pointer, up to an alignment of 1024.
    if (len < 1024) {
      return wuffs_base__make_status(
          wuffs_base__error__bad_argument_length_too_short);
    }
    wuffs_base__table_u8* tab =
        &pb->private_impl
             .planes[WUFFS_BASE__PIXEL_FORMAT__INDEXED__COLOR_PLANE];
    tab->ptr = ptr;
    tab->width = 1024;
    tab->height = 1;
    tab->stride = 1024;
    ptr += 1024;
    len -= 1024;
  }

  uint64_t wh = ((uint64_t)pixcfg->private_impl.width) *
                ((uint64_t)pixcfg->private_impl.height);
  size_t width = (size_t)(pixcfg->private_impl.width);
  if ((wh > (UINT64_MAX / bytes_per_pixel)) ||
      (width > (SIZE_MAX / bytes_per_pixel))) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  wh *= bytes_per_pixel;
  width *= bytes_per_pixel;
  if (wh > len) {
    return wuffs_base__make_status(
        wuffs_base__error__bad_argument_length_too_short);
  }

  pb->pixcfg = *pixcfg;
  wuffs_base__table_u8* tab = &pb->private_impl.planes[0];
  tab->ptr = ptr;
  tab->width = width;
  tab->height = pixcfg->private_impl.height;
  tab->stride = width;
  return wuffs_base__make_status(NULL);
}

static inline wuffs_base__status  //
wuffs_base__pixel_buffer__set_from_table(wuffs_base__pixel_buffer* pb,
                                         const wuffs_base__pixel_config* pixcfg,
                                         wuffs_base__table_u8 pixbuf_memory) {
  if (!pb) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  memset(pb, 0, sizeof(*pb));
  if (!pixcfg ||
      wuffs_base__pixel_format__is_planar(&pixcfg->private_impl.pixfmt)) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  uint32_t bits_per_pixel =
      wuffs_base__pixel_format__bits_per_pixel(&pixcfg->private_impl.pixfmt);
  if ((bits_per_pixel == 0) || ((bits_per_pixel % 8) != 0)) {
    // TODO: support fraction-of-byte pixels, e.g. 1 bit per pixel?
    return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }
  uint64_t bytes_per_pixel = bits_per_pixel / 8;

  uint64_t width_in_bytes =
      ((uint64_t)pixcfg->private_impl.width) * bytes_per_pixel;
  if ((width_in_bytes > pixbuf_memory.width) ||
      (pixcfg->private_impl.height > pixbuf_memory.height)) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }

  pb->pixcfg = *pixcfg;
  pb->private_impl.planes[0] = pixbuf_memory;
  return wuffs_base__make_status(NULL);
}

// wuffs_base__pixel_buffer__palette returns the palette color data. If
// non-empty, it will have length 1024.
static inline wuffs_base__slice_u8  //
wuffs_base__pixel_buffer__palette(wuffs_base__pixel_buffer* pb) {
  if (pb &&
      wuffs_base__pixel_format__is_indexed(&pb->pixcfg.private_impl.pixfmt)) {
    wuffs_base__table_u8* tab =
        &pb->private_impl
             .planes[WUFFS_BASE__PIXEL_FORMAT__INDEXED__COLOR_PLANE];
    if ((tab->width == 1024) && (tab->height == 1)) {
      return wuffs_base__make_slice_u8(tab->ptr, 1024);
    }
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

static inline wuffs_base__pixel_format  //
wuffs_base__pixel_buffer__pixel_format(const wuffs_base__pixel_buffer* pb) {
  if (pb) {
    return pb->pixcfg.private_impl.pixfmt;
  }
  return wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__INVALID);
}

static inline wuffs_base__table_u8  //
wuffs_base__pixel_buffer__plane(wuffs_base__pixel_buffer* pb, uint32_t p) {
  if (pb && (p < WUFFS_BASE__PIXEL_FORMAT__NUM_PLANES_MAX)) {
    return pb->private_impl.planes[p];
  }

  wuffs_base__table_u8 ret;
  ret.ptr = NULL;
  ret.width = 0;
  ret.height = 0;
  ret.stride = 0;
  return ret;
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__color_u32_argb_premul  //
wuffs_base__pixel_buffer__color_u32_at(const wuffs_base__pixel_buffer* pb,
                                       uint32_t x,
                                       uint32_t y);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status  //
wuffs_base__pixel_buffer__set_color_u32_at(
    wuffs_base__pixel_buffer* pb,
    uint32_t x,
    uint32_t y,
    wuffs_base__color_u32_argb_premul color);

#ifdef __cplusplus

inline wuffs_base__status  //
wuffs_base__pixel_buffer::set_from_slice(
    const wuffs_base__pixel_config* pixcfg_arg,
    wuffs_base__slice_u8 pixbuf_memory) {
  return wuffs_base__pixel_buffer__set_from_slice(this, pixcfg_arg,
                                                  pixbuf_memory);
}

inline wuffs_base__status  //
wuffs_base__pixel_buffer::set_from_table(
    const wuffs_base__pixel_config* pixcfg_arg,
    wuffs_base__table_u8 pixbuf_memory) {
  return wuffs_base__pixel_buffer__set_from_table(this, pixcfg_arg,
                                                  pixbuf_memory);
}

inline wuffs_base__slice_u8  //
wuffs_base__pixel_buffer::palette() {
  return wuffs_base__pixel_buffer__palette(this);
}

inline wuffs_base__pixel_format  //
wuffs_base__pixel_buffer::pixel_format() const {
  return wuffs_base__pixel_buffer__pixel_format(this);
}

inline wuffs_base__table_u8  //
wuffs_base__pixel_buffer::plane(uint32_t p) {
  return wuffs_base__pixel_buffer__plane(this, p);
}

inline wuffs_base__color_u32_argb_premul  //
wuffs_base__pixel_buffer::color_u32_at(uint32_t x, uint32_t y) const {
  return wuffs_base__pixel_buffer__color_u32_at(this, x, y);
}

inline wuffs_base__status  //
wuffs_base__pixel_buffer::set_color_u32_at(
    uint32_t x,
    uint32_t y,
    wuffs_base__color_u32_argb_premul color) {
  return wuffs_base__pixel_buffer__set_color_u32_at(this, x, y, color);
}

#endif  // __cplusplus

// --------

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    uint8_t TODO;
  } private_impl;

#ifdef __cplusplus
#endif  // __cplusplus

} wuffs_base__decode_frame_options;

#ifdef __cplusplus

#endif  // __cplusplus

// --------

// wuffs_base__pixel_palette__closest_element returns the index of the palette
// element that minimizes the sum of squared differences of the four ARGB
// channels, working in premultiplied alpha. Ties favor the smaller index.
//
// The palette_slice.len may equal (N*4), for N less than 256, which means that
// only the first N palette elements are considered. It returns 0 when N is 0.
//
// Applying this function on a per-pixel basis will not produce whole-of-image
// dithering.
WUFFS_BASE__MAYBE_STATIC uint8_t  //
wuffs_base__pixel_palette__closest_element(
    wuffs_base__slice_u8 palette_slice,
    wuffs_base__pixel_format palette_format,
    wuffs_base__color_u32_argb_premul c);

// --------

// TODO: should the func type take restrict pointers?
typedef uint64_t (*wuffs_base__pixel_swizzler__func)(uint8_t* dst_ptr,
                                                     size_t dst_len,
                                                     uint8_t* dst_palette_ptr,
                                                     size_t dst_palette_len,
                                                     const uint8_t* src_ptr,
                                                     size_t src_len);

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    wuffs_base__pixel_swizzler__func func;
    uint32_t src_pixfmt_bytes_per_pixel;
  } private_impl;

#ifdef __cplusplus
  inline wuffs_base__status prepare(wuffs_base__pixel_format dst_pixfmt,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__pixel_format src_pixfmt,
                                    wuffs_base__slice_u8 src_palette,
                                    wuffs_base__pixel_blend blend);
  inline uint64_t swizzle_interleaved_from_slice(
      wuffs_base__slice_u8 dst,
      wuffs_base__slice_u8 dst_palette,
      wuffs_base__slice_u8 src) const;
#endif  // __cplusplus

} wuffs_base__pixel_swizzler;

// wuffs_base__pixel_swizzler__prepare readies the pixel swizzler so that its
// other methods may be called.
//
// For modular builds that divide the base module into sub-modules, using this
// function requires the WUFFS_CONFIG__MODULE__BASE__PIXCONV sub-module, not
// just WUFFS_CONFIG__MODULE__BASE__CORE.
WUFFS_BASE__MAYBE_STATIC wuffs_base__status  //
wuffs_base__pixel_swizzler__prepare(wuffs_base__pixel_swizzler* p,
                                    wuffs_base__pixel_format dst_pixfmt,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__pixel_format src_pixfmt,
                                    wuffs_base__slice_u8 src_palette,
                                    wuffs_base__pixel_blend blend);

// wuffs_base__pixel_swizzler__swizzle_interleaved_from_slice converts pixels
// from a source format to a destination format.
//
// For modular builds that divide the base module into sub-modules, using this
// function requires the WUFFS_CONFIG__MODULE__BASE__PIXCONV sub-module, not
// just WUFFS_CONFIG__MODULE__BASE__CORE.
WUFFS_BASE__MAYBE_STATIC uint64_t  //
wuffs_base__pixel_swizzler__swizzle_interleaved_from_slice(
    const wuffs_base__pixel_swizzler* p,
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src);

#ifdef __cplusplus

inline wuffs_base__status  //
wuffs_base__pixel_swizzler::prepare(wuffs_base__pixel_format dst_pixfmt,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__pixel_format src_pixfmt,
                                    wuffs_base__slice_u8 src_palette,
                                    wuffs_base__pixel_blend blend) {
  return wuffs_base__pixel_swizzler__prepare(this, dst_pixfmt, dst_palette,
                                             src_pixfmt, src_palette, blend);
}

uint64_t  //
wuffs_base__pixel_swizzler::swizzle_interleaved_from_slice(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) const {
  return wuffs_base__pixel_swizzler__swizzle_interleaved_from_slice(
      this, dst, dst_palette, src);
}

#endif  // __cplusplus

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

// ---------------- Interface Declarations.

// For modular builds that divide the base module into sub-modules, using these
// functions require the WUFFS_CONFIG__MODULE__BASE__INTERFACES sub-module, not
// just WUFFS_CONFIG__MODULE__BASE__CORE.

// --------

extern const char* wuffs_base__hasher_u32__vtable_name;

typedef struct {
  wuffs_base__empty_struct (*set_quirk_enabled)(
    void* self,
    uint32_t a_quirk,
    bool a_enabled);
  uint32_t (*update_u32)(
    void* self,
    wuffs_base__slice_u8 a_x);
} wuffs_base__hasher_u32__func_ptrs;

typedef struct wuffs_base__hasher_u32__struct wuffs_base__hasher_u32;

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_base__hasher_u32__set_quirk_enabled(
    wuffs_base__hasher_u32* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_base__hasher_u32__update_u32(
    wuffs_base__hasher_u32* self,
    wuffs_base__slice_u8 a_x);

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_base__hasher_u32__struct {
  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable first_vtable;
  } private_impl;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_base__hasher_u32, decltype(&free)>;
#endif

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_base__hasher_u32__set_quirk_enabled(
        this, a_quirk, a_enabled);
  }

  inline uint32_t
  update_u32(
      wuffs_base__slice_u8 a_x) {
    return wuffs_base__hasher_u32__update_u32(
        this, a_x);
  }

#endif  // __cplusplus
};  // struct wuffs_base__hasher_u32__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

// --------

extern const char* wuffs_base__image_decoder__vtable_name;

typedef struct {
  wuffs_base__status (*decode_frame)(
    void* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend,
    wuffs_base__slice_u8 a_workbuf,
    wuffs_base__decode_frame_options* a_opts);
  wuffs_base__status (*decode_frame_config)(
    void* self,
    wuffs_base__frame_config* a_dst,
    wuffs_base__io_buffer* a_src);
  wuffs_base__status (*decode_image_config)(
    void* self,
    wuffs_base__image_config* a_dst,
    wuffs_base__io_buffer* a_src);
  wuffs_base__rect_ie_u32 (*frame_dirty_rect)(
    const void* self);
  uint32_t (*num_animation_loops)(
    const void* self);
  uint64_t (*num_decoded_frame_configs)(
    const void* self);
  uint64_t (*num_decoded_frames)(
    const void* self);
  wuffs_base__status (*restart_frame)(
    void* self,
    uint64_t a_index,
    uint64_t a_io_position);
  wuffs_base__empty_struct (*set_quirk_enabled)(
    void* self,
    uint32_t a_quirk,
    bool a_enabled);
  wuffs_base__empty_struct (*set_report_metadata)(
    void* self,
    uint32_t a_fourcc,
    bool a_report);
  wuffs_base__status (*tell_me_more)(
    void* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__more_information* a_minfo,
    wuffs_base__io_buffer* a_src);
  wuffs_base__range_ii_u64 (*workbuf_len)(
    const void* self);
} wuffs_base__image_decoder__func_ptrs;

typedef struct wuffs_base__image_decoder__struct wuffs_base__image_decoder;

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__image_decoder__decode_frame(
    wuffs_base__image_decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend,
    wuffs_base__slice_u8 a_workbuf,
    wuffs_base__decode_frame_options* a_opts);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__image_decoder__decode_frame_config(
    wuffs_base__image_decoder* self,
    wuffs_base__frame_config* a_dst,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__image_decoder__decode_image_config(
    wuffs_base__image_decoder* self,
    wuffs_base__image_config* a_dst,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__rect_ie_u32
wuffs_base__image_decoder__frame_dirty_rect(
    const wuffs_base__image_decoder* self);

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_base__image_decoder__num_animation_loops(
    const wuffs_base__image_decoder* self);

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_base__image_decoder__num_decoded_frame_configs(
    const wuffs_base__image_decoder* self);

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_base__image_decoder__num_decoded_frames(
    const wuffs_base__image_decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__image_decoder__restart_frame(
    wuffs_base__image_decoder* self,
    uint64_t a_index,
    uint64_t a_io_position);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_base__image_decoder__set_quirk_enabled(
    wuffs_base__image_decoder* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_base__image_decoder__set_report_metadata(
    wuffs_base__image_decoder* self,
    uint32_t a_fourcc,
    bool a_report);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__image_decoder__tell_me_more(
    wuffs_base__image_decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__more_information* a_minfo,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_base__image_decoder__workbuf_len(
    const wuffs_base__image_decoder* self);

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_base__image_decoder__struct {
  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable first_vtable;
  } private_impl;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_base__image_decoder, decltype(&free)>;
#endif

  inline wuffs_base__status
  decode_frame(
      wuffs_base__pixel_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__pixel_blend a_blend,
      wuffs_base__slice_u8 a_workbuf,
      wuffs_base__decode_frame_options* a_opts) {
    return wuffs_base__image_decoder__decode_frame(
        this, a_dst, a_src, a_blend, a_workbuf, a_opts);
  }

  inline wuffs_base__status
  decode_frame_config(
      wuffs_base__frame_config* a_dst,
      wuffs_base__io_buffer* a_src) {
    return wuffs_base__image_decoder__decode_frame_config(
        this, a_dst, a_src);
  }

  inline wuffs_base__status
  decode_image_config(
      wuffs_base__image_config* a_dst,
      wuffs_base__io_buffer* a_src) {
    return wuffs_base__image_decoder__decode_image_config(
        this, a_dst, a_src);
  }

  inline wuffs_base__rect_ie_u32
  frame_dirty_rect() const {
    return wuffs_base__image_decoder__frame_dirty_rect(this);
  }

  inline uint32_t
  num_animation_loops() const {
    return wuffs_base__image_decoder__num_animation_loops(this);
  }

  inline uint64_t
  num_decoded_frame_configs() const {
    return wuffs_base__image_decoder__num_decoded_frame_configs(this);
  }

  inline uint64_t
  num_decoded_frames() const {
    return wuffs_base__image_decoder__num_decoded_frames(this);
  }

  inline wuffs_base__status
  restart_frame(
      uint64_t a_index,
      uint64_t a_io_position) {
    return wuffs_base__image_decoder__restart_frame(
        this, a_index, a_io_position);
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_base__image_decoder__set_quirk_enabled(
        this, a_quirk, a_enabled);
  }

  inline wuffs_base__empty_struct
  set_report_metadata(
      uint32_t a_fourcc,
      bool a_report) {
    return wuffs_base__image_decoder__set_report_metadata(
        this, a_fourcc, a_report);
  }

  inline wuffs_base__status
  tell_me_more(
      wuffs_base__io_buffer* a_dst,
      wuffs_base__more_information* a_minfo,
      wuffs_base__io_buffer* a_src) {
    return wuffs_base__image_decoder__tell_me_more(
        this, a_dst, a_minfo, a_src);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_base__image_decoder__workbuf_len(this);
  }

#endif  // __cplusplus
};  // struct wuffs_base__image_decoder__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

// --------

extern const char* wuffs_base__io_transformer__vtable_name;

typedef struct {
  wuffs_base__empty_struct (*set_quirk_enabled)(
    void* self,
    uint32_t a_quirk,
    bool a_enabled);
  wuffs_base__status (*transform_io)(
    void* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf);
  wuffs_base__range_ii_u64 (*workbuf_len)(
    const void* self);
} wuffs_base__io_transformer__func_ptrs;

typedef struct wuffs_base__io_transformer__struct wuffs_base__io_transformer;

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_base__io_transformer__set_quirk_enabled(
    wuffs_base__io_transformer* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__io_transformer__transform_io(
    wuffs_base__io_transformer* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_base__io_transformer__workbuf_len(
    const wuffs_base__io_transformer* self);

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_base__io_transformer__struct {
  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable first_vtable;
  } private_impl;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_base__io_transformer, decltype(&free)>;
#endif

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_base__io_transformer__set_quirk_enabled(
        this, a_quirk, a_enabled);
  }

  inline wuffs_base__status
  transform_io(
      wuffs_base__io_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__slice_u8 a_workbuf) {
    return wuffs_base__io_transformer__transform_io(
        this, a_dst, a_src, a_workbuf);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_base__io_transformer__workbuf_len(this);
  }

#endif  // __cplusplus
};  // struct wuffs_base__io_transformer__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

// --------

extern const char* wuffs_base__token_decoder__vtable_name;

typedef struct {
  wuffs_base__status (*decode_tokens)(
    void* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf);
  wuffs_base__empty_struct (*set_quirk_enabled)(
    void* self,
    uint32_t a_quirk,
    bool a_enabled);
  wuffs_base__range_ii_u64 (*workbuf_len)(
    const void* self);
} wuffs_base__token_decoder__func_ptrs;

typedef struct wuffs_base__token_decoder__struct wuffs_base__token_decoder;

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__token_decoder__decode_tokens(
    wuffs_base__token_decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_base__token_decoder__set_quirk_enabled(
    wuffs_base__token_decoder* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_base__token_decoder__workbuf_len(
    const wuffs_base__token_decoder* self);

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_base__token_decoder__struct {
  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable first_vtable;
  } private_impl;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_base__token_decoder, decltype(&free)>;
#endif

  inline wuffs_base__status
  decode_tokens(
      wuffs_base__token_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__slice_u8 a_workbuf) {
    return wuffs_base__token_decoder__decode_tokens(
        this, a_dst, a_src, a_workbuf);
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_base__token_decoder__set_quirk_enabled(
        this, a_quirk, a_enabled);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_base__token_decoder__workbuf_len(this);
  }

#endif  // __cplusplus
};  // struct wuffs_base__token_decoder__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

// ----------------

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Status Codes

// ---------------- Public Consts

// ---------------- Struct Declarations

typedef struct wuffs_adler32__hasher__struct wuffs_adler32__hasher;

// ---------------- Public Initializer Prototypes

// For any given "wuffs_foo__bar* self", "wuffs_foo__bar__initialize(self,
// etc)" should be called before any other "wuffs_foo__bar__xxx(self, etc)".
//
// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.
// Pass 0 (or some combination of WUFFS_INITIALIZE__XXX) for initialize_flags.

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_adler32__hasher__initialize(
    wuffs_adler32__hasher* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags);

size_t
sizeof__wuffs_adler32__hasher();

// ---------------- Allocs

// These functions allocate and initialize Wuffs structs. They return NULL if
// memory allocation fails. If they return non-NULL, there is no need to call
// wuffs_foo__bar__initialize, but the caller is responsible for eventually
// calling free on the returned pointer. That pointer is effectively a C++
// std::unique_ptr<T, decltype(&free)>.

wuffs_adler32__hasher*
wuffs_adler32__hasher__alloc();

static inline wuffs_base__hasher_u32*
wuffs_adler32__hasher__alloc_as__wuffs_base__hasher_u32() {
  return (wuffs_base__hasher_u32*)(wuffs_adler32__hasher__alloc());
}

// ---------------- Upcasts

static inline wuffs_base__hasher_u32*
wuffs_adler32__hasher__upcast_as__wuffs_base__hasher_u32(
    wuffs_adler32__hasher* p) {
  return (wuffs_base__hasher_u32*)p;
}

// ---------------- Public Function Prototypes

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_adler32__hasher__set_quirk_enabled(
    wuffs_adler32__hasher* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_adler32__hasher__update_u32(
    wuffs_adler32__hasher* self,
    wuffs_base__slice_u8 a_x);

// ---------------- Struct Definitions

// These structs' fields, and the sizeof them, are private implementation
// details that aren't guaranteed to be stable across Wuffs versions.
//
// See https://en.wikipedia.org/wiki/Opaque_pointer#C

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_adler32__hasher__struct {
  // Do not access the private_impl's or private_data's fields directly. There
  // is no API/ABI compatibility or safety guarantee if you do so. Instead, use
  // the wuffs_foo__bar__baz functions.
  //
  // It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct
  // can be stack allocated when WUFFS_IMPLEMENTATION is defined.

  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable vtable_for__wuffs_base__hasher_u32;
    wuffs_base__vtable null_vtable;

    uint32_t f_state;
    bool f_started;
  } private_impl;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_adler32__hasher, decltype(&free)>;

  // On failure, the alloc_etc functions return nullptr. They don't throw.

  static inline unique_ptr
  alloc() {
    return unique_ptr(wuffs_adler32__hasher__alloc(), &free);
  }

  static inline wuffs_base__hasher_u32::unique_ptr
  alloc_as__wuffs_base__hasher_u32() {
    return wuffs_base__hasher_u32::unique_ptr(
        wuffs_adler32__hasher__alloc_as__wuffs_base__hasher_u32(), &free);
  }
#endif  // (__cplusplus >= 201103L)

#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)
  // Disallow constructing or copying an object via standard C++ mechanisms,
  // e.g. the "new" operator, as this struct is intentionally opaque. Its total
  // size and field layout is not part of the public, stable, memory-safe API.
  // Use malloc or memcpy and the sizeof__wuffs_foo__bar function instead, and
  // call wuffs_foo__bar__baz methods (which all take a "this"-like pointer as
  // their first argument) rather than tweaking bar.private_impl.qux fields.
  //
  // In C, we can just leave wuffs_foo__bar as an incomplete type (unless
  // WUFFS_IMPLEMENTATION is #define'd). In C++, we define a complete type in
  // order to provide convenience methods. These forward on "this", so that you
  // can write "bar->baz(etc)" instead of "wuffs_foo__bar__baz(bar, etc)".
  wuffs_adler32__hasher__struct() = delete;
  wuffs_adler32__hasher__struct(const wuffs_adler32__hasher__struct&) = delete;
  wuffs_adler32__hasher__struct& operator=(
      const wuffs_adler32__hasher__struct&) = delete;

  // As above, the size of the struct is not part of the public API, and unless
  // WUFFS_IMPLEMENTATION is #define'd, this struct type T should be heap
  // allocated, not stack allocated. Its size is not intended to be known at
  // compile time, but it is unfortunately divulged as a side effect of
  // defining C++ convenience methods. Use "sizeof__T()", calling the function,
  // instead of "sizeof T", invoking the operator. To make the two values
  // different, so that passing the latter will be rejected by the initialize
  // function, we add an arbitrary amount of dead weight.
  uint8_t dead_weight[123000000];  // 123 MB.
#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)

  inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
  initialize(
      size_t sizeof_star_self,
      uint64_t wuffs_version,
      uint32_t initialize_flags) {
    return wuffs_adler32__hasher__initialize(
        this, sizeof_star_self, wuffs_version, initialize_flags);
  }

  inline wuffs_base__hasher_u32*
  upcast_as__wuffs_base__hasher_u32() {
    return (wuffs_base__hasher_u32*)this;
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_adler32__hasher__set_quirk_enabled(this, a_quirk, a_enabled);
  }

  inline uint32_t
  update_u32(
      wuffs_base__slice_u8 a_x) {
    return wuffs_adler32__hasher__update_u32(this, a_x);
  }

#endif  // __cplusplus
};  // struct wuffs_adler32__hasher__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Status Codes

extern const char* wuffs_bmp__error__bad_header;
extern const char* wuffs_bmp__error__unsupported_bmp_file;

// ---------------- Public Consts

#define WUFFS_BMP__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE 0

// ---------------- Struct Declarations

typedef struct wuffs_bmp__decoder__struct wuffs_bmp__decoder;

// ---------------- Public Initializer Prototypes

// For any given "wuffs_foo__bar* self", "wuffs_foo__bar__initialize(self,
// etc)" should be called before any other "wuffs_foo__bar__xxx(self, etc)".
//
// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.
// Pass 0 (or some combination of WUFFS_INITIALIZE__XXX) for initialize_flags.

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_bmp__decoder__initialize(
    wuffs_bmp__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags);

size_t
sizeof__wuffs_bmp__decoder();

// ---------------- Allocs

// These functions allocate and initialize Wuffs structs. They return NULL if
// memory allocation fails. If they return non-NULL, there is no need to call
// wuffs_foo__bar__initialize, but the caller is responsible for eventually
// calling free on the returned pointer. That pointer is effectively a C++
// std::unique_ptr<T, decltype(&free)>.

wuffs_bmp__decoder*
wuffs_bmp__decoder__alloc();

static inline wuffs_base__image_decoder*
wuffs_bmp__decoder__alloc_as__wuffs_base__image_decoder() {
  return (wuffs_base__image_decoder*)(wuffs_bmp__decoder__alloc());
}

// ---------------- Upcasts

static inline wuffs_base__image_decoder*
wuffs_bmp__decoder__upcast_as__wuffs_base__image_decoder(
    wuffs_bmp__decoder* p) {
  return (wuffs_base__image_decoder*)p;
}

// ---------------- Public Function Prototypes

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_bmp__decoder__set_quirk_enabled(
    wuffs_bmp__decoder* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_bmp__decoder__decode_image_config(
    wuffs_bmp__decoder* self,
    wuffs_base__image_config* a_dst,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_bmp__decoder__decode_frame_config(
    wuffs_bmp__decoder* self,
    wuffs_base__frame_config* a_dst,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_bmp__decoder__decode_frame(
    wuffs_bmp__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend,
    wuffs_base__slice_u8 a_workbuf,
    wuffs_base__decode_frame_options* a_opts);

WUFFS_BASE__MAYBE_STATIC wuffs_base__rect_ie_u32
wuffs_bmp__decoder__frame_dirty_rect(
    const wuffs_bmp__decoder* self);

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_bmp__decoder__num_animation_loops(
    const wuffs_bmp__decoder* self);

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_bmp__decoder__num_decoded_frame_configs(
    const wuffs_bmp__decoder* self);

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_bmp__decoder__num_decoded_frames(
    const wuffs_bmp__decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_bmp__decoder__restart_frame(
    wuffs_bmp__decoder* self,
    uint64_t a_index,
    uint64_t a_io_position);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_bmp__decoder__set_report_metadata(
    wuffs_bmp__decoder* self,
    uint32_t a_fourcc,
    bool a_report);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_bmp__decoder__tell_me_more(
    wuffs_bmp__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__more_information* a_minfo,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_bmp__decoder__workbuf_len(
    const wuffs_bmp__decoder* self);

// ---------------- Struct Definitions

// These structs' fields, and the sizeof them, are private implementation
// details that aren't guaranteed to be stable across Wuffs versions.
//
// See https://en.wikipedia.org/wiki/Opaque_pointer#C

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_bmp__decoder__struct {
  // Do not access the private_impl's or private_data's fields directly. There
  // is no API/ABI compatibility or safety guarantee if you do so. Instead, use
  // the wuffs_foo__bar__baz functions.
  //
  // It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct
  // can be stack allocated when WUFFS_IMPLEMENTATION is defined.

  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable vtable_for__wuffs_base__image_decoder;
    wuffs_base__vtable null_vtable;

    uint32_t f_width;
    uint32_t f_height;
    uint8_t f_call_sequence;
    bool f_top_down;
    uint32_t f_pad_per_row;
    uint64_t f_bytes_per_row;
    wuffs_base__pixel_format f_pixfmt;
    uint32_t f_io_redirect_fourcc;
    uint64_t f_io_redirect_pos;
    uint64_t f_frame_config_io_position;
    uint32_t f_padding;
    uint32_t f_mask_r;
    uint32_t f_mask_g;
    uint32_t f_mask_b;
    uint32_t f_mask_a;
    uint32_t f_dst_x;
    uint32_t f_dst_y;
    uint32_t f_dst_y_end;
    uint32_t f_dst_y_inc;
    uint32_t f_pending_pad;
    wuffs_base__pixel_swizzler f_swizzler;

    uint32_t p_decode_image_config[1];
    uint32_t p_decode_frame_config[1];
    uint32_t p_decode_frame[1];
    uint32_t p_skip_frame[1];
  } private_impl;

  struct {
    struct {
      uint32_t v_bitmap_info_len;
      uint32_t v_bits_per_pixel;
      uint32_t v_compression;
      uint64_t scratch;
    } s_decode_image_config[1];
    struct {
      uint64_t scratch;
    } s_decode_frame[1];
    struct {
      uint64_t scratch;
    } s_skip_frame[1];
  } private_data;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_bmp__decoder, decltype(&free)>;

  // On failure, the alloc_etc functions return nullptr. They don't throw.

  static inline unique_ptr
  alloc() {
    return unique_ptr(wuffs_bmp__decoder__alloc(), &free);
  }

  static inline wuffs_base__image_decoder::unique_ptr
  alloc_as__wuffs_base__image_decoder() {
    return wuffs_base__image_decoder::unique_ptr(
        wuffs_bmp__decoder__alloc_as__wuffs_base__image_decoder(), &free);
  }
#endif  // (__cplusplus >= 201103L)

#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)
  // Disallow constructing or copying an object via standard C++ mechanisms,
  // e.g. the "new" operator, as this struct is intentionally opaque. Its total
  // size and field layout is not part of the public, stable, memory-safe API.
  // Use malloc or memcpy and the sizeof__wuffs_foo__bar function instead, and
  // call wuffs_foo__bar__baz methods (which all take a "this"-like pointer as
  // their first argument) rather than tweaking bar.private_impl.qux fields.
  //
  // In C, we can just leave wuffs_foo__bar as an incomplete type (unless
  // WUFFS_IMPLEMENTATION is #define'd). In C++, we define a complete type in
  // order to provide convenience methods. These forward on "this", so that you
  // can write "bar->baz(etc)" instead of "wuffs_foo__bar__baz(bar, etc)".
  wuffs_bmp__decoder__struct() = delete;
  wuffs_bmp__decoder__struct(const wuffs_bmp__decoder__struct&) = delete;
  wuffs_bmp__decoder__struct& operator=(
      const wuffs_bmp__decoder__struct&) = delete;

  // As above, the size of the struct is not part of the public API, and unless
  // WUFFS_IMPLEMENTATION is #define'd, this struct type T should be heap
  // allocated, not stack allocated. Its size is not intended to be known at
  // compile time, but it is unfortunately divulged as a side effect of
  // defining C++ convenience methods. Use "sizeof__T()", calling the function,
  // instead of "sizeof T", invoking the operator. To make the two values
  // different, so that passing the latter will be rejected by the initialize
  // function, we add an arbitrary amount of dead weight.
  uint8_t dead_weight[123000000];  // 123 MB.
#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)

  inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
  initialize(
      size_t sizeof_star_self,
      uint64_t wuffs_version,
      uint32_t initialize_flags) {
    return wuffs_bmp__decoder__initialize(
        this, sizeof_star_self, wuffs_version, initialize_flags);
  }

  inline wuffs_base__image_decoder*
  upcast_as__wuffs_base__image_decoder() {
    return (wuffs_base__image_decoder*)this;
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_bmp__decoder__set_quirk_enabled(this, a_quirk, a_enabled);
  }

  inline wuffs_base__status
  decode_image_config(
      wuffs_base__image_config* a_dst,
      wuffs_base__io_buffer* a_src) {
    return wuffs_bmp__decoder__decode_image_config(this, a_dst, a_src);
  }

  inline wuffs_base__status
  decode_frame_config(
      wuffs_base__frame_config* a_dst,
      wuffs_base__io_buffer* a_src) {
    return wuffs_bmp__decoder__decode_frame_config(this, a_dst, a_src);
  }

  inline wuffs_base__status
  decode_frame(
      wuffs_base__pixel_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__pixel_blend a_blend,
      wuffs_base__slice_u8 a_workbuf,
      wuffs_base__decode_frame_options* a_opts) {
    return wuffs_bmp__decoder__decode_frame(this, a_dst, a_src, a_blend, a_workbuf, a_opts);
  }

  inline wuffs_base__rect_ie_u32
  frame_dirty_rect() const {
    return wuffs_bmp__decoder__frame_dirty_rect(this);
  }

  inline uint32_t
  num_animation_loops() const {
    return wuffs_bmp__decoder__num_animation_loops(this);
  }

  inline uint64_t
  num_decoded_frame_configs() const {
    return wuffs_bmp__decoder__num_decoded_frame_configs(this);
  }

  inline uint64_t
  num_decoded_frames() const {
    return wuffs_bmp__decoder__num_decoded_frames(this);
  }

  inline wuffs_base__status
  restart_frame(
      uint64_t a_index,
      uint64_t a_io_position) {
    return wuffs_bmp__decoder__restart_frame(this, a_index, a_io_position);
  }

  inline wuffs_base__empty_struct
  set_report_metadata(
      uint32_t a_fourcc,
      bool a_report) {
    return wuffs_bmp__decoder__set_report_metadata(this, a_fourcc, a_report);
  }

  inline wuffs_base__status
  tell_me_more(
      wuffs_base__io_buffer* a_dst,
      wuffs_base__more_information* a_minfo,
      wuffs_base__io_buffer* a_src) {
    return wuffs_bmp__decoder__tell_me_more(this, a_dst, a_minfo, a_src);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_bmp__decoder__workbuf_len(this);
  }

#endif  // __cplusplus
};  // struct wuffs_bmp__decoder__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Status Codes

// ---------------- Public Consts

// ---------------- Struct Declarations

typedef struct wuffs_crc32__ieee_hasher__struct wuffs_crc32__ieee_hasher;

// ---------------- Public Initializer Prototypes

// For any given "wuffs_foo__bar* self", "wuffs_foo__bar__initialize(self,
// etc)" should be called before any other "wuffs_foo__bar__xxx(self, etc)".
//
// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.
// Pass 0 (or some combination of WUFFS_INITIALIZE__XXX) for initialize_flags.

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_crc32__ieee_hasher__initialize(
    wuffs_crc32__ieee_hasher* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags);

size_t
sizeof__wuffs_crc32__ieee_hasher();

// ---------------- Allocs

// These functions allocate and initialize Wuffs structs. They return NULL if
// memory allocation fails. If they return non-NULL, there is no need to call
// wuffs_foo__bar__initialize, but the caller is responsible for eventually
// calling free on the returned pointer. That pointer is effectively a C++
// std::unique_ptr<T, decltype(&free)>.

wuffs_crc32__ieee_hasher*
wuffs_crc32__ieee_hasher__alloc();

static inline wuffs_base__hasher_u32*
wuffs_crc32__ieee_hasher__alloc_as__wuffs_base__hasher_u32() {
  return (wuffs_base__hasher_u32*)(wuffs_crc32__ieee_hasher__alloc());
}

// ---------------- Upcasts

static inline wuffs_base__hasher_u32*
wuffs_crc32__ieee_hasher__upcast_as__wuffs_base__hasher_u32(
    wuffs_crc32__ieee_hasher* p) {
  return (wuffs_base__hasher_u32*)p;
}

// ---------------- Public Function Prototypes

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_crc32__ieee_hasher__set_quirk_enabled(
    wuffs_crc32__ieee_hasher* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_crc32__ieee_hasher__update_u32(
    wuffs_crc32__ieee_hasher* self,
    wuffs_base__slice_u8 a_x);

// ---------------- Struct Definitions

// These structs' fields, and the sizeof them, are private implementation
// details that aren't guaranteed to be stable across Wuffs versions.
//
// See https://en.wikipedia.org/wiki/Opaque_pointer#C

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_crc32__ieee_hasher__struct {
  // Do not access the private_impl's or private_data's fields directly. There
  // is no API/ABI compatibility or safety guarantee if you do so. Instead, use
  // the wuffs_foo__bar__baz functions.
  //
  // It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct
  // can be stack allocated when WUFFS_IMPLEMENTATION is defined.

  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable vtable_for__wuffs_base__hasher_u32;
    wuffs_base__vtable null_vtable;

    uint32_t f_state;
  } private_impl;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_crc32__ieee_hasher, decltype(&free)>;

  // On failure, the alloc_etc functions return nullptr. They don't throw.

  static inline unique_ptr
  alloc() {
    return unique_ptr(wuffs_crc32__ieee_hasher__alloc(), &free);
  }

  static inline wuffs_base__hasher_u32::unique_ptr
  alloc_as__wuffs_base__hasher_u32() {
    return wuffs_base__hasher_u32::unique_ptr(
        wuffs_crc32__ieee_hasher__alloc_as__wuffs_base__hasher_u32(), &free);
  }
#endif  // (__cplusplus >= 201103L)

#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)
  // Disallow constructing or copying an object via standard C++ mechanisms,
  // e.g. the "new" operator, as this struct is intentionally opaque. Its total
  // size and field layout is not part of the public, stable, memory-safe API.
  // Use malloc or memcpy and the sizeof__wuffs_foo__bar function instead, and
  // call wuffs_foo__bar__baz methods (which all take a "this"-like pointer as
  // their first argument) rather than tweaking bar.private_impl.qux fields.
  //
  // In C, we can just leave wuffs_foo__bar as an incomplete type (unless
  // WUFFS_IMPLEMENTATION is #define'd). In C++, we define a complete type in
  // order to provide convenience methods. These forward on "this", so that you
  // can write "bar->baz(etc)" instead of "wuffs_foo__bar__baz(bar, etc)".
  wuffs_crc32__ieee_hasher__struct() = delete;
  wuffs_crc32__ieee_hasher__struct(const wuffs_crc32__ieee_hasher__struct&) = delete;
  wuffs_crc32__ieee_hasher__struct& operator=(
      const wuffs_crc32__ieee_hasher__struct&) = delete;

  // As above, the size of the struct is not part of the public API, and unless
  // WUFFS_IMPLEMENTATION is #define'd, this struct type T should be heap
  // allocated, not stack allocated. Its size is not intended to be known at
  // compile time, but it is unfortunately divulged as a side effect of
  // defining C++ convenience methods. Use "sizeof__T()", calling the function,
  // instead of "sizeof T", invoking the operator. To make the two values
  // different, so that passing the latter will be rejected by the initialize
  // function, we add an arbitrary amount of dead weight.
  uint8_t dead_weight[123000000];  // 123 MB.
#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)

  inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
  initialize(
      size_t sizeof_star_self,
      uint64_t wuffs_version,
      uint32_t initialize_flags) {
    return wuffs_crc32__ieee_hasher__initialize(
        this, sizeof_star_self, wuffs_version, initialize_flags);
  }

  inline wuffs_base__hasher_u32*
  upcast_as__wuffs_base__hasher_u32() {
    return (wuffs_base__hasher_u32*)this;
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_crc32__ieee_hasher__set_quirk_enabled(this, a_quirk, a_enabled);
  }

  inline uint32_t
  update_u32(
      wuffs_base__slice_u8 a_x) {
    return wuffs_crc32__ieee_hasher__update_u32(this, a_x);
  }

#endif  // __cplusplus
};  // struct wuffs_crc32__ieee_hasher__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Status Codes

extern const char* wuffs_deflate__error__bad_huffman_code_over_subscribed;
extern const char* wuffs_deflate__error__bad_huffman_code_under_subscribed;
extern const char* wuffs_deflate__error__bad_huffman_code_length_count;
extern const char* wuffs_deflate__error__bad_huffman_code_length_repetition;
extern const char* wuffs_deflate__error__bad_huffman_code;
extern const char* wuffs_deflate__error__bad_huffman_minimum_code_length;
extern const char* wuffs_deflate__error__bad_block;
extern const char* wuffs_deflate__error__bad_distance;
extern const char* wuffs_deflate__error__bad_distance_code_count;
extern const char* wuffs_deflate__error__bad_literal_length_code_count;
extern const char* wuffs_deflate__error__inconsistent_stored_block_length;
extern const char* wuffs_deflate__error__missing_end_of_block_code;
extern const char* wuffs_deflate__error__no_huffman_codes;

// ---------------- Public Consts

#define WUFFS_DEFLATE__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE 1

// ---------------- Struct Declarations

typedef struct wuffs_deflate__decoder__struct wuffs_deflate__decoder;

// ---------------- Public Initializer Prototypes

// For any given "wuffs_foo__bar* self", "wuffs_foo__bar__initialize(self,
// etc)" should be called before any other "wuffs_foo__bar__xxx(self, etc)".
//
// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.
// Pass 0 (or some combination of WUFFS_INITIALIZE__XXX) for initialize_flags.

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_deflate__decoder__initialize(
    wuffs_deflate__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags);

size_t
sizeof__wuffs_deflate__decoder();

// ---------------- Allocs

// These functions allocate and initialize Wuffs structs. They return NULL if
// memory allocation fails. If they return non-NULL, there is no need to call
// wuffs_foo__bar__initialize, but the caller is responsible for eventually
// calling free on the returned pointer. That pointer is effectively a C++
// std::unique_ptr<T, decltype(&free)>.

wuffs_deflate__decoder*
wuffs_deflate__decoder__alloc();

static inline wuffs_base__io_transformer*
wuffs_deflate__decoder__alloc_as__wuffs_base__io_transformer() {
  return (wuffs_base__io_transformer*)(wuffs_deflate__decoder__alloc());
}

// ---------------- Upcasts

static inline wuffs_base__io_transformer*
wuffs_deflate__decoder__upcast_as__wuffs_base__io_transformer(
    wuffs_deflate__decoder* p) {
  return (wuffs_base__io_transformer*)p;
}

// ---------------- Public Function Prototypes

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_deflate__decoder__add_history(
    wuffs_deflate__decoder* self,
    wuffs_base__slice_u8 a_hist);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_deflate__decoder__set_quirk_enabled(
    wuffs_deflate__decoder* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_deflate__decoder__workbuf_len(
    const wuffs_deflate__decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_deflate__decoder__transform_io(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf);

// ---------------- Struct Definitions

// These structs' fields, and the sizeof them, are private implementation
// details that aren't guaranteed to be stable across Wuffs versions.
//
// See https://en.wikipedia.org/wiki/Opaque_pointer#C

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_deflate__decoder__struct {
  // Do not access the private_impl's or private_data's fields directly. There
  // is no API/ABI compatibility or safety guarantee if you do so. Instead, use
  // the wuffs_foo__bar__baz functions.
  //
  // It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct
  // can be stack allocated when WUFFS_IMPLEMENTATION is defined.

  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable vtable_for__wuffs_base__io_transformer;
    wuffs_base__vtable null_vtable;

    uint32_t f_bits;
    uint32_t f_n_bits;
    uint32_t f_history_index;
    uint32_t f_n_huffs_bits[2];
    bool f_end_of_block;

    uint32_t p_transform_io[1];
    uint32_t p_decode_blocks[1];
    uint32_t p_decode_uncompressed[1];
    uint32_t p_init_dynamic_huffman[1];
    uint32_t p_decode_huffman_slow[1];
  } private_impl;

  struct {
    uint32_t f_huffs[2][1024];
    uint8_t f_history[33025];
    uint8_t f_code_lengths[320];

    struct {
      uint32_t v_final;
    } s_decode_blocks[1];
    struct {
      uint32_t v_length;
      uint64_t scratch;
    } s_decode_uncompressed[1];
    struct {
      uint32_t v_bits;
      uint32_t v_n_bits;
      uint32_t v_n_lit;
      uint32_t v_n_dist;
      uint32_t v_n_clen;
      uint32_t v_i;
      uint32_t v_mask;
      uint32_t v_table_entry;
      uint32_t v_n_extra_bits;
      uint8_t v_rep_symbol;
      uint32_t v_rep_count;
    } s_init_dynamic_huffman[1];
    struct {
      uint32_t v_bits;
      uint32_t v_n_bits;
      uint32_t v_table_entry;
      uint32_t v_table_entry_n_bits;
      uint32_t v_lmask;
      uint32_t v_dmask;
      uint32_t v_redir_top;
      uint32_t v_redir_mask;
      uint32_t v_length;
      uint32_t v_dist_minus_1;
      uint32_t v_hlen;
      uint32_t v_hdist;
      uint64_t scratch;
    } s_decode_huffman_slow[1];
  } private_data;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_deflate__decoder, decltype(&free)>;

  // On failure, the alloc_etc functions return nullptr. They don't throw.

  static inline unique_ptr
  alloc() {
    return unique_ptr(wuffs_deflate__decoder__alloc(), &free);
  }

  static inline wuffs_base__io_transformer::unique_ptr
  alloc_as__wuffs_base__io_transformer() {
    return wuffs_base__io_transformer::unique_ptr(
        wuffs_deflate__decoder__alloc_as__wuffs_base__io_transformer(), &free);
  }
#endif  // (__cplusplus >= 201103L)

#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)
  // Disallow constructing or copying an object via standard C++ mechanisms,
  // e.g. the "new" operator, as this struct is intentionally opaque. Its total
  // size and field layout is not part of the public, stable, memory-safe API.
  // Use malloc or memcpy and the sizeof__wuffs_foo__bar function instead, and
  // call wuffs_foo__bar__baz methods (which all take a "this"-like pointer as
  // their first argument) rather than tweaking bar.private_impl.qux fields.
  //
  // In C, we can just leave wuffs_foo__bar as an incomplete type (unless
  // WUFFS_IMPLEMENTATION is #define'd). In C++, we define a complete type in
  // order to provide convenience methods. These forward on "this", so that you
  // can write "bar->baz(etc)" instead of "wuffs_foo__bar__baz(bar, etc)".
  wuffs_deflate__decoder__struct() = delete;
  wuffs_deflate__decoder__struct(const wuffs_deflate__decoder__struct&) = delete;
  wuffs_deflate__decoder__struct& operator=(
      const wuffs_deflate__decoder__struct&) = delete;

  // As above, the size of the struct is not part of the public API, and unless
  // WUFFS_IMPLEMENTATION is #define'd, this struct type T should be heap
  // allocated, not stack allocated. Its size is not intended to be known at
  // compile time, but it is unfortunately divulged as a side effect of
  // defining C++ convenience methods. Use "sizeof__T()", calling the function,
  // instead of "sizeof T", invoking the operator. To make the two values
  // different, so that passing the latter will be rejected by the initialize
  // function, we add an arbitrary amount of dead weight.
  uint8_t dead_weight[123000000];  // 123 MB.
#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)

  inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
  initialize(
      size_t sizeof_star_self,
      uint64_t wuffs_version,
      uint32_t initialize_flags) {
    return wuffs_deflate__decoder__initialize(
        this, sizeof_star_self, wuffs_version, initialize_flags);
  }

  inline wuffs_base__io_transformer*
  upcast_as__wuffs_base__io_transformer() {
    return (wuffs_base__io_transformer*)this;
  }

  inline wuffs_base__empty_struct
  add_history(
      wuffs_base__slice_u8 a_hist) {
    return wuffs_deflate__decoder__add_history(this, a_hist);
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_deflate__decoder__set_quirk_enabled(this, a_quirk, a_enabled);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_deflate__decoder__workbuf_len(this);
  }

  inline wuffs_base__status
  transform_io(
      wuffs_base__io_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__slice_u8 a_workbuf) {
    return wuffs_deflate__decoder__transform_io(this, a_dst, a_src, a_workbuf);
  }

#endif  // __cplusplus
};  // struct wuffs_deflate__decoder__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Status Codes

extern const char* wuffs_lzw__error__bad_code;

// ---------------- Public Consts

#define WUFFS_LZW__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE 0

// ---------------- Struct Declarations

typedef struct wuffs_lzw__decoder__struct wuffs_lzw__decoder;

// ---------------- Public Initializer Prototypes

// For any given "wuffs_foo__bar* self", "wuffs_foo__bar__initialize(self,
// etc)" should be called before any other "wuffs_foo__bar__xxx(self, etc)".
//
// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.
// Pass 0 (or some combination of WUFFS_INITIALIZE__XXX) for initialize_flags.

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_lzw__decoder__initialize(
    wuffs_lzw__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags);

size_t
sizeof__wuffs_lzw__decoder();

// ---------------- Allocs

// These functions allocate and initialize Wuffs structs. They return NULL if
// memory allocation fails. If they return non-NULL, there is no need to call
// wuffs_foo__bar__initialize, but the caller is responsible for eventually
// calling free on the returned pointer. That pointer is effectively a C++
// std::unique_ptr<T, decltype(&free)>.

wuffs_lzw__decoder*
wuffs_lzw__decoder__alloc();

static inline wuffs_base__io_transformer*
wuffs_lzw__decoder__alloc_as__wuffs_base__io_transformer() {
  return (wuffs_base__io_transformer*)(wuffs_lzw__decoder__alloc());
}

// ---------------- Upcasts

static inline wuffs_base__io_transformer*
wuffs_lzw__decoder__upcast_as__wuffs_base__io_transformer(
    wuffs_lzw__decoder* p) {
  return (wuffs_base__io_transformer*)p;
}

// ---------------- Public Function Prototypes

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_lzw__decoder__set_quirk_enabled(
    wuffs_lzw__decoder* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_lzw__decoder__set_literal_width(
    wuffs_lzw__decoder* self,
    uint32_t a_lw);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_lzw__decoder__workbuf_len(
    const wuffs_lzw__decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_lzw__decoder__transform_io(
    wuffs_lzw__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf);

WUFFS_BASE__MAYBE_STATIC wuffs_base__slice_u8
wuffs_lzw__decoder__flush(
    wuffs_lzw__decoder* self);

// ---------------- Struct Definitions

// These structs' fields, and the sizeof them, are private implementation
// details that aren't guaranteed to be stable across Wuffs versions.
//
// See https://en.wikipedia.org/wiki/Opaque_pointer#C

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_lzw__decoder__struct {
  // Do not access the private_impl's or private_data's fields directly. There
  // is no API/ABI compatibility or safety guarantee if you do so. Instead, use
  // the wuffs_foo__bar__baz functions.
  //
  // It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct
  // can be stack allocated when WUFFS_IMPLEMENTATION is defined.

  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable vtable_for__wuffs_base__io_transformer;
    wuffs_base__vtable null_vtable;

    uint32_t f_set_literal_width_arg;
    uint32_t f_literal_width;
    uint32_t f_clear_code;
    uint32_t f_end_code;
    uint32_t f_save_code;
    uint32_t f_prev_code;
    uint32_t f_width;
    uint32_t f_bits;
    uint32_t f_n_bits;
    uint32_t f_output_ri;
    uint32_t f_output_wi;
    uint32_t f_read_from_return_value;
    uint16_t f_prefixes[4096];

    uint32_t p_transform_io[1];
    uint32_t p_write_to[1];
  } private_impl;

  struct {
    uint8_t f_suffixes[4096][8];
    uint16_t f_lm1s[4096];
    uint8_t f_output[8199];
  } private_data;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_lzw__decoder, decltype(&free)>;

  // On failure, the alloc_etc functions return nullptr. They don't throw.

  static inline unique_ptr
  alloc() {
    return unique_ptr(wuffs_lzw__decoder__alloc(), &free);
  }

  static inline wuffs_base__io_transformer::unique_ptr
  alloc_as__wuffs_base__io_transformer() {
    return wuffs_base__io_transformer::unique_ptr(
        wuffs_lzw__decoder__alloc_as__wuffs_base__io_transformer(), &free);
  }
#endif  // (__cplusplus >= 201103L)

#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)
  // Disallow constructing or copying an object via standard C++ mechanisms,
  // e.g. the "new" operator, as this struct is intentionally opaque. Its total
  // size and field layout is not part of the public, stable, memory-safe API.
  // Use malloc or memcpy and the sizeof__wuffs_foo__bar function instead, and
  // call wuffs_foo__bar__baz methods (which all take a "this"-like pointer as
  // their first argument) rather than tweaking bar.private_impl.qux fields.
  //
  // In C, we can just leave wuffs_foo__bar as an incomplete type (unless
  // WUFFS_IMPLEMENTATION is #define'd). In C++, we define a complete type in
  // order to provide convenience methods. These forward on "this", so that you
  // can write "bar->baz(etc)" instead of "wuffs_foo__bar__baz(bar, etc)".
  wuffs_lzw__decoder__struct() = delete;
  wuffs_lzw__decoder__struct(const wuffs_lzw__decoder__struct&) = delete;
  wuffs_lzw__decoder__struct& operator=(
      const wuffs_lzw__decoder__struct&) = delete;

  // As above, the size of the struct is not part of the public API, and unless
  // WUFFS_IMPLEMENTATION is #define'd, this struct type T should be heap
  // allocated, not stack allocated. Its size is not intended to be known at
  // compile time, but it is unfortunately divulged as a side effect of
  // defining C++ convenience methods. Use "sizeof__T()", calling the function,
  // instead of "sizeof T", invoking the operator. To make the two values
  // different, so that passing the latter will be rejected by the initialize
  // function, we add an arbitrary amount of dead weight.
  uint8_t dead_weight[123000000];  // 123 MB.
#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)

  inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
  initialize(
      size_t sizeof_star_self,
      uint64_t wuffs_version,
      uint32_t initialize_flags) {
    return wuffs_lzw__decoder__initialize(
        this, sizeof_star_self, wuffs_version, initialize_flags);
  }

  inline wuffs_base__io_transformer*
  upcast_as__wuffs_base__io_transformer() {
    return (wuffs_base__io_transformer*)this;
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_lzw__decoder__set_quirk_enabled(this, a_quirk, a_enabled);
  }

  inline wuffs_base__empty_struct
  set_literal_width(
      uint32_t a_lw) {
    return wuffs_lzw__decoder__set_literal_width(this, a_lw);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_lzw__decoder__workbuf_len(this);
  }

  inline wuffs_base__status
  transform_io(
      wuffs_base__io_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__slice_u8 a_workbuf) {
    return wuffs_lzw__decoder__transform_io(this, a_dst, a_src, a_workbuf);
  }

  inline wuffs_base__slice_u8
  flush() {
    return wuffs_lzw__decoder__flush(this);
  }

#endif  // __cplusplus
};  // struct wuffs_lzw__decoder__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Status Codes

extern const char* wuffs_gif__error__bad_extension_label;
extern const char* wuffs_gif__error__bad_frame_size;
extern const char* wuffs_gif__error__bad_graphic_control;
extern const char* wuffs_gif__error__bad_header;
extern const char* wuffs_gif__error__bad_literal_width;
extern const char* wuffs_gif__error__bad_palette;

// ---------------- Public Consts

#define WUFFS_GIF__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE 0

#define WUFFS_GIF__QUIRK_DELAY_NUM_DECODED_FRAMES 1041635328

#define WUFFS_GIF__QUIRK_FIRST_FRAME_LOCAL_PALETTE_MEANS_BLACK_BACKGROUND 1041635329

#define WUFFS_GIF__QUIRK_HONOR_BACKGROUND_COLOR 1041635330

#define WUFFS_GIF__QUIRK_IGNORE_TOO_MUCH_PIXEL_DATA 1041635331

#define WUFFS_GIF__QUIRK_IMAGE_BOUNDS_ARE_STRICT 1041635332

#define WUFFS_GIF__QUIRK_REJECT_EMPTY_FRAME 1041635333

#define WUFFS_GIF__QUIRK_REJECT_EMPTY_PALETTE 1041635334

// ---------------- Struct Declarations

typedef struct wuffs_gif__config_decoder__struct wuffs_gif__config_decoder;

typedef struct wuffs_gif__decoder__struct wuffs_gif__decoder;

// ---------------- Public Initializer Prototypes

// For any given "wuffs_foo__bar* self", "wuffs_foo__bar__initialize(self,
// etc)" should be called before any other "wuffs_foo__bar__xxx(self, etc)".
//
// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.
// Pass 0 (or some combination of WUFFS_INITIALIZE__XXX) for initialize_flags.

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_gif__config_decoder__initialize(
    wuffs_gif__config_decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags);

size_t
sizeof__wuffs_gif__config_decoder();

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_gif__decoder__initialize(
    wuffs_gif__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags);

size_t
sizeof__wuffs_gif__decoder();

// ---------------- Allocs

// These functions allocate and initialize Wuffs structs. They return NULL if
// memory allocation fails. If they return non-NULL, there is no need to call
// wuffs_foo__bar__initialize, but the caller is responsible for eventually
// calling free on the returned pointer. That pointer is effectively a C++
// std::unique_ptr<T, decltype(&free)>.

wuffs_gif__config_decoder*
wuffs_gif__config_decoder__alloc();

static inline wuffs_base__image_decoder*
wuffs_gif__config_decoder__alloc_as__wuffs_base__image_decoder() {
  return (wuffs_base__image_decoder*)(wuffs_gif__config_decoder__alloc());
}

wuffs_gif__decoder*
wuffs_gif__decoder__alloc();

static inline wuffs_base__image_decoder*
wuffs_gif__decoder__alloc_as__wuffs_base__image_decoder() {
  return (wuffs_base__image_decoder*)(wuffs_gif__decoder__alloc());
}

// ---------------- Upcasts

static inline wuffs_base__image_decoder*
wuffs_gif__config_decoder__upcast_as__wuffs_base__image_decoder(
    wuffs_gif__config_decoder* p) {
  return (wuffs_base__image_decoder*)p;
}

static inline wuffs_base__image_decoder*
wuffs_gif__decoder__upcast_as__wuffs_base__image_decoder(
    wuffs_gif__decoder* p) {
  return (wuffs_base__image_decoder*)p;
}

// ---------------- Public Function Prototypes

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gif__config_decoder__set_quirk_enabled(
    wuffs_gif__config_decoder* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__config_decoder__decode_image_config(
    wuffs_gif__config_decoder* self,
    wuffs_base__image_config* a_dst,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gif__config_decoder__set_report_metadata(
    wuffs_gif__config_decoder* self,
    uint32_t a_fourcc,
    bool a_report);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__config_decoder__tell_me_more(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__more_information* a_minfo,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_gif__config_decoder__num_animation_loops(
    const wuffs_gif__config_decoder* self);

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_gif__config_decoder__num_decoded_frame_configs(
    const wuffs_gif__config_decoder* self);

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_gif__config_decoder__num_decoded_frames(
    const wuffs_gif__config_decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__rect_ie_u32
wuffs_gif__config_decoder__frame_dirty_rect(
    const wuffs_gif__config_decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_gif__config_decoder__workbuf_len(
    const wuffs_gif__config_decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__config_decoder__restart_frame(
    wuffs_gif__config_decoder* self,
    uint64_t a_index,
    uint64_t a_io_position);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__config_decoder__decode_frame_config(
    wuffs_gif__config_decoder* self,
    wuffs_base__frame_config* a_dst,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__config_decoder__decode_frame(
    wuffs_gif__config_decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend,
    wuffs_base__slice_u8 a_workbuf,
    wuffs_base__decode_frame_options* a_opts);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gif__decoder__set_quirk_enabled(
    wuffs_gif__decoder* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__decoder__decode_image_config(
    wuffs_gif__decoder* self,
    wuffs_base__image_config* a_dst,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gif__decoder__set_report_metadata(
    wuffs_gif__decoder* self,
    uint32_t a_fourcc,
    bool a_report);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__decoder__tell_me_more(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__more_information* a_minfo,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_gif__decoder__num_animation_loops(
    const wuffs_gif__decoder* self);

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_gif__decoder__num_decoded_frame_configs(
    const wuffs_gif__decoder* self);

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_gif__decoder__num_decoded_frames(
    const wuffs_gif__decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__rect_ie_u32
wuffs_gif__decoder__frame_dirty_rect(
    const wuffs_gif__decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_gif__decoder__workbuf_len(
    const wuffs_gif__decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__decoder__restart_frame(
    wuffs_gif__decoder* self,
    uint64_t a_index,
    uint64_t a_io_position);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__decoder__decode_frame_config(
    wuffs_gif__decoder* self,
    wuffs_base__frame_config* a_dst,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__decoder__decode_frame(
    wuffs_gif__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend,
    wuffs_base__slice_u8 a_workbuf,
    wuffs_base__decode_frame_options* a_opts);

// ---------------- Struct Definitions

// These structs' fields, and the sizeof them, are private implementation
// details that aren't guaranteed to be stable across Wuffs versions.
//
// See https://en.wikipedia.org/wiki/Opaque_pointer#C

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_gif__config_decoder__struct {
  // Do not access the private_impl's or private_data's fields directly. There
  // is no API/ABI compatibility or safety guarantee if you do so. Instead, use
  // the wuffs_foo__bar__baz functions.
  //
  // It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct
  // can be stack allocated when WUFFS_IMPLEMENTATION is defined.

  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable vtable_for__wuffs_base__image_decoder;
    wuffs_base__vtable null_vtable;

    uint32_t f_width;
    uint32_t f_height;
    uint8_t f_call_sequence;
    bool f_ignore_metadata;
    bool f_report_metadata_iccp;
    bool f_report_metadata_xmp;
    uint32_t f_metadata_fourcc;
    uint64_t f_metadata_io_position;
    bool f_quirks[7];
    bool f_delayed_num_decoded_frames;
    bool f_end_of_data;
    bool f_restarted;
    bool f_previous_lzw_decode_ended_abruptly;
    bool f_has_global_palette;
    uint8_t f_interlace;
    bool f_seen_num_loops;
    uint32_t f_num_loops;
    uint32_t f_background_color_u32_argb_premul;
    uint32_t f_black_color_u32_argb_premul;
    bool f_gc_has_transparent_index;
    uint8_t f_gc_transparent_index;
    uint8_t f_gc_disposal;
    uint64_t f_gc_duration;
    uint64_t f_frame_config_io_position;
    uint64_t f_num_decoded_frame_configs_value;
    uint64_t f_num_decoded_frames_value;
    uint32_t f_frame_rect_x0;
    uint32_t f_frame_rect_y0;
    uint32_t f_frame_rect_x1;
    uint32_t f_frame_rect_y1;

    uint32_t p_decode_image_config[1];
    uint32_t p_tell_me_more[1];
    uint32_t p_decode_frame_config[1];
    uint32_t p_skip_frame[1];
    uint32_t p_decode_up_to_id_part1[1];
    uint32_t p_decode_header[1];
    uint32_t p_decode_lsd[1];
    uint32_t p_decode_extension[1];
    uint32_t p_skip_blocks[1];
    uint32_t p_decode_ae[1];
    uint32_t p_decode_gc[1];
    uint32_t p_decode_id_part0[1];
  } private_impl;

  struct {
    uint8_t f_palettes[1][1024];

    struct {
      uint32_t v_background_color;
    } s_decode_frame_config[1];
    struct {
      uint64_t scratch;
    } s_skip_frame[1];
    struct {
      uint8_t v_c[6];
      uint32_t v_i;
    } s_decode_header[1];
    struct {
      uint8_t v_flags;
      uint8_t v_background_color_index;
      uint32_t v_num_palette_entries;
      uint32_t v_i;
      uint64_t scratch;
    } s_decode_lsd[1];
    struct {
      uint64_t scratch;
    } s_skip_blocks[1];
    struct {
      uint8_t v_block_size;
      bool v_is_animexts;
      bool v_is_netscape;
      bool v_is_iccp;
      bool v_is_xmp;
      uint64_t scratch;
    } s_decode_ae[1];
    struct {
      uint64_t scratch;
    } s_decode_gc[1];
    struct {
      uint64_t scratch;
    } s_decode_id_part0[1];
  } private_data;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_gif__config_decoder, decltype(&free)>;

  // On failure, the alloc_etc functions return nullptr. They don't throw.

  static inline unique_ptr
  alloc() {
    return unique_ptr(wuffs_gif__config_decoder__alloc(), &free);
  }

  static inline wuffs_base__image_decoder::unique_ptr
  alloc_as__wuffs_base__image_decoder() {
    return wuffs_base__image_decoder::unique_ptr(
        wuffs_gif__config_decoder__alloc_as__wuffs_base__image_decoder(), &free);
  }
#endif  // (__cplusplus >= 201103L)

#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)
  // Disallow constructing or copying an object via standard C++ mechanisms,
  // e.g. the "new" operator, as this struct is intentionally opaque. Its total
  // size and field layout is not part of the public, stable, memory-safe API.
  // Use malloc or memcpy and the sizeof__wuffs_foo__bar function instead, and
  // call wuffs_foo__bar__baz methods (which all take a "this"-like pointer as
  // their first argument) rather than tweaking bar.private_impl.qux fields.
  //
  // In C, we can just leave wuffs_foo__bar as an incomplete type (unless
  // WUFFS_IMPLEMENTATION is #define'd). In C++, we define a complete type in
  // order to provide convenience methods. These forward on "this", so that you
  // can write "bar->baz(etc)" instead of "wuffs_foo__bar__baz(bar, etc)".
  wuffs_gif__config_decoder__struct() = delete;
  wuffs_gif__config_decoder__struct(const wuffs_gif__config_decoder__struct&) = delete;
  wuffs_gif__config_decoder__struct& operator=(
      const wuffs_gif__config_decoder__struct&) = delete;

  // As above, the size of the struct is not part of the public API, and unless
  // WUFFS_IMPLEMENTATION is #define'd, this struct type T should be heap
  // allocated, not stack allocated. Its size is not intended to be known at
  // compile time, but it is unfortunately divulged as a side effect of
  // defining C++ convenience methods. Use "sizeof__T()", calling the function,
  // instead of "sizeof T", invoking the operator. To make the two values
  // different, so that passing the latter will be rejected by the initialize
  // function, we add an arbitrary amount of dead weight.
  uint8_t dead_weight[123000000];  // 123 MB.
#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)

  inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
  initialize(
      size_t sizeof_star_self,
      uint64_t wuffs_version,
      uint32_t initialize_flags) {
    return wuffs_gif__config_decoder__initialize(
        this, sizeof_star_self, wuffs_version, initialize_flags);
  }

  inline wuffs_base__image_decoder*
  upcast_as__wuffs_base__image_decoder() {
    return (wuffs_base__image_decoder*)this;
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_gif__config_decoder__set_quirk_enabled(this, a_quirk, a_enabled);
  }

  inline wuffs_base__status
  decode_image_config(
      wuffs_base__image_config* a_dst,
      wuffs_base__io_buffer* a_src) {
    return wuffs_gif__config_decoder__decode_image_config(this, a_dst, a_src);
  }

  inline wuffs_base__empty_struct
  set_report_metadata(
      uint32_t a_fourcc,
      bool a_report) {
    return wuffs_gif__config_decoder__set_report_metadata(this, a_fourcc, a_report);
  }

  inline wuffs_base__status
  tell_me_more(
      wuffs_base__io_buffer* a_dst,
      wuffs_base__more_information* a_minfo,
      wuffs_base__io_buffer* a_src) {
    return wuffs_gif__config_decoder__tell_me_more(this, a_dst, a_minfo, a_src);
  }

  inline uint32_t
  num_animation_loops() const {
    return wuffs_gif__config_decoder__num_animation_loops(this);
  }

  inline uint64_t
  num_decoded_frame_configs() const {
    return wuffs_gif__config_decoder__num_decoded_frame_configs(this);
  }

  inline uint64_t
  num_decoded_frames() const {
    return wuffs_gif__config_decoder__num_decoded_frames(this);
  }

  inline wuffs_base__rect_ie_u32
  frame_dirty_rect() const {
    return wuffs_gif__config_decoder__frame_dirty_rect(this);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_gif__config_decoder__workbuf_len(this);
  }

  inline wuffs_base__status
  restart_frame(
      uint64_t a_index,
      uint64_t a_io_position) {
    return wuffs_gif__config_decoder__restart_frame(this, a_index, a_io_position);
  }

  inline wuffs_base__status
  decode_frame_config(
      wuffs_base__frame_config* a_dst,
      wuffs_base__io_buffer* a_src) {
    return wuffs_gif__config_decoder__decode_frame_config(this, a_dst, a_src);
  }

  inline wuffs_base__status
  decode_frame(
      wuffs_base__pixel_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__pixel_blend a_blend,
      wuffs_base__slice_u8 a_workbuf,
      wuffs_base__decode_frame_options* a_opts) {
    return wuffs_gif__config_decoder__decode_frame(this, a_dst, a_src, a_blend, a_workbuf, a_opts);
  }

#endif  // __cplusplus
};  // struct wuffs_gif__config_decoder__struct

struct wuffs_gif__decoder__struct {
  // Do not access the private_impl's or private_data's fields directly. There
  // is no API/ABI compatibility or safety guarantee if you do so. Instead, use
  // the wuffs_foo__bar__baz functions.
  //
  // It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct
  // can be stack allocated when WUFFS_IMPLEMENTATION is defined.

  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable vtable_for__wuffs_base__image_decoder;
    wuffs_base__vtable null_vtable;

    uint32_t f_width;
    uint32_t f_height;
    uint8_t f_call_sequence;
    bool f_ignore_metadata;
    bool f_report_metadata_iccp;
    bool f_report_metadata_xmp;
    uint32_t f_metadata_fourcc;
    uint64_t f_metadata_io_position;
    bool f_quirks[7];
    bool f_delayed_num_decoded_frames;
    bool f_end_of_data;
    bool f_restarted;
    bool f_previous_lzw_decode_ended_abruptly;
    bool f_has_global_palette;
    uint8_t f_interlace;
    bool f_seen_num_loops;
    uint32_t f_num_loops;
    uint32_t f_background_color_u32_argb_premul;
    uint32_t f_black_color_u32_argb_premul;
    bool f_gc_has_transparent_index;
    uint8_t f_gc_transparent_index;
    uint8_t f_gc_disposal;
    uint64_t f_gc_duration;
    uint64_t f_frame_config_io_position;
    uint64_t f_num_decoded_frame_configs_value;
    uint64_t f_num_decoded_frames_value;
    uint32_t f_frame_rect_x0;
    uint32_t f_frame_rect_y0;
    uint32_t f_frame_rect_x1;
    uint32_t f_frame_rect_y1;
    uint32_t f_dst_x;
    uint32_t f_dst_y;
    uint32_t f_dirty_max_excl_y;
    uint64_t f_compressed_ri;
    uint64_t f_compressed_wi;
    wuffs_base__pixel_swizzler f_swizzler;

    uint32_t p_decode_image_config[1];
    uint32_t p_tell_me_more[1];
    uint32_t p_decode_frame_config[1];
    uint32_t p_skip_frame[1];
    uint32_t p_decode_frame[1];
    uint32_t p_decode_up_to_id_part1[1];
    uint32_t p_decode_header[1];
    uint32_t p_decode_lsd[1];
    uint32_t p_decode_extension[1];
    uint32_t p_skip_blocks[1];
    uint32_t p_decode_ae[1];
    uint32_t p_decode_gc[1];
    uint32_t p_decode_id_part0[1];
    uint32_t p_decode_id_part1[1];
    uint32_t p_decode_id_part2[1];
  } private_impl;

  struct {
    uint8_t f_compressed[4096];
    uint8_t f_palettes[2][1024];
    uint8_t f_dst_palette[1024];
    wuffs_lzw__decoder f_lzw;

    struct {
      uint32_t v_background_color;
    } s_decode_frame_config[1];
    struct {
      uint64_t scratch;
    } s_skip_frame[1];
    struct {
      uint8_t v_c[6];
      uint32_t v_i;
    } s_decode_header[1];
    struct {
      uint8_t v_flags;
      uint8_t v_background_color_index;
      uint32_t v_num_palette_entries;
      uint32_t v_i;
      uint64_t scratch;
    } s_decode_lsd[1];
    struct {
      uint64_t scratch;
    } s_skip_blocks[1];
    struct {
      uint8_t v_block_size;
      bool v_is_animexts;
      bool v_is_netscape;
      bool v_is_iccp;
      bool v_is_xmp;
      uint64_t scratch;
    } s_decode_ae[1];
    struct {
      uint64_t scratch;
    } s_decode_gc[1];
    struct {
      uint64_t scratch;
    } s_decode_id_part0[1];
    struct {
      uint8_t v_which_palette;
      uint32_t v_num_palette_entries;
      uint32_t v_i;
      uint64_t scratch;
    } s_decode_id_part1[1];
    struct {
      uint64_t v_block_size;
      bool v_need_block_size;
      wuffs_base__status v_lzw_status;
      uint64_t scratch;
    } s_decode_id_part2[1];
  } private_data;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_gif__decoder, decltype(&free)>;

  // On failure, the alloc_etc functions return nullptr. They don't throw.

  static inline unique_ptr
  alloc() {
    return unique_ptr(wuffs_gif__decoder__alloc(), &free);
  }

  static inline wuffs_base__image_decoder::unique_ptr
  alloc_as__wuffs_base__image_decoder() {
    return wuffs_base__image_decoder::unique_ptr(
        wuffs_gif__decoder__alloc_as__wuffs_base__image_decoder(), &free);
  }
#endif  // (__cplusplus >= 201103L)

#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)
  // Disallow constructing or copying an object via standard C++ mechanisms,
  // e.g. the "new" operator, as this struct is intentionally opaque. Its total
  // size and field layout is not part of the public, stable, memory-safe API.
  // Use malloc or memcpy and the sizeof__wuffs_foo__bar function instead, and
  // call wuffs_foo__bar__baz methods (which all take a "this"-like pointer as
  // their first argument) rather than tweaking bar.private_impl.qux fields.
  //
  // In C, we can just leave wuffs_foo__bar as an incomplete type (unless
  // WUFFS_IMPLEMENTATION is #define'd). In C++, we define a complete type in
  // order to provide convenience methods. These forward on "this", so that you
  // can write "bar->baz(etc)" instead of "wuffs_foo__bar__baz(bar, etc)".
  wuffs_gif__decoder__struct() = delete;
  wuffs_gif__decoder__struct(const wuffs_gif__decoder__struct&) = delete;
  wuffs_gif__decoder__struct& operator=(
      const wuffs_gif__decoder__struct&) = delete;

  // As above, the size of the struct is not part of the public API, and unless
  // WUFFS_IMPLEMENTATION is #define'd, this struct type T should be heap
  // allocated, not stack allocated. Its size is not intended to be known at
  // compile time, but it is unfortunately divulged as a side effect of
  // defining C++ convenience methods. Use "sizeof__T()", calling the function,
  // instead of "sizeof T", invoking the operator. To make the two values
  // different, so that passing the latter will be rejected by the initialize
  // function, we add an arbitrary amount of dead weight.
  uint8_t dead_weight[123000000];  // 123 MB.
#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)

  inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
  initialize(
      size_t sizeof_star_self,
      uint64_t wuffs_version,
      uint32_t initialize_flags) {
    return wuffs_gif__decoder__initialize(
        this, sizeof_star_self, wuffs_version, initialize_flags);
  }

  inline wuffs_base__image_decoder*
  upcast_as__wuffs_base__image_decoder() {
    return (wuffs_base__image_decoder*)this;
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_gif__decoder__set_quirk_enabled(this, a_quirk, a_enabled);
  }

  inline wuffs_base__status
  decode_image_config(
      wuffs_base__image_config* a_dst,
      wuffs_base__io_buffer* a_src) {
    return wuffs_gif__decoder__decode_image_config(this, a_dst, a_src);
  }

  inline wuffs_base__empty_struct
  set_report_metadata(
      uint32_t a_fourcc,
      bool a_report) {
    return wuffs_gif__decoder__set_report_metadata(this, a_fourcc, a_report);
  }

  inline wuffs_base__status
  tell_me_more(
      wuffs_base__io_buffer* a_dst,
      wuffs_base__more_information* a_minfo,
      wuffs_base__io_buffer* a_src) {
    return wuffs_gif__decoder__tell_me_more(this, a_dst, a_minfo, a_src);
  }

  inline uint32_t
  num_animation_loops() const {
    return wuffs_gif__decoder__num_animation_loops(this);
  }

  inline uint64_t
  num_decoded_frame_configs() const {
    return wuffs_gif__decoder__num_decoded_frame_configs(this);
  }

  inline uint64_t
  num_decoded_frames() const {
    return wuffs_gif__decoder__num_decoded_frames(this);
  }

  inline wuffs_base__rect_ie_u32
  frame_dirty_rect() const {
    return wuffs_gif__decoder__frame_dirty_rect(this);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_gif__decoder__workbuf_len(this);
  }

  inline wuffs_base__status
  restart_frame(
      uint64_t a_index,
      uint64_t a_io_position) {
    return wuffs_gif__decoder__restart_frame(this, a_index, a_io_position);
  }

  inline wuffs_base__status
  decode_frame_config(
      wuffs_base__frame_config* a_dst,
      wuffs_base__io_buffer* a_src) {
    return wuffs_gif__decoder__decode_frame_config(this, a_dst, a_src);
  }

  inline wuffs_base__status
  decode_frame(
      wuffs_base__pixel_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__pixel_blend a_blend,
      wuffs_base__slice_u8 a_workbuf,
      wuffs_base__decode_frame_options* a_opts) {
    return wuffs_gif__decoder__decode_frame(this, a_dst, a_src, a_blend, a_workbuf, a_opts);
  }

#endif  // __cplusplus
};  // struct wuffs_gif__decoder__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Status Codes

extern const char* wuffs_gzip__error__bad_checksum;
extern const char* wuffs_gzip__error__bad_compression_method;
extern const char* wuffs_gzip__error__bad_encoding_flags;
extern const char* wuffs_gzip__error__bad_header;

// ---------------- Public Consts

#define WUFFS_GZIP__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE 1

// ---------------- Struct Declarations

typedef struct wuffs_gzip__decoder__struct wuffs_gzip__decoder;

// ---------------- Public Initializer Prototypes

// For any given "wuffs_foo__bar* self", "wuffs_foo__bar__initialize(self,
// etc)" should be called before any other "wuffs_foo__bar__xxx(self, etc)".
//
// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.
// Pass 0 (or some combination of WUFFS_INITIALIZE__XXX) for initialize_flags.

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_gzip__decoder__initialize(
    wuffs_gzip__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags);

size_t
sizeof__wuffs_gzip__decoder();

// ---------------- Allocs

// These functions allocate and initialize Wuffs structs. They return NULL if
// memory allocation fails. If they return non-NULL, there is no need to call
// wuffs_foo__bar__initialize, but the caller is responsible for eventually
// calling free on the returned pointer. That pointer is effectively a C++
// std::unique_ptr<T, decltype(&free)>.

wuffs_gzip__decoder*
wuffs_gzip__decoder__alloc();

static inline wuffs_base__io_transformer*
wuffs_gzip__decoder__alloc_as__wuffs_base__io_transformer() {
  return (wuffs_base__io_transformer*)(wuffs_gzip__decoder__alloc());
}

// ---------------- Upcasts

static inline wuffs_base__io_transformer*
wuffs_gzip__decoder__upcast_as__wuffs_base__io_transformer(
    wuffs_gzip__decoder* p) {
  return (wuffs_base__io_transformer*)p;
}

// ---------------- Public Function Prototypes

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gzip__decoder__set_ignore_checksum(
    wuffs_gzip__decoder* self,
    bool a_ic);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gzip__decoder__set_quirk_enabled(
    wuffs_gzip__decoder* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_gzip__decoder__workbuf_len(
    const wuffs_gzip__decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gzip__decoder__transform_io(
    wuffs_gzip__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf);

// ---------------- Struct Definitions

// These structs' fields, and the sizeof them, are private implementation
// details that aren't guaranteed to be stable across Wuffs versions.
//
// See https://en.wikipedia.org/wiki/Opaque_pointer#C

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_gzip__decoder__struct {
  // Do not access the private_impl's or private_data's fields directly. There
  // is no API/ABI compatibility or safety guarantee if you do so. Instead, use
  // the wuffs_foo__bar__baz functions.
  //
  // It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct
  // can be stack allocated when WUFFS_IMPLEMENTATION is defined.

  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable vtable_for__wuffs_base__io_transformer;
    wuffs_base__vtable null_vtable;

    bool f_ignore_checksum;

    uint32_t p_transform_io[1];
  } private_impl;

  struct {
    wuffs_crc32__ieee_hasher f_checksum;
    wuffs_deflate__decoder f_flate;

    struct {
      uint8_t v_flags;
      uint32_t v_checksum_got;
      uint32_t v_decoded_length_got;
      uint32_t v_checksum_want;
      uint64_t scratch;
    } s_transform_io[1];
  } private_data;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_gzip__decoder, decltype(&free)>;

  // On failure, the alloc_etc functions return nullptr. They don't throw.

  static inline unique_ptr
  alloc() {
    return unique_ptr(wuffs_gzip__decoder__alloc(), &free);
  }

  static inline wuffs_base__io_transformer::unique_ptr
  alloc_as__wuffs_base__io_transformer() {
    return wuffs_base__io_transformer::unique_ptr(
        wuffs_gzip__decoder__alloc_as__wuffs_base__io_transformer(), &free);
  }
#endif  // (__cplusplus >= 201103L)

#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)
  // Disallow constructing or copying an object via standard C++ mechanisms,
  // e.g. the "new" operator, as this struct is intentionally opaque. Its total
  // size and field layout is not part of the public, stable, memory-safe API.
  // Use malloc or memcpy and the sizeof__wuffs_foo__bar function instead, and
  // call wuffs_foo__bar__baz methods (which all take a "this"-like pointer as
  // their first argument) rather than tweaking bar.private_impl.qux fields.
  //
  // In C, we can just leave wuffs_foo__bar as an incomplete type (unless
  // WUFFS_IMPLEMENTATION is #define'd). In C++, we define a complete type in
  // order to provide convenience methods. These forward on "this", so that you
  // can write "bar->baz(etc)" instead of "wuffs_foo__bar__baz(bar, etc)".
  wuffs_gzip__decoder__struct() = delete;
  wuffs_gzip__decoder__struct(const wuffs_gzip__decoder__struct&) = delete;
  wuffs_gzip__decoder__struct& operator=(
      const wuffs_gzip__decoder__struct&) = delete;

  // As above, the size of the struct is not part of the public API, and unless
  // WUFFS_IMPLEMENTATION is #define'd, this struct type T should be heap
  // allocated, not stack allocated. Its size is not intended to be known at
  // compile time, but it is unfortunately divulged as a side effect of
  // defining C++ convenience methods. Use "sizeof__T()", calling the function,
  // instead of "sizeof T", invoking the operator. To make the two values
  // different, so that passing the latter will be rejected by the initialize
  // function, we add an arbitrary amount of dead weight.
  uint8_t dead_weight[123000000];  // 123 MB.
#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)

  inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
  initialize(
      size_t sizeof_star_self,
      uint64_t wuffs_version,
      uint32_t initialize_flags) {
    return wuffs_gzip__decoder__initialize(
        this, sizeof_star_self, wuffs_version, initialize_flags);
  }

  inline wuffs_base__io_transformer*
  upcast_as__wuffs_base__io_transformer() {
    return (wuffs_base__io_transformer*)this;
  }

  inline wuffs_base__empty_struct
  set_ignore_checksum(
      bool a_ic) {
    return wuffs_gzip__decoder__set_ignore_checksum(this, a_ic);
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_gzip__decoder__set_quirk_enabled(this, a_quirk, a_enabled);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_gzip__decoder__workbuf_len(this);
  }

  inline wuffs_base__status
  transform_io(
      wuffs_base__io_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__slice_u8 a_workbuf) {
    return wuffs_gzip__decoder__transform_io(this, a_dst, a_src, a_workbuf);
  }

#endif  // __cplusplus
};  // struct wuffs_gzip__decoder__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Status Codes

extern const char* wuffs_json__error__bad_c0_control_code;
extern const char* wuffs_json__error__bad_utf_8;
extern const char* wuffs_json__error__bad_backslash_escape;
extern const char* wuffs_json__error__bad_input;
extern const char* wuffs_json__error__unsupported_number_length;
extern const char* wuffs_json__error__unsupported_recursion_depth;

// ---------------- Public Consts

#define WUFFS_JSON__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE 0

#define WUFFS_JSON__DECODER_DEPTH_MAX_INCL 1024

#define WUFFS_JSON__DECODER_DST_TOKEN_BUFFER_LENGTH_MIN_INCL 1

#define WUFFS_JSON__DECODER_SRC_IO_BUFFER_LENGTH_MIN_INCL 100

#define WUFFS_JSON__QUIRK_ALLOW_ASCII_CONTROL_CODES 1225364480

#define WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_A 1225364481

#define WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_CAPITAL_U 1225364482

#define WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_E 1225364483

#define WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_NEW_LINE 1225364484

#define WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_QUESTION_MARK 1225364485

#define WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_SINGLE_QUOTE 1225364486

#define WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_V 1225364487

#define WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_X 1225364488

#define WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_ZERO 1225364489

#define WUFFS_JSON__QUIRK_ALLOW_COMMENT_BLOCK 1225364490

#define WUFFS_JSON__QUIRK_ALLOW_COMMENT_LINE 1225364491

#define WUFFS_JSON__QUIRK_ALLOW_EXTRA_COMMA 1225364492

#define WUFFS_JSON__QUIRK_ALLOW_INF_NAN_NUMBERS 1225364493

#define WUFFS_JSON__QUIRK_ALLOW_LEADING_ASCII_RECORD_SEPARATOR 1225364494

#define WUFFS_JSON__QUIRK_ALLOW_LEADING_UNICODE_BYTE_ORDER_MARK 1225364495

#define WUFFS_JSON__QUIRK_ALLOW_TRAILING_NEW_LINE 1225364496

#define WUFFS_JSON__QUIRK_REPLACE_INVALID_UNICODE 1225364497

// ---------------- Struct Declarations

typedef struct wuffs_json__decoder__struct wuffs_json__decoder;

// ---------------- Public Initializer Prototypes

// For any given "wuffs_foo__bar* self", "wuffs_foo__bar__initialize(self,
// etc)" should be called before any other "wuffs_foo__bar__xxx(self, etc)".
//
// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.
// Pass 0 (or some combination of WUFFS_INITIALIZE__XXX) for initialize_flags.

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_json__decoder__initialize(
    wuffs_json__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags);

size_t
sizeof__wuffs_json__decoder();

// ---------------- Allocs

// These functions allocate and initialize Wuffs structs. They return NULL if
// memory allocation fails. If they return non-NULL, there is no need to call
// wuffs_foo__bar__initialize, but the caller is responsible for eventually
// calling free on the returned pointer. That pointer is effectively a C++
// std::unique_ptr<T, decltype(&free)>.

wuffs_json__decoder*
wuffs_json__decoder__alloc();

static inline wuffs_base__token_decoder*
wuffs_json__decoder__alloc_as__wuffs_base__token_decoder() {
  return (wuffs_base__token_decoder*)(wuffs_json__decoder__alloc());
}

// ---------------- Upcasts

static inline wuffs_base__token_decoder*
wuffs_json__decoder__upcast_as__wuffs_base__token_decoder(
    wuffs_json__decoder* p) {
  return (wuffs_base__token_decoder*)p;
}

// ---------------- Public Function Prototypes

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_json__decoder__set_quirk_enabled(
    wuffs_json__decoder* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_json__decoder__workbuf_len(
    const wuffs_json__decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_json__decoder__decode_tokens(
    wuffs_json__decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf);

// ---------------- Struct Definitions

// These structs' fields, and the sizeof them, are private implementation
// details that aren't guaranteed to be stable across Wuffs versions.
//
// See https://en.wikipedia.org/wiki/Opaque_pointer#C

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_json__decoder__struct {
  // Do not access the private_impl's or private_data's fields directly. There
  // is no API/ABI compatibility or safety guarantee if you do so. Instead, use
  // the wuffs_foo__bar__baz functions.
  //
  // It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct
  // can be stack allocated when WUFFS_IMPLEMENTATION is defined.

  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable vtable_for__wuffs_base__token_decoder;
    wuffs_base__vtable null_vtable;

    bool f_quirks[18];
    bool f_allow_leading_ars;
    bool f_allow_leading_ubom;
    bool f_end_of_data;

    uint32_t p_decode_tokens[1];
    uint32_t p_decode_leading[1];
    uint32_t p_decode_comment[1];
    uint32_t p_decode_inf_nan[1];
    uint32_t p_decode_trailing_new_line[1];
  } private_impl;

  struct {
    uint32_t f_stack[32];

    struct {
      uint32_t v_depth;
      uint32_t v_expect;
      uint32_t v_expect_after_value;
    } s_decode_tokens[1];
    struct {
      uint32_t v_neg;
    } s_decode_inf_nan[1];
  } private_data;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_json__decoder, decltype(&free)>;

  // On failure, the alloc_etc functions return nullptr. They don't throw.

  static inline unique_ptr
  alloc() {
    return unique_ptr(wuffs_json__decoder__alloc(), &free);
  }

  static inline wuffs_base__token_decoder::unique_ptr
  alloc_as__wuffs_base__token_decoder() {
    return wuffs_base__token_decoder::unique_ptr(
        wuffs_json__decoder__alloc_as__wuffs_base__token_decoder(), &free);
  }
#endif  // (__cplusplus >= 201103L)

#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)
  // Disallow constructing or copying an object via standard C++ mechanisms,
  // e.g. the "new" operator, as this struct is intentionally opaque. Its total
  // size and field layout is not part of the public, stable, memory-safe API.
  // Use malloc or memcpy and the sizeof__wuffs_foo__bar function instead, and
  // call wuffs_foo__bar__baz methods (which all take a "this"-like pointer as
  // their first argument) rather than tweaking bar.private_impl.qux fields.
  //
  // In C, we can just leave wuffs_foo__bar as an incomplete type (unless
  // WUFFS_IMPLEMENTATION is #define'd). In C++, we define a complete type in
  // order to provide convenience methods. These forward on "this", so that you
  // can write "bar->baz(etc)" instead of "wuffs_foo__bar__baz(bar, etc)".
  wuffs_json__decoder__struct() = delete;
  wuffs_json__decoder__struct(const wuffs_json__decoder__struct&) = delete;
  wuffs_json__decoder__struct& operator=(
      const wuffs_json__decoder__struct&) = delete;

  // As above, the size of the struct is not part of the public API, and unless
  // WUFFS_IMPLEMENTATION is #define'd, this struct type T should be heap
  // allocated, not stack allocated. Its size is not intended to be known at
  // compile time, but it is unfortunately divulged as a side effect of
  // defining C++ convenience methods. Use "sizeof__T()", calling the function,
  // instead of "sizeof T", invoking the operator. To make the two values
  // different, so that passing the latter will be rejected by the initialize
  // function, we add an arbitrary amount of dead weight.
  uint8_t dead_weight[123000000];  // 123 MB.
#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)

  inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
  initialize(
      size_t sizeof_star_self,
      uint64_t wuffs_version,
      uint32_t initialize_flags) {
    return wuffs_json__decoder__initialize(
        this, sizeof_star_self, wuffs_version, initialize_flags);
  }

  inline wuffs_base__token_decoder*
  upcast_as__wuffs_base__token_decoder() {
    return (wuffs_base__token_decoder*)this;
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_json__decoder__set_quirk_enabled(this, a_quirk, a_enabled);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_json__decoder__workbuf_len(this);
  }

  inline wuffs_base__status
  decode_tokens(
      wuffs_base__token_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__slice_u8 a_workbuf) {
    return wuffs_json__decoder__decode_tokens(this, a_dst, a_src, a_workbuf);
  }

#endif  // __cplusplus
};  // struct wuffs_json__decoder__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Status Codes

extern const char* wuffs_wbmp__error__bad_header;

// ---------------- Public Consts

#define WUFFS_WBMP__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE 0

// ---------------- Struct Declarations

typedef struct wuffs_wbmp__decoder__struct wuffs_wbmp__decoder;

// ---------------- Public Initializer Prototypes

// For any given "wuffs_foo__bar* self", "wuffs_foo__bar__initialize(self,
// etc)" should be called before any other "wuffs_foo__bar__xxx(self, etc)".
//
// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.
// Pass 0 (or some combination of WUFFS_INITIALIZE__XXX) for initialize_flags.

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_wbmp__decoder__initialize(
    wuffs_wbmp__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags);

size_t
sizeof__wuffs_wbmp__decoder();

// ---------------- Allocs

// These functions allocate and initialize Wuffs structs. They return NULL if
// memory allocation fails. If they return non-NULL, there is no need to call
// wuffs_foo__bar__initialize, but the caller is responsible for eventually
// calling free on the returned pointer. That pointer is effectively a C++
// std::unique_ptr<T, decltype(&free)>.

wuffs_wbmp__decoder*
wuffs_wbmp__decoder__alloc();

static inline wuffs_base__image_decoder*
wuffs_wbmp__decoder__alloc_as__wuffs_base__image_decoder() {
  return (wuffs_base__image_decoder*)(wuffs_wbmp__decoder__alloc());
}

// ---------------- Upcasts

static inline wuffs_base__image_decoder*
wuffs_wbmp__decoder__upcast_as__wuffs_base__image_decoder(
    wuffs_wbmp__decoder* p) {
  return (wuffs_base__image_decoder*)p;
}

// ---------------- Public Function Prototypes

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_wbmp__decoder__set_quirk_enabled(
    wuffs_wbmp__decoder* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_wbmp__decoder__decode_image_config(
    wuffs_wbmp__decoder* self,
    wuffs_base__image_config* a_dst,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_wbmp__decoder__decode_frame_config(
    wuffs_wbmp__decoder* self,
    wuffs_base__frame_config* a_dst,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_wbmp__decoder__decode_frame(
    wuffs_wbmp__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend,
    wuffs_base__slice_u8 a_workbuf,
    wuffs_base__decode_frame_options* a_opts);

WUFFS_BASE__MAYBE_STATIC wuffs_base__rect_ie_u32
wuffs_wbmp__decoder__frame_dirty_rect(
    const wuffs_wbmp__decoder* self);

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_wbmp__decoder__num_animation_loops(
    const wuffs_wbmp__decoder* self);

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_wbmp__decoder__num_decoded_frame_configs(
    const wuffs_wbmp__decoder* self);

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_wbmp__decoder__num_decoded_frames(
    const wuffs_wbmp__decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_wbmp__decoder__restart_frame(
    wuffs_wbmp__decoder* self,
    uint64_t a_index,
    uint64_t a_io_position);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_wbmp__decoder__set_report_metadata(
    wuffs_wbmp__decoder* self,
    uint32_t a_fourcc,
    bool a_report);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_wbmp__decoder__tell_me_more(
    wuffs_wbmp__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__more_information* a_minfo,
    wuffs_base__io_buffer* a_src);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_wbmp__decoder__workbuf_len(
    const wuffs_wbmp__decoder* self);

// ---------------- Struct Definitions

// These structs' fields, and the sizeof them, are private implementation
// details that aren't guaranteed to be stable across Wuffs versions.
//
// See https://en.wikipedia.org/wiki/Opaque_pointer#C

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_wbmp__decoder__struct {
  // Do not access the private_impl's or private_data's fields directly. There
  // is no API/ABI compatibility or safety guarantee if you do so. Instead, use
  // the wuffs_foo__bar__baz functions.
  //
  // It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct
  // can be stack allocated when WUFFS_IMPLEMENTATION is defined.

  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable vtable_for__wuffs_base__image_decoder;
    wuffs_base__vtable null_vtable;

    uint32_t f_width;
    uint32_t f_height;
    uint8_t f_call_sequence;
    uint64_t f_frame_config_io_position;
    wuffs_base__pixel_swizzler f_swizzler;

    uint32_t p_decode_image_config[1];
    uint32_t p_decode_frame_config[1];
    uint32_t p_decode_frame[1];
    uint32_t p_skip_frame[1];
  } private_impl;

  struct {
    struct {
      uint32_t v_i;
      uint32_t v_x32;
    } s_decode_image_config[1];
    struct {
      uint64_t v_dst_bytes_per_pixel;
      uint32_t v_dst_x;
      uint32_t v_dst_y;
      uint8_t v_src[1];
      uint8_t v_c;
    } s_decode_frame[1];
    struct {
      uint64_t scratch;
    } s_skip_frame[1];
  } private_data;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_wbmp__decoder, decltype(&free)>;

  // On failure, the alloc_etc functions return nullptr. They don't throw.

  static inline unique_ptr
  alloc() {
    return unique_ptr(wuffs_wbmp__decoder__alloc(), &free);
  }

  static inline wuffs_base__image_decoder::unique_ptr
  alloc_as__wuffs_base__image_decoder() {
    return wuffs_base__image_decoder::unique_ptr(
        wuffs_wbmp__decoder__alloc_as__wuffs_base__image_decoder(), &free);
  }
#endif  // (__cplusplus >= 201103L)

#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)
  // Disallow constructing or copying an object via standard C++ mechanisms,
  // e.g. the "new" operator, as this struct is intentionally opaque. Its total
  // size and field layout is not part of the public, stable, memory-safe API.
  // Use malloc or memcpy and the sizeof__wuffs_foo__bar function instead, and
  // call wuffs_foo__bar__baz methods (which all take a "this"-like pointer as
  // their first argument) rather than tweaking bar.private_impl.qux fields.
  //
  // In C, we can just leave wuffs_foo__bar as an incomplete type (unless
  // WUFFS_IMPLEMENTATION is #define'd). In C++, we define a complete type in
  // order to provide convenience methods. These forward on "this", so that you
  // can write "bar->baz(etc)" instead of "wuffs_foo__bar__baz(bar, etc)".
  wuffs_wbmp__decoder__struct() = delete;
  wuffs_wbmp__decoder__struct(const wuffs_wbmp__decoder__struct&) = delete;
  wuffs_wbmp__decoder__struct& operator=(
      const wuffs_wbmp__decoder__struct&) = delete;

  // As above, the size of the struct is not part of the public API, and unless
  // WUFFS_IMPLEMENTATION is #define'd, this struct type T should be heap
  // allocated, not stack allocated. Its size is not intended to be known at
  // compile time, but it is unfortunately divulged as a side effect of
  // defining C++ convenience methods. Use "sizeof__T()", calling the function,
  // instead of "sizeof T", invoking the operator. To make the two values
  // different, so that passing the latter will be rejected by the initialize
  // function, we add an arbitrary amount of dead weight.
  uint8_t dead_weight[123000000];  // 123 MB.
#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)

  inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
  initialize(
      size_t sizeof_star_self,
      uint64_t wuffs_version,
      uint32_t initialize_flags) {
    return wuffs_wbmp__decoder__initialize(
        this, sizeof_star_self, wuffs_version, initialize_flags);
  }

  inline wuffs_base__image_decoder*
  upcast_as__wuffs_base__image_decoder() {
    return (wuffs_base__image_decoder*)this;
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_wbmp__decoder__set_quirk_enabled(this, a_quirk, a_enabled);
  }

  inline wuffs_base__status
  decode_image_config(
      wuffs_base__image_config* a_dst,
      wuffs_base__io_buffer* a_src) {
    return wuffs_wbmp__decoder__decode_image_config(this, a_dst, a_src);
  }

  inline wuffs_base__status
  decode_frame_config(
      wuffs_base__frame_config* a_dst,
      wuffs_base__io_buffer* a_src) {
    return wuffs_wbmp__decoder__decode_frame_config(this, a_dst, a_src);
  }

  inline wuffs_base__status
  decode_frame(
      wuffs_base__pixel_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__pixel_blend a_blend,
      wuffs_base__slice_u8 a_workbuf,
      wuffs_base__decode_frame_options* a_opts) {
    return wuffs_wbmp__decoder__decode_frame(this, a_dst, a_src, a_blend, a_workbuf, a_opts);
  }

  inline wuffs_base__rect_ie_u32
  frame_dirty_rect() const {
    return wuffs_wbmp__decoder__frame_dirty_rect(this);
  }

  inline uint32_t
  num_animation_loops() const {
    return wuffs_wbmp__decoder__num_animation_loops(this);
  }

  inline uint64_t
  num_decoded_frame_configs() const {
    return wuffs_wbmp__decoder__num_decoded_frame_configs(this);
  }

  inline uint64_t
  num_decoded_frames() const {
    return wuffs_wbmp__decoder__num_decoded_frames(this);
  }

  inline wuffs_base__status
  restart_frame(
      uint64_t a_index,
      uint64_t a_io_position) {
    return wuffs_wbmp__decoder__restart_frame(this, a_index, a_io_position);
  }

  inline wuffs_base__empty_struct
  set_report_metadata(
      uint32_t a_fourcc,
      bool a_report) {
    return wuffs_wbmp__decoder__set_report_metadata(this, a_fourcc, a_report);
  }

  inline wuffs_base__status
  tell_me_more(
      wuffs_base__io_buffer* a_dst,
      wuffs_base__more_information* a_minfo,
      wuffs_base__io_buffer* a_src) {
    return wuffs_wbmp__decoder__tell_me_more(this, a_dst, a_minfo, a_src);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_wbmp__decoder__workbuf_len(this);
  }

#endif  // __cplusplus
};  // struct wuffs_wbmp__decoder__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Status Codes

extern const char* wuffs_zlib__note__dictionary_required;
extern const char* wuffs_zlib__error__bad_checksum;
extern const char* wuffs_zlib__error__bad_compression_method;
extern const char* wuffs_zlib__error__bad_compression_window_size;
extern const char* wuffs_zlib__error__bad_parity_check;
extern const char* wuffs_zlib__error__incorrect_dictionary;

// ---------------- Public Consts

#define WUFFS_ZLIB__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE 1

// ---------------- Struct Declarations

typedef struct wuffs_zlib__decoder__struct wuffs_zlib__decoder;

// ---------------- Public Initializer Prototypes

// For any given "wuffs_foo__bar* self", "wuffs_foo__bar__initialize(self,
// etc)" should be called before any other "wuffs_foo__bar__xxx(self, etc)".
//
// Pass sizeof(*self) and WUFFS_VERSION for sizeof_star_self and wuffs_version.
// Pass 0 (or some combination of WUFFS_INITIALIZE__XXX) for initialize_flags.

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_zlib__decoder__initialize(
    wuffs_zlib__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags);

size_t
sizeof__wuffs_zlib__decoder();

// ---------------- Allocs

// These functions allocate and initialize Wuffs structs. They return NULL if
// memory allocation fails. If they return non-NULL, there is no need to call
// wuffs_foo__bar__initialize, but the caller is responsible for eventually
// calling free on the returned pointer. That pointer is effectively a C++
// std::unique_ptr<T, decltype(&free)>.

wuffs_zlib__decoder*
wuffs_zlib__decoder__alloc();

static inline wuffs_base__io_transformer*
wuffs_zlib__decoder__alloc_as__wuffs_base__io_transformer() {
  return (wuffs_base__io_transformer*)(wuffs_zlib__decoder__alloc());
}

// ---------------- Upcasts

static inline wuffs_base__io_transformer*
wuffs_zlib__decoder__upcast_as__wuffs_base__io_transformer(
    wuffs_zlib__decoder* p) {
  return (wuffs_base__io_transformer*)p;
}

// ---------------- Public Function Prototypes

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_zlib__decoder__dictionary_id(
    const wuffs_zlib__decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_zlib__decoder__add_dictionary(
    wuffs_zlib__decoder* self,
    wuffs_base__slice_u8 a_dict);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_zlib__decoder__set_ignore_checksum(
    wuffs_zlib__decoder* self,
    bool a_ic);

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_zlib__decoder__set_quirk_enabled(
    wuffs_zlib__decoder* self,
    uint32_t a_quirk,
    bool a_enabled);

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_zlib__decoder__workbuf_len(
    const wuffs_zlib__decoder* self);

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_zlib__decoder__transform_io(
    wuffs_zlib__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf);

// ---------------- Struct Definitions

// These structs' fields, and the sizeof them, are private implementation
// details that aren't guaranteed to be stable across Wuffs versions.
//
// See https://en.wikipedia.org/wiki/Opaque_pointer#C

#if defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

struct wuffs_zlib__decoder__struct {
  // Do not access the private_impl's or private_data's fields directly. There
  // is no API/ABI compatibility or safety guarantee if you do so. Instead, use
  // the wuffs_foo__bar__baz functions.
  //
  // It is a struct, not a struct*, so that the outermost wuffs_foo__bar struct
  // can be stack allocated when WUFFS_IMPLEMENTATION is defined.

  struct {
    uint32_t magic;
    uint32_t active_coroutine;
    wuffs_base__vtable vtable_for__wuffs_base__io_transformer;
    wuffs_base__vtable null_vtable;

    bool f_bad_call_sequence;
    bool f_header_complete;
    bool f_got_dictionary;
    bool f_want_dictionary;
    bool f_ignore_checksum;
    uint32_t f_dict_id_got;
    uint32_t f_dict_id_want;

    uint32_t p_transform_io[1];
  } private_impl;

  struct {
    wuffs_adler32__hasher f_checksum;
    wuffs_adler32__hasher f_dict_id_hasher;
    wuffs_deflate__decoder f_flate;

    struct {
      uint32_t v_checksum_got;
      uint64_t scratch;
    } s_transform_io[1];
  } private_data;

#ifdef __cplusplus
#if (__cplusplus >= 201103L)
  using unique_ptr = std::unique_ptr<wuffs_zlib__decoder, decltype(&free)>;

  // On failure, the alloc_etc functions return nullptr. They don't throw.

  static inline unique_ptr
  alloc() {
    return unique_ptr(wuffs_zlib__decoder__alloc(), &free);
  }

  static inline wuffs_base__io_transformer::unique_ptr
  alloc_as__wuffs_base__io_transformer() {
    return wuffs_base__io_transformer::unique_ptr(
        wuffs_zlib__decoder__alloc_as__wuffs_base__io_transformer(), &free);
  }
#endif  // (__cplusplus >= 201103L)

#if (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)
  // Disallow constructing or copying an object via standard C++ mechanisms,
  // e.g. the "new" operator, as this struct is intentionally opaque. Its total
  // size and field layout is not part of the public, stable, memory-safe API.
  // Use malloc or memcpy and the sizeof__wuffs_foo__bar function instead, and
  // call wuffs_foo__bar__baz methods (which all take a "this"-like pointer as
  // their first argument) rather than tweaking bar.private_impl.qux fields.
  //
  // In C, we can just leave wuffs_foo__bar as an incomplete type (unless
  // WUFFS_IMPLEMENTATION is #define'd). In C++, we define a complete type in
  // order to provide convenience methods. These forward on "this", so that you
  // can write "bar->baz(etc)" instead of "wuffs_foo__bar__baz(bar, etc)".
  wuffs_zlib__decoder__struct() = delete;
  wuffs_zlib__decoder__struct(const wuffs_zlib__decoder__struct&) = delete;
  wuffs_zlib__decoder__struct& operator=(
      const wuffs_zlib__decoder__struct&) = delete;

  // As above, the size of the struct is not part of the public API, and unless
  // WUFFS_IMPLEMENTATION is #define'd, this struct type T should be heap
  // allocated, not stack allocated. Its size is not intended to be known at
  // compile time, but it is unfortunately divulged as a side effect of
  // defining C++ convenience methods. Use "sizeof__T()", calling the function,
  // instead of "sizeof T", invoking the operator. To make the two values
  // different, so that passing the latter will be rejected by the initialize
  // function, we add an arbitrary amount of dead weight.
  uint8_t dead_weight[123000000];  // 123 MB.
#endif  // (__cplusplus >= 201103L) && !defined(WUFFS_IMPLEMENTATION)

  inline wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
  initialize(
      size_t sizeof_star_self,
      uint64_t wuffs_version,
      uint32_t initialize_flags) {
    return wuffs_zlib__decoder__initialize(
        this, sizeof_star_self, wuffs_version, initialize_flags);
  }

  inline wuffs_base__io_transformer*
  upcast_as__wuffs_base__io_transformer() {
    return (wuffs_base__io_transformer*)this;
  }

  inline uint32_t
  dictionary_id() const {
    return wuffs_zlib__decoder__dictionary_id(this);
  }

  inline wuffs_base__empty_struct
  add_dictionary(
      wuffs_base__slice_u8 a_dict) {
    return wuffs_zlib__decoder__add_dictionary(this, a_dict);
  }

  inline wuffs_base__empty_struct
  set_ignore_checksum(
      bool a_ic) {
    return wuffs_zlib__decoder__set_ignore_checksum(this, a_ic);
  }

  inline wuffs_base__empty_struct
  set_quirk_enabled(
      uint32_t a_quirk,
      bool a_enabled) {
    return wuffs_zlib__decoder__set_quirk_enabled(this, a_quirk, a_enabled);
  }

  inline wuffs_base__range_ii_u64
  workbuf_len() const {
    return wuffs_zlib__decoder__workbuf_len(this);
  }

  inline wuffs_base__status
  transform_io(
      wuffs_base__io_buffer* a_dst,
      wuffs_base__io_buffer* a_src,
      wuffs_base__slice_u8 a_workbuf) {
    return wuffs_zlib__decoder__transform_io(this, a_dst, a_src, a_workbuf);
  }

#endif  // __cplusplus
};  // struct wuffs_zlib__decoder__struct

#endif  // defined(__cplusplus) || defined(WUFFS_IMPLEMENTATION)

#ifdef __cplusplus
}  // extern "C"
#endif

// WUFFS C HEADER ENDS HERE.
#ifdef WUFFS_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- Fundamentals

// WUFFS_BASE__MAGIC is a magic number to check that initializers are called.
// It's not foolproof, given C doesn't automatically zero memory before use,
// but it should catch 99.99% of cases.
//
// Its (non-zero) value is arbitrary, based on md5sum("wuffs").
#define WUFFS_BASE__MAGIC ((uint32_t)0x3CCB6C71)

// WUFFS_BASE__DISABLED is a magic number to indicate that a non-recoverable
// error was previously encountered.
//
// Its (non-zero) value is arbitrary, based on md5sum("disabled").
#define WUFFS_BASE__DISABLED ((uint32_t)0x075AE3D2)

// Denote intentional fallthroughs for -Wimplicit-fallthrough.
//
// The order matters here. Clang also defines "__GNUC__".
#if defined(__clang__) && defined(__cplusplus) && (__cplusplus >= 201103L)
#define WUFFS_BASE__FALLTHROUGH [[clang::fallthrough]]
#elif !defined(__clang__) && defined(__GNUC__) && (__GNUC__ >= 7)
#define WUFFS_BASE__FALLTHROUGH __attribute__((fallthrough))
#else
#define WUFFS_BASE__FALLTHROUGH
#endif

// Use switch cases for coroutine suspension points, similar to the technique
// in https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
//
// We use trivial macros instead of an explicit assignment and case statement
// so that clang-format doesn't get confused by the unusual "case"s.
#define WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0 case 0:;
#define WUFFS_BASE__COROUTINE_SUSPENSION_POINT(n) \
  coro_susp_point = n;                            \
  WUFFS_BASE__FALLTHROUGH;                        \
  case n:;

#define WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(n) \
  if (!status.repr) {                                           \
    goto ok;                                                    \
  } else if (*status.repr != '$') {                             \
    goto exit;                                                  \
  }                                                             \
  coro_susp_point = n;                                          \
  goto suspend;                                                 \
  case n:;

// Clang also defines "__GNUC__".
#if defined(__GNUC__)
#define WUFFS_BASE__LIKELY(expr) (__builtin_expect(!!(expr), 1))
#define WUFFS_BASE__UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#else
#define WUFFS_BASE__LIKELY(expr) (expr)
#define WUFFS_BASE__UNLIKELY(expr) (expr)
#endif

// --------

static inline wuffs_base__empty_struct  //
wuffs_base__ignore_status(wuffs_base__status z) {
  return wuffs_base__make_empty_struct();
}

static inline wuffs_base__status  //
wuffs_base__status__ensure_not_a_suspension(wuffs_base__status z) {
  if (z.repr && (*z.repr == '$')) {
    z.repr = wuffs_base__error__cannot_return_a_suspension;
  }
  return z;
}

// ---------------- Numeric Types

extern const uint8_t wuffs_base__low_bits_mask__u8[9];
extern const uint16_t wuffs_base__low_bits_mask__u16[17];
extern const uint32_t wuffs_base__low_bits_mask__u32[33];
extern const uint64_t wuffs_base__low_bits_mask__u64[65];

#define WUFFS_BASE__LOW_BITS_MASK__U8(n) (wuffs_base__low_bits_mask__u8[n])
#define WUFFS_BASE__LOW_BITS_MASK__U16(n) (wuffs_base__low_bits_mask__u16[n])
#define WUFFS_BASE__LOW_BITS_MASK__U32(n) (wuffs_base__low_bits_mask__u32[n])
#define WUFFS_BASE__LOW_BITS_MASK__U64(n) (wuffs_base__low_bits_mask__u64[n])

// --------

static inline void  //
wuffs_base__u8__sat_add_indirect(uint8_t* x, uint8_t y) {
  *x = wuffs_base__u8__sat_add(*x, y);
}

static inline void  //
wuffs_base__u8__sat_sub_indirect(uint8_t* x, uint8_t y) {
  *x = wuffs_base__u8__sat_sub(*x, y);
}

static inline void  //
wuffs_base__u16__sat_add_indirect(uint16_t* x, uint16_t y) {
  *x = wuffs_base__u16__sat_add(*x, y);
}

static inline void  //
wuffs_base__u16__sat_sub_indirect(uint16_t* x, uint16_t y) {
  *x = wuffs_base__u16__sat_sub(*x, y);
}

static inline void  //
wuffs_base__u32__sat_add_indirect(uint32_t* x, uint32_t y) {
  *x = wuffs_base__u32__sat_add(*x, y);
}

static inline void  //
wuffs_base__u32__sat_sub_indirect(uint32_t* x, uint32_t y) {
  *x = wuffs_base__u32__sat_sub(*x, y);
}

static inline void  //
wuffs_base__u64__sat_add_indirect(uint64_t* x, uint64_t y) {
  *x = wuffs_base__u64__sat_add(*x, y);
}

static inline void  //
wuffs_base__u64__sat_sub_indirect(uint64_t* x, uint64_t y) {
  *x = wuffs_base__u64__sat_sub(*x, y);
}

// ---------------- Slices and Tables

// wuffs_base__slice_u8__prefix returns up to the first up_to bytes of s.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__prefix(wuffs_base__slice_u8 s, uint64_t up_to) {
  if ((uint64_t)(s.len) > up_to) {
    s.len = up_to;
  }
  return s;
}

// wuffs_base__slice_u8__suffix returns up to the last up_to bytes of s.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__suffix(wuffs_base__slice_u8 s, uint64_t up_to) {
  if ((uint64_t)(s.len) > up_to) {
    s.ptr += (uint64_t)(s.len) - up_to;
    s.len = up_to;
  }
  return s;
}

// wuffs_base__slice_u8__copy_from_slice calls memmove(dst.ptr, src.ptr, len)
// where len is the minimum of dst.len and src.len.
//
// Passing a wuffs_base__slice_u8 with all fields NULL or zero (a valid, empty
// slice) is valid and results in a no-op.
static inline uint64_t  //
wuffs_base__slice_u8__copy_from_slice(wuffs_base__slice_u8 dst,
                                      wuffs_base__slice_u8 src) {
  size_t len = dst.len < src.len ? dst.len : src.len;
  if (len > 0) {
    memmove(dst.ptr, src.ptr, len);
  }
  return len;
}

// --------

static inline wuffs_base__slice_u8  //
wuffs_base__table_u8__row(wuffs_base__table_u8 t, uint32_t y) {
  if (y < t.height) {
    return wuffs_base__make_slice_u8(t.ptr + (t.stride * y), t.width);
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

// ---------------- Slices and Tables (Utility)

#define wuffs_base__utility__empty_slice_u8 wuffs_base__empty_slice_u8

// ---------------- Ranges and Rects

static inline uint32_t  //
wuffs_base__range_ii_u32__get_min_incl(const wuffs_base__range_ii_u32* r) {
  return r->min_incl;
}

static inline uint32_t  //
wuffs_base__range_ii_u32__get_max_incl(const wuffs_base__range_ii_u32* r) {
  return r->max_incl;
}

static inline uint32_t  //
wuffs_base__range_ie_u32__get_min_incl(const wuffs_base__range_ie_u32* r) {
  return r->min_incl;
}

static inline uint32_t  //
wuffs_base__range_ie_u32__get_max_excl(const wuffs_base__range_ie_u32* r) {
  return r->max_excl;
}

static inline uint64_t  //
wuffs_base__range_ii_u64__get_min_incl(const wuffs_base__range_ii_u64* r) {
  return r->min_incl;
}

static inline uint64_t  //
wuffs_base__range_ii_u64__get_max_incl(const wuffs_base__range_ii_u64* r) {
  return r->max_incl;
}

static inline uint64_t  //
wuffs_base__range_ie_u64__get_min_incl(const wuffs_base__range_ie_u64* r) {
  return r->min_incl;
}

static inline uint64_t  //
wuffs_base__range_ie_u64__get_max_excl(const wuffs_base__range_ie_u64* r) {
  return r->max_excl;
}

// ---------------- Ranges and Rects (Utility)

#define wuffs_base__utility__empty_range_ii_u32 wuffs_base__empty_range_ii_u32
#define wuffs_base__utility__empty_range_ie_u32 wuffs_base__empty_range_ie_u32
#define wuffs_base__utility__empty_range_ii_u64 wuffs_base__empty_range_ii_u64
#define wuffs_base__utility__empty_range_ie_u64 wuffs_base__empty_range_ie_u64
#define wuffs_base__utility__empty_rect_ii_u32 wuffs_base__empty_rect_ii_u32
#define wuffs_base__utility__empty_rect_ie_u32 wuffs_base__empty_rect_ie_u32
#define wuffs_base__utility__make_range_ii_u32 wuffs_base__make_range_ii_u32
#define wuffs_base__utility__make_range_ie_u32 wuffs_base__make_range_ie_u32
#define wuffs_base__utility__make_range_ii_u64 wuffs_base__make_range_ii_u64
#define wuffs_base__utility__make_range_ie_u64 wuffs_base__make_range_ie_u64
#define wuffs_base__utility__make_rect_ii_u32 wuffs_base__make_rect_ii_u32
#define wuffs_base__utility__make_rect_ie_u32 wuffs_base__make_rect_ie_u32

// ---------------- I/O

static inline uint64_t  //
wuffs_base__io__count_since(uint64_t mark, uint64_t index) {
  if (index >= mark) {
    return index - mark;
  }
  return 0;
}

static inline wuffs_base__slice_u8  //
wuffs_base__io__since(uint64_t mark, uint64_t index, uint8_t* ptr) {
  if (index >= mark) {
    return wuffs_base__make_slice_u8(ptr + mark, index - mark);
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

// --------

static inline uint32_t  //
wuffs_base__io_reader__limited_copy_u32_to_slice(const uint8_t** ptr_iop_r,
                                                 const uint8_t* io2_r,
                                                 uint32_t length,
                                                 wuffs_base__slice_u8 dst) {
  const uint8_t* iop_r = *ptr_iop_r;
  size_t n = dst.len;
  if (n > length) {
    n = length;
  }
  if (n > ((size_t)(io2_r - iop_r))) {
    n = (size_t)(io2_r - iop_r);
  }
  if (n > 0) {
    memmove(dst.ptr, iop_r, n);
    *ptr_iop_r += n;
  }
  return (uint32_t)(n);
}

// wuffs_base__io_reader__match7 returns whether the io_reader's upcoming bytes
// start with the given prefix (up to 7 bytes long). It is peek-like, not
// read-like, in that there are no side-effects.
//
// The low 3 bits of a hold the prefix length, n.
//
// The high 56 bits of a hold the prefix itself, in little-endian order. The
// first prefix byte is in bits 8..=15, the second prefix byte is in bits
// 16..=23, etc. The high (8 * (7 - n)) bits are ignored.
//
// There are three possible return values:
//  - 0 means success.
//  - 1 means inconclusive, equivalent to "$short read".
//  - 2 means failure.
static inline uint32_t  //
wuffs_base__io_reader__match7(const uint8_t* iop_r,
                              const uint8_t* io2_r,
                              wuffs_base__io_buffer* r,
                              uint64_t a) {
  uint32_t n = a & 7;
  a >>= 8;
  if ((io2_r - iop_r) >= 8) {
    uint64_t x = wuffs_base__load_u64le__no_bounds_check(iop_r);
    uint32_t shift = 8 * (8 - n);
    return ((a << shift) == (x << shift)) ? 0 : 2;
  }
  for (; n > 0; n--) {
    if (iop_r >= io2_r) {
      return (r && r->meta.closed) ? 2 : 1;
    } else if (*iop_r != ((uint8_t)(a))) {
      return 2;
    }
    iop_r++;
    a >>= 8;
  }
  return 0;
}

static inline wuffs_base__io_buffer*  //
wuffs_base__io_reader__set(wuffs_base__io_buffer* b,
                           const uint8_t** ptr_iop_r,
                           const uint8_t** ptr_io0_r,
                           const uint8_t** ptr_io1_r,
                           const uint8_t** ptr_io2_r,
                           wuffs_base__slice_u8 data) {
  b->data = data;
  b->meta.wi = data.len;
  b->meta.ri = 0;
  b->meta.pos = 0;
  b->meta.closed = false;

  *ptr_iop_r = data.ptr;
  *ptr_io0_r = data.ptr;
  *ptr_io1_r = data.ptr;
  *ptr_io2_r = data.ptr + data.len;

  return b;
}

// --------

static inline uint64_t  //
wuffs_base__io_writer__copy_from_slice(uint8_t** ptr_iop_w,
                                       uint8_t* io2_w,
                                       wuffs_base__slice_u8 src) {
  uint8_t* iop_w = *ptr_iop_w;
  size_t n = src.len;
  if (n > ((size_t)(io2_w - iop_w))) {
    n = (size_t)(io2_w - iop_w);
  }
  if (n > 0) {
    memmove(iop_w, src.ptr, n);
    *ptr_iop_w += n;
  }
  return (uint64_t)(n);
}

static inline uint32_t  //
wuffs_base__io_writer__limited_copy_u32_from_history(uint8_t** ptr_iop_w,
                                                     uint8_t* io1_w,
                                                     uint8_t* io2_w,
                                                     uint32_t length,
                                                     uint32_t distance) {
  if (!distance) {
    return 0;
  }
  uint8_t* p = *ptr_iop_w;
  if ((size_t)(p - io1_w) < (size_t)(distance)) {
    return 0;
  }
  uint8_t* q = p - distance;
  size_t n = (size_t)(io2_w - p);
  if ((size_t)(length) > n) {
    length = (uint32_t)(n);
  } else {
    n = (size_t)(length);
  }
  // TODO: unrolling by 3 seems best for the std/deflate benchmarks, but that
  // is mostly because 3 is the minimum length for the deflate format. This
  // function implementation shouldn't overfit to that one format. Perhaps the
  // limited_copy_u32_from_history Wuffs method should also take an unroll hint
  // argument, and the cgen can look if that argument is the constant
  // expression '3'.
  //
  // See also wuffs_base__io_writer__limited_copy_u32_from_history_fast below.
  //
  // Alternatively or additionally, have a sloppy_limited_copy_u32_from_history
  // method that copies 8 bytes at a time, which can more than length bytes?
  for (; n >= 3; n -= 3) {
    *p++ = *q++;
    *p++ = *q++;
    *p++ = *q++;
  }
  for (; n; n--) {
    *p++ = *q++;
  }
  *ptr_iop_w = p;
  return length;
}

// wuffs_base__io_writer__limited_copy_u32_from_history_fast is like the
// wuffs_base__io_writer__limited_copy_u32_from_history function above, but has
// stronger pre-conditions. The caller needs to prove that:
//  - distance >  0
//  - distance <= (*ptr_iop_w - io1_w)
//  - length   <= (io2_w      - *ptr_iop_w)
static inline uint32_t  //
wuffs_base__io_writer__limited_copy_u32_from_history_fast(uint8_t** ptr_iop_w,
                                                          uint8_t* io1_w,
                                                          uint8_t* io2_w,
                                                          uint32_t length,
                                                          uint32_t distance) {
  uint8_t* p = *ptr_iop_w;
  uint8_t* q = p - distance;
  uint32_t n = length;
  for (; n >= 3; n -= 3) {
    *p++ = *q++;
    *p++ = *q++;
    *p++ = *q++;
  }
  for (; n; n--) {
    *p++ = *q++;
  }
  *ptr_iop_w = p;
  return length;
}

static inline uint32_t  //
wuffs_base__io_writer__limited_copy_u32_from_reader(uint8_t** ptr_iop_w,
                                                    uint8_t* io2_w,
                                                    uint32_t length,
                                                    const uint8_t** ptr_iop_r,
                                                    const uint8_t* io2_r) {
  uint8_t* iop_w = *ptr_iop_w;
  size_t n = length;
  if (n > ((size_t)(io2_w - iop_w))) {
    n = (size_t)(io2_w - iop_w);
  }
  const uint8_t* iop_r = *ptr_iop_r;
  if (n > ((size_t)(io2_r - iop_r))) {
    n = (size_t)(io2_r - iop_r);
  }
  if (n > 0) {
    memmove(iop_w, iop_r, n);
    *ptr_iop_w += n;
    *ptr_iop_r += n;
  }
  return (uint32_t)(n);
}

static inline uint32_t  //
wuffs_base__io_writer__limited_copy_u32_from_slice(uint8_t** ptr_iop_w,
                                                   uint8_t* io2_w,
                                                   uint32_t length,
                                                   wuffs_base__slice_u8 src) {
  uint8_t* iop_w = *ptr_iop_w;
  size_t n = src.len;
  if (n > length) {
    n = length;
  }
  if (n > ((size_t)(io2_w - iop_w))) {
    n = (size_t)(io2_w - iop_w);
  }
  if (n > 0) {
    memmove(iop_w, src.ptr, n);
    *ptr_iop_w += n;
  }
  return (uint32_t)(n);
}

static inline wuffs_base__io_buffer*  //
wuffs_base__io_writer__set(wuffs_base__io_buffer* b,
                           uint8_t** ptr_iop_w,
                           uint8_t** ptr_io0_w,
                           uint8_t** ptr_io1_w,
                           uint8_t** ptr_io2_w,
                           wuffs_base__slice_u8 data) {
  b->data = data;
  b->meta.wi = 0;
  b->meta.ri = 0;
  b->meta.pos = 0;
  b->meta.closed = false;

  *ptr_iop_w = data.ptr;
  *ptr_io0_w = data.ptr;
  *ptr_io1_w = data.ptr;
  *ptr_io2_w = data.ptr + data.len;

  return b;
}

// ---------------- I/O (Utility)

#define wuffs_base__utility__empty_io_reader wuffs_base__empty_io_reader
#define wuffs_base__utility__empty_io_writer wuffs_base__empty_io_writer

// ---------------- Tokens

// ---------------- Tokens (Utility)

// ---------------- Memory Allocation

// ---------------- Images

WUFFS_BASE__MAYBE_STATIC uint64_t  //
wuffs_base__pixel_swizzler__swizzle_interleaved_from_reader(
    const wuffs_base__pixel_swizzler* p,
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    const uint8_t** ptr_iop_r,
    const uint8_t* io2_r);

// ---------------- Images (Utility)

#define wuffs_base__utility__make_pixel_format wuffs_base__make_pixel_format

// ---------------- String Conversions

// ---------------- Unicode and UTF-8

// ----------------

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__BASE) || \
    defined(WUFFS_CONFIG__MODULE__BASE__CORE)

const uint8_t wuffs_base__low_bits_mask__u8[9] = {
    0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF,
};

const uint16_t wuffs_base__low_bits_mask__u16[17] = {
    0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF,
    0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
};

const uint32_t wuffs_base__low_bits_mask__u32[33] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000F, 0x0000001F,
    0x0000003F, 0x0000007F, 0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
    0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF, 0x0001FFFF,
    0x0003FFFF, 0x0007FFFF, 0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
    0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF, 0x1FFFFFFF,
    0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

const uint64_t wuffs_base__low_bits_mask__u64[65] = {
    0x0000000000000000, 0x0000000000000001, 0x0000000000000003,
    0x0000000000000007, 0x000000000000000F, 0x000000000000001F,
    0x000000000000003F, 0x000000000000007F, 0x00000000000000FF,
    0x00000000000001FF, 0x00000000000003FF, 0x00000000000007FF,
    0x0000000000000FFF, 0x0000000000001FFF, 0x0000000000003FFF,
    0x0000000000007FFF, 0x000000000000FFFF, 0x000000000001FFFF,
    0x000000000003FFFF, 0x000000000007FFFF, 0x00000000000FFFFF,
    0x00000000001FFFFF, 0x00000000003FFFFF, 0x00000000007FFFFF,
    0x0000000000FFFFFF, 0x0000000001FFFFFF, 0x0000000003FFFFFF,
    0x0000000007FFFFFF, 0x000000000FFFFFFF, 0x000000001FFFFFFF,
    0x000000003FFFFFFF, 0x000000007FFFFFFF, 0x00000000FFFFFFFF,
    0x00000001FFFFFFFF, 0x00000003FFFFFFFF, 0x00000007FFFFFFFF,
    0x0000000FFFFFFFFF, 0x0000001FFFFFFFFF, 0x0000003FFFFFFFFF,
    0x0000007FFFFFFFFF, 0x000000FFFFFFFFFF, 0x000001FFFFFFFFFF,
    0x000003FFFFFFFFFF, 0x000007FFFFFFFFFF, 0x00000FFFFFFFFFFF,
    0x00001FFFFFFFFFFF, 0x00003FFFFFFFFFFF, 0x00007FFFFFFFFFFF,
    0x0000FFFFFFFFFFFF, 0x0001FFFFFFFFFFFF, 0x0003FFFFFFFFFFFF,
    0x0007FFFFFFFFFFFF, 0x000FFFFFFFFFFFFF, 0x001FFFFFFFFFFFFF,
    0x003FFFFFFFFFFFFF, 0x007FFFFFFFFFFFFF, 0x00FFFFFFFFFFFFFF,
    0x01FFFFFFFFFFFFFF, 0x03FFFFFFFFFFFFFF, 0x07FFFFFFFFFFFFFF,
    0x0FFFFFFFFFFFFFFF, 0x1FFFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFFFF,
    0x7FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF,
};

const uint32_t wuffs_base__pixel_format__bits_per_channel[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x0A, 0x0C, 0x10, 0x18, 0x20, 0x30, 0x40,
};

const char* wuffs_base__note__i_o_redirect = "@base: I/O redirect";
const char* wuffs_base__note__end_of_data = "@base: end of data";
const char* wuffs_base__note__metadata_reported = "@base: metadata reported";
const char* wuffs_base__suspension__even_more_information = "$base: even more information";
const char* wuffs_base__suspension__mispositioned_read = "$base: mispositioned read";
const char* wuffs_base__suspension__mispositioned_write = "$base: mispositioned write";
const char* wuffs_base__suspension__short_read = "$base: short read";
const char* wuffs_base__suspension__short_write = "$base: short write";
const char* wuffs_base__error__bad_i_o_position = "#base: bad I/O position";
const char* wuffs_base__error__bad_argument_length_too_short = "#base: bad argument (length too short)";
const char* wuffs_base__error__bad_argument = "#base: bad argument";
const char* wuffs_base__error__bad_call_sequence = "#base: bad call sequence";
const char* wuffs_base__error__bad_receiver = "#base: bad receiver";
const char* wuffs_base__error__bad_restart = "#base: bad restart";
const char* wuffs_base__error__bad_sizeof_receiver = "#base: bad sizeof receiver";
const char* wuffs_base__error__bad_vtable = "#base: bad vtable";
const char* wuffs_base__error__bad_workbuf_length = "#base: bad workbuf length";
const char* wuffs_base__error__bad_wuffs_version = "#base: bad wuffs version";
const char* wuffs_base__error__cannot_return_a_suspension = "#base: cannot return a suspension";
const char* wuffs_base__error__disabled_by_previous_error = "#base: disabled by previous error";
const char* wuffs_base__error__initialize_falsely_claimed_already_zeroed = "#base: initialize falsely claimed already zeroed";
const char* wuffs_base__error__initialize_not_called = "#base: initialize not called";
const char* wuffs_base__error__interleaved_coroutine_calls = "#base: interleaved coroutine calls";
const char* wuffs_base__error__no_more_information = "#base: no more information";
const char* wuffs_base__error__not_enough_data = "#base: not enough data";
const char* wuffs_base__error__out_of_bounds = "#base: out of bounds";
const char* wuffs_base__error__unsupported_method = "#base: unsupported method";
const char* wuffs_base__error__unsupported_option = "#base: unsupported option";
const char* wuffs_base__error__unsupported_pixel_swizzler_option = "#base: unsupported pixel swizzler option";
const char* wuffs_base__error__too_much_data = "#base: too much data";

const char* wuffs_base__hasher_u32__vtable_name = "{vtable}wuffs_base__hasher_u32";
const char* wuffs_base__image_decoder__vtable_name = "{vtable}wuffs_base__image_decoder";
const char* wuffs_base__io_transformer__vtable_name = "{vtable}wuffs_base__io_transformer";
const char* wuffs_base__token_decoder__vtable_name = "{vtable}wuffs_base__token_decoder";

// ---------------- String Conversions

// wuffs_base__parse_number__foo_digits entries are 0x00 for invalid digits,
// and (0x80 | v) for valid digits, where v is the 4 bit value.

static const uint8_t wuffs_base__parse_number__decimal_digits[256] = {
    // 0     1     2     3     4     5     6     7
    // 8     9     A     B     C     D     E     F
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x00 ..= 0x07.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x08 ..= 0x0F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x10 ..= 0x17.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x18 ..= 0x1F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x20 ..= 0x27.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x28 ..= 0x2F.
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,  // 0x30 ..= 0x37. '0'-'7'.
    0x88, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x38 ..= 0x3F. '8'-'9'.

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x40 ..= 0x47.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x48 ..= 0x4F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x50 ..= 0x57.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x58 ..= 0x5F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x60 ..= 0x67.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x68 ..= 0x6F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x70 ..= 0x77.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x78 ..= 0x7F.

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x80 ..= 0x87.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x88 ..= 0x8F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x90 ..= 0x97.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x98 ..= 0x9F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xA0 ..= 0xA7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xA8 ..= 0xAF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xB0 ..= 0xB7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xB8 ..= 0xBF.

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xC0 ..= 0xC7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xC8 ..= 0xCF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xD0 ..= 0xD7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xD8 ..= 0xDF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xE0 ..= 0xE7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xE8 ..= 0xEF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xF0 ..= 0xF7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xF8 ..= 0xFF.
    // 0     1     2     3     4     5     6     7
    // 8     9     A     B     C     D     E     F
};

static const uint8_t wuffs_base__parse_number__hexadecimal_digits[256] = {
    // 0     1     2     3     4     5     6     7
    // 8     9     A     B     C     D     E     F
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x00 ..= 0x07.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x08 ..= 0x0F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x10 ..= 0x17.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x18 ..= 0x1F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x20 ..= 0x27.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x28 ..= 0x2F.
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,  // 0x30 ..= 0x37. '0'-'7'.
    0x88, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x38 ..= 0x3F. '8'-'9'.

    0x00, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x00,  // 0x40 ..= 0x47. 'A'-'F'.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x48 ..= 0x4F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x50 ..= 0x57.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x58 ..= 0x5F.
    0x00, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x00,  // 0x60 ..= 0x67. 'a'-'f'.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x68 ..= 0x6F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x70 ..= 0x77.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x78 ..= 0x7F.

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x80 ..= 0x87.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x88 ..= 0x8F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x90 ..= 0x97.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x98 ..= 0x9F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xA0 ..= 0xA7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xA8 ..= 0xAF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xB0 ..= 0xB7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xB8 ..= 0xBF.

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xC0 ..= 0xC7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xC8 ..= 0xCF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xD0 ..= 0xD7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xD8 ..= 0xDF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xE0 ..= 0xE7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xE8 ..= 0xEF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xF0 ..= 0xF7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xF8 ..= 0xFF.
    // 0     1     2     3     4     5     6     7
    // 8     9     A     B     C     D     E     F
};

// --------

WUFFS_BASE__MAYBE_STATIC wuffs_base__result_i64  //
wuffs_base__parse_number_i64(wuffs_base__slice_u8 s) {
  uint8_t* p = s.ptr;
  uint8_t* q = s.ptr + s.len;

  for (; (p < q) && (*p == '_'); p++) {
  }

  bool negative = false;
  if (p >= q) {
    goto fail_bad_argument;
  } else if (*p == '-') {
    p++;
    negative = true;
  } else if (*p == '+') {
    p++;
  }

  do {
    wuffs_base__result_u64 r = wuffs_base__parse_number_u64(
        wuffs_base__make_slice_u8(p, (size_t)(q - p)));
    if (r.status.repr != NULL) {
      wuffs_base__result_i64 ret;
      ret.status.repr = r.status.repr;
      ret.value = 0;
      return ret;
    } else if (negative) {
      if (r.value > 0x8000000000000000) {
        goto fail_out_of_bounds;
      }
      wuffs_base__result_i64 ret;
      ret.status.repr = NULL;
      ret.value = -(int64_t)(r.value);
      return ret;
    } else if (r.value > 0x7FFFFFFFFFFFFFFF) {
      goto fail_out_of_bounds;
    } else {
      wuffs_base__result_i64 ret;
      ret.status.repr = NULL;
      ret.value = +(int64_t)(r.value);
      return ret;
    }
  } while (0);

fail_bad_argument:
  do {
    wuffs_base__result_i64 ret;
    ret.status.repr = wuffs_base__error__bad_argument;
    ret.value = 0;
    return ret;
  } while (0);

fail_out_of_bounds:
  do {
    wuffs_base__result_i64 ret;
    ret.status.repr = wuffs_base__error__out_of_bounds;
    ret.value = 0;
    return ret;
  } while (0);
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__result_u64  //
wuffs_base__parse_number_u64(wuffs_base__slice_u8 s) {
  uint8_t* p = s.ptr;
  uint8_t* q = s.ptr + s.len;

  for (; (p < q) && (*p == '_'); p++) {
  }

  if (p >= q) {
    goto fail_bad_argument;

  } else if (*p == '0') {
    p++;
    if (p >= q) {
      goto ok_zero;
    }
    if (*p == '_') {
      p++;
      for (; p < q; p++) {
        if (*p != '_') {
          goto fail_bad_argument;
        }
      }
      goto ok_zero;
    }

    if ((*p == 'x') || (*p == 'X')) {
      p++;
      for (; (p < q) && (*p == '_'); p++) {
      }
      if (p < q) {
        goto hexadecimal;
      }

    } else if ((*p == 'd') || (*p == 'D')) {
      p++;
      for (; (p < q) && (*p == '_'); p++) {
      }
      if (p < q) {
        goto decimal;
      }
    }

    goto fail_bad_argument;
  }

decimal:
  do {
    uint64_t v = wuffs_base__parse_number__decimal_digits[*p++];
    if (v == 0) {
      goto fail_bad_argument;
    }
    v &= 0x0F;

    // UINT64_MAX is 18446744073709551615, which is ((10 * max10) + max1).
    const uint64_t max10 = 1844674407370955161u;
    const uint8_t max1 = 5;

    for (; p < q; p++) {
      if (*p == '_') {
        continue;
      }
      uint8_t digit = wuffs_base__parse_number__decimal_digits[*p];
      if (digit == 0) {
        goto fail_bad_argument;
      }
      digit &= 0x0F;
      if ((v > max10) || ((v == max10) && (digit > max1))) {
        goto fail_out_of_bounds;
      }
      v = (10 * v) + ((uint64_t)(digit));
    }

    wuffs_base__result_u64 ret;
    ret.status.repr = NULL;
    ret.value = v;
    return ret;
  } while (0);

hexadecimal:
  do {
    uint64_t v = wuffs_base__parse_number__hexadecimal_digits[*p++];
    if (v == 0) {
      goto fail_bad_argument;
    }
    v &= 0x0F;

    for (; p < q; p++) {
      if (*p == '_') {
        continue;
      }
      uint8_t digit = wuffs_base__parse_number__hexadecimal_digits[*p];
      if (digit == 0) {
        goto fail_bad_argument;
      }
      digit &= 0x0F;
      if ((v >> 60) != 0) {
        goto fail_out_of_bounds;
      }
      v = (v << 4) | ((uint64_t)(digit));
    }

    wuffs_base__result_u64 ret;
    ret.status.repr = NULL;
    ret.value = v;
    return ret;
  } while (0);

ok_zero:
  do {
    wuffs_base__result_u64 ret;
    ret.status.repr = NULL;
    ret.value = 0;
    return ret;
  } while (0);

fail_bad_argument:
  do {
    wuffs_base__result_u64 ret;
    ret.status.repr = wuffs_base__error__bad_argument;
    ret.value = 0;
    return ret;
  } while (0);

fail_out_of_bounds:
  do {
    wuffs_base__result_u64 ret;
    ret.status.repr = wuffs_base__error__out_of_bounds;
    ret.value = 0;
    return ret;
  } while (0);
}

// ---------------- Hexadecimal

WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__hexadecimal__decode2(wuffs_base__slice_u8 dst,
                                 wuffs_base__slice_u8 src) {
  size_t src_len2 = src.len / 2;
  size_t len = dst.len < src_len2 ? dst.len : src_len2;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  while (n--) {
    *d = (uint8_t)((wuffs_base__parse_number__hexadecimal_digits[s[0]] << 4) |
                   (wuffs_base__parse_number__hexadecimal_digits[s[1]] & 0x0F));
    d += 1;
    s += 2;
  }

  return len;
}

WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__hexadecimal__decode4(wuffs_base__slice_u8 dst,
                                 wuffs_base__slice_u8 src) {
  size_t src_len4 = src.len / 4;
  size_t len = dst.len < src_len4 ? dst.len : src_len4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  while (n--) {
    *d = (uint8_t)((wuffs_base__parse_number__hexadecimal_digits[s[2]] << 4) |
                   (wuffs_base__parse_number__hexadecimal_digits[s[3]] & 0x0F));
    d += 1;
    s += 4;
  }

  return len;
}

// ---------------- Unicode and UTF-8

WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__utf_8__encode(wuffs_base__slice_u8 dst, uint32_t code_point) {
  if (code_point <= 0x7F) {
    if (dst.len >= 1) {
      dst.ptr[0] = (uint8_t)(code_point);
      return 1;
    }

  } else if (code_point <= 0x07FF) {
    if (dst.len >= 2) {
      dst.ptr[0] = (uint8_t)(0xC0 | ((code_point >> 6)));
      dst.ptr[1] = (uint8_t)(0x80 | ((code_point >> 0) & 0x3F));
      return 2;
    }

  } else if (code_point <= 0xFFFF) {
    if ((dst.len >= 3) && ((code_point < 0xD800) || (0xDFFF < code_point))) {
      dst.ptr[0] = (uint8_t)(0xE0 | ((code_point >> 12)));
      dst.ptr[1] = (uint8_t)(0x80 | ((code_point >> 6) & 0x3F));
      dst.ptr[2] = (uint8_t)(0x80 | ((code_point >> 0) & 0x3F));
      return 3;
    }

  } else if (code_point <= 0x10FFFF) {
    if (dst.len >= 4) {
      dst.ptr[0] = (uint8_t)(0xF0 | ((code_point >> 18)));
      dst.ptr[1] = (uint8_t)(0x80 | ((code_point >> 12) & 0x3F));
      dst.ptr[2] = (uint8_t)(0x80 | ((code_point >> 6) & 0x3F));
      dst.ptr[3] = (uint8_t)(0x80 | ((code_point >> 0) & 0x3F));
      return 4;
    }
  }

  return 0;
}

// wuffs_base__utf_8__byte_length_minus_1 is the byte length (minus 1) of a
// UTF-8 encoded code point, based on the encoding's initial byte.
//  - 0x00 is 1-byte UTF-8 (ASCII).
//  - 0x01 is the start of 2-byte UTF-8.
//  - 0x02 is the start of 3-byte UTF-8.
//  - 0x03 is the start of 4-byte UTF-8.
//  - 0x40 is a UTF-8 tail byte.
//  - 0x80 is invalid UTF-8.
//
// RFC 3629 (UTF-8) gives this grammar for valid UTF-8:
//    UTF8-1      = %x00-7F
//    UTF8-2      = %xC2-DF UTF8-tail
//    UTF8-3      = %xE0 %xA0-BF UTF8-tail / %xE1-EC 2( UTF8-tail ) /
//                  %xED %x80-9F UTF8-tail / %xEE-EF 2( UTF8-tail )
//    UTF8-4      = %xF0 %x90-BF 2( UTF8-tail ) / %xF1-F3 3( UTF8-tail ) /
//                  %xF4 %x80-8F 2( UTF8-tail )
//    UTF8-tail   = %x80-BF
static const uint8_t wuffs_base__utf_8__byte_length_minus_1[256] = {
    // 0     1     2     3     4     5     6     7
    // 8     9     A     B     C     D     E     F
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x00 ..= 0x07.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x08 ..= 0x0F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x10 ..= 0x17.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x18 ..= 0x1F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x20 ..= 0x27.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x28 ..= 0x2F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x30 ..= 0x37.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x38 ..= 0x3F.

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x40 ..= 0x47.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x48 ..= 0x4F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x50 ..= 0x57.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x58 ..= 0x5F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x60 ..= 0x67.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x68 ..= 0x6F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x70 ..= 0x77.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x78 ..= 0x7F.

    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0x80 ..= 0x87.
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0x88 ..= 0x8F.
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0x90 ..= 0x97.
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0x98 ..= 0x9F.
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0xA0 ..= 0xA7.
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0xA8 ..= 0xAF.
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0xB0 ..= 0xB7.
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0xB8 ..= 0xBF.

    0x80, 0x80, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,  // 0xC0 ..= 0xC7.
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,  // 0xC8 ..= 0xCF.
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,  // 0xD0 ..= 0xD7.
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,  // 0xD8 ..= 0xDF.
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,  // 0xE0 ..= 0xE7.
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,  // 0xE8 ..= 0xEF.
    0x03, 0x03, 0x03, 0x03, 0x03, 0x80, 0x80, 0x80,  // 0xF0 ..= 0xF7.
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,  // 0xF8 ..= 0xFF.
    // 0     1     2     3     4     5     6     7
    // 8     9     A     B     C     D     E     F
};

WUFFS_BASE__MAYBE_STATIC wuffs_base__utf_8__next__output  //
wuffs_base__utf_8__next(wuffs_base__slice_u8 s) {
  if (s.len == 0) {
    return wuffs_base__make_utf_8__next__output(0, 0);
  }
  uint32_t c = s.ptr[0];
  switch (wuffs_base__utf_8__byte_length_minus_1[c & 0xFF]) {
    case 0:
      return wuffs_base__make_utf_8__next__output(c, 1);

    case 1:
      if (s.len < 2) {
        break;
      }
      c = wuffs_base__load_u16le__no_bounds_check(s.ptr);
      if ((c & 0xC000) != 0x8000) {
        break;
      }
      c = (0x0007C0 & (c << 6)) | (0x00003F & (c >> 8));
      return wuffs_base__make_utf_8__next__output(c, 2);

    case 2:
      if (s.len < 3) {
        break;
      }
      c = wuffs_base__load_u24le__no_bounds_check(s.ptr);
      if ((c & 0xC0C000) != 0x808000) {
        break;
      }
      c = (0x00F000 & (c << 12)) | (0x000FC0 & (c >> 2)) |
          (0x00003F & (c >> 16));
      if ((c <= 0x07FF) || ((0xD800 <= c) && (c <= 0xDFFF))) {
        break;
      }
      return wuffs_base__make_utf_8__next__output(c, 3);

    case 3:
      if (s.len < 4) {
        break;
      }
      c = wuffs_base__load_u32le__no_bounds_check(s.ptr);
      if ((c & 0xC0C0C000) != 0x80808000) {
        break;
      }
      c = (0x1C0000 & (c << 18)) | (0x03F000 & (c << 4)) |
          (0x000FC0 & (c >> 10)) | (0x00003F & (c >> 24));
      if ((c <= 0xFFFF) || (0x110000 <= c)) {
        break;
      }
      return wuffs_base__make_utf_8__next__output(c, 4);
  }

  return wuffs_base__make_utf_8__next__output(
      WUFFS_BASE__UNICODE_REPLACEMENT_CHARACTER, 1);
}

WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__utf_8__longest_valid_prefix(wuffs_base__slice_u8 s) {
  // TODO: possibly optimize the all-ASCII case (4 or 8 bytes at a time).
  //
  // TODO: possibly optimize this by manually inlining the
  // wuffs_base__utf_8__next calls.
  size_t original_len = s.len;
  while (s.len > 0) {
    wuffs_base__utf_8__next__output o = wuffs_base__utf_8__next(s);
    if ((o.code_point > 0x7F) && (o.byte_length == 1)) {
      break;
    }
    s.ptr += o.byte_length;
    s.len -= o.byte_length;
  }
  return original_len - s.len;
}

WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__ascii__longest_valid_prefix(wuffs_base__slice_u8 s) {
  // TODO: possibly optimize this by checking 4 or 8 bytes at a time.
  uint8_t* original_ptr = s.ptr;
  uint8_t* p = s.ptr;
  uint8_t* q = s.ptr + s.len;
  for (; (p != q) && ((*p & 0x80) == 0); p++) {
  }
  return (size_t)(p - original_ptr);
}

#endif  // !defined(WUFFS_CONFIG__MODULES) ||
        // defined(WUFFS_CONFIG__MODULE__BASE)  ||
        // defined(WUFFS_CONFIG__MODULE__BASE__CORE)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__BASE) || \
    defined(WUFFS_CONFIG__MODULE__BASE__INTERFACES)

// ---------------- Interface Definitions.

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_base__hasher_u32__set_quirk_enabled(
    wuffs_base__hasher_u32* self,
    uint32_t a_quirk,
    bool a_enabled) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__hasher_u32__vtable_name) {
      const wuffs_base__hasher_u32__func_ptrs* func_ptrs =
          (const wuffs_base__hasher_u32__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->set_quirk_enabled)(self, a_quirk, a_enabled);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_empty_struct();
}

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_base__hasher_u32__update_u32(
    wuffs_base__hasher_u32* self,
    wuffs_base__slice_u8 a_x) {
  if (!self) {
    return 0;
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return 0;
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__hasher_u32__vtable_name) {
      const wuffs_base__hasher_u32__func_ptrs* func_ptrs =
          (const wuffs_base__hasher_u32__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->update_u32)(self, a_x);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return 0;
}

// --------

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__image_decoder__decode_frame(
    wuffs_base__image_decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend,
    wuffs_base__slice_u8 a_workbuf,
    wuffs_base__decode_frame_options* a_opts) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
            ? wuffs_base__error__disabled_by_previous_error
            : wuffs_base__error__initialize_not_called);
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->decode_frame)(self, a_dst, a_src, a_blend, a_workbuf, a_opts);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_status(wuffs_base__error__bad_vtable);
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__image_decoder__decode_frame_config(
    wuffs_base__image_decoder* self,
    wuffs_base__frame_config* a_dst,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
            ? wuffs_base__error__disabled_by_previous_error
            : wuffs_base__error__initialize_not_called);
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->decode_frame_config)(self, a_dst, a_src);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_status(wuffs_base__error__bad_vtable);
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__image_decoder__decode_image_config(
    wuffs_base__image_decoder* self,
    wuffs_base__image_config* a_dst,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
            ? wuffs_base__error__disabled_by_previous_error
            : wuffs_base__error__initialize_not_called);
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->decode_image_config)(self, a_dst, a_src);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_status(wuffs_base__error__bad_vtable);
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__rect_ie_u32
wuffs_base__image_decoder__frame_dirty_rect(
    const wuffs_base__image_decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_rect_ie_u32();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_rect_ie_u32();
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->frame_dirty_rect)(self);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__utility__empty_rect_ie_u32();
}

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_base__image_decoder__num_animation_loops(
    const wuffs_base__image_decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->num_animation_loops)(self);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return 0;
}

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_base__image_decoder__num_decoded_frame_configs(
    const wuffs_base__image_decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->num_decoded_frame_configs)(self);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return 0;
}

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_base__image_decoder__num_decoded_frames(
    const wuffs_base__image_decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->num_decoded_frames)(self);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return 0;
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__image_decoder__restart_frame(
    wuffs_base__image_decoder* self,
    uint64_t a_index,
    uint64_t a_io_position) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
            ? wuffs_base__error__disabled_by_previous_error
            : wuffs_base__error__initialize_not_called);
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->restart_frame)(self, a_index, a_io_position);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_status(wuffs_base__error__bad_vtable);
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_base__image_decoder__set_quirk_enabled(
    wuffs_base__image_decoder* self,
    uint32_t a_quirk,
    bool a_enabled) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->set_quirk_enabled)(self, a_quirk, a_enabled);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_empty_struct();
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_base__image_decoder__set_report_metadata(
    wuffs_base__image_decoder* self,
    uint32_t a_fourcc,
    bool a_report) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->set_report_metadata)(self, a_fourcc, a_report);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_empty_struct();
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__image_decoder__tell_me_more(
    wuffs_base__image_decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__more_information* a_minfo,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
            ? wuffs_base__error__disabled_by_previous_error
            : wuffs_base__error__initialize_not_called);
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->tell_me_more)(self, a_dst, a_minfo, a_src);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_status(wuffs_base__error__bad_vtable);
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_base__image_decoder__workbuf_len(
    const wuffs_base__image_decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__image_decoder__vtable_name) {
      const wuffs_base__image_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__image_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->workbuf_len)(self);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__utility__empty_range_ii_u64();
}

// --------

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_base__io_transformer__set_quirk_enabled(
    wuffs_base__io_transformer* self,
    uint32_t a_quirk,
    bool a_enabled) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__io_transformer__vtable_name) {
      const wuffs_base__io_transformer__func_ptrs* func_ptrs =
          (const wuffs_base__io_transformer__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->set_quirk_enabled)(self, a_quirk, a_enabled);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_empty_struct();
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__io_transformer__transform_io(
    wuffs_base__io_transformer* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
            ? wuffs_base__error__disabled_by_previous_error
            : wuffs_base__error__initialize_not_called);
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__io_transformer__vtable_name) {
      const wuffs_base__io_transformer__func_ptrs* func_ptrs =
          (const wuffs_base__io_transformer__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->transform_io)(self, a_dst, a_src, a_workbuf);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_status(wuffs_base__error__bad_vtable);
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_base__io_transformer__workbuf_len(
    const wuffs_base__io_transformer* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__io_transformer__vtable_name) {
      const wuffs_base__io_transformer__func_ptrs* func_ptrs =
          (const wuffs_base__io_transformer__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->workbuf_len)(self);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__utility__empty_range_ii_u64();
}

// --------

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_base__token_decoder__decode_tokens(
    wuffs_base__token_decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
            ? wuffs_base__error__disabled_by_previous_error
            : wuffs_base__error__initialize_not_called);
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__token_decoder__vtable_name) {
      const wuffs_base__token_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__token_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->decode_tokens)(self, a_dst, a_src, a_workbuf);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_status(wuffs_base__error__bad_vtable);
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_base__token_decoder__set_quirk_enabled(
    wuffs_base__token_decoder* self,
    uint32_t a_quirk,
    bool a_enabled) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__token_decoder__vtable_name) {
      const wuffs_base__token_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__token_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->set_quirk_enabled)(self, a_quirk, a_enabled);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__make_empty_struct();
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_base__token_decoder__workbuf_len(
    const wuffs_base__token_decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  const wuffs_base__vtable* v = &self->private_impl.first_vtable;
  int i;
  for (i = 0; i < 63; i++) {
    if (v->vtable_name == wuffs_base__token_decoder__vtable_name) {
      const wuffs_base__token_decoder__func_ptrs* func_ptrs =
          (const wuffs_base__token_decoder__func_ptrs*)(v->function_pointers);
      return (*func_ptrs->workbuf_len)(self);
    } else if (v->vtable_name == NULL) {
      break;
    }
    v++;
  }

  return wuffs_base__utility__empty_range_ii_u64();
}

#endif  // !defined(WUFFS_CONFIG__MODULES) ||
        // defined(WUFFS_CONFIG__MODULE__BASE) ||
        // defined(WUFFS_CONFIG__MODULE__BASE__INTERFACES)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__BASE) || \
    defined(WUFFS_CONFIG__MODULE__BASE__F64CONV)

// ---------------- IEEE 754 Floating Point

#define WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE 1023
#define WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION 500

// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL is the largest N
// such that ((10 << N) < (1 << 64)).
#define WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL 60

// wuffs_base__private_implementation__high_prec_dec (abbreviated as HPD) is a
// fixed precision floating point decimal number, augmented with ±infinity
// values, but it cannot represent NaN (Not a Number).
//
// "High precision" means that the mantissa holds 500 decimal digits. 500 is
// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION.
//
// An HPD isn't for general purpose arithmetic, only for conversions to and
// from IEEE 754 double-precision floating point, where the largest and
// smallest positive, finite values are approximately 1.8e+308 and 4.9e-324.
// HPD exponents above +1023 mean infinity, below -1023 mean zero. The ±1023
// bounds are further away from zero than ±(324 + 500), where 500 and 1023 is
// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION and
// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE.
//
// digits[.. num_digits] are the number's digits in big-endian order. The
// uint8_t values are in the range [0 ..= 9], not ['0' ..= '9'], where e.g. '7'
// is the ASCII value 0x37.
//
// decimal_point is the index (within digits) of the decimal point. It may be
// negative or be larger than num_digits, in which case the explicit digits are
// padded with implicit zeroes.
//
// For example, if num_digits is 3 and digits is "\x07\x08\x09":
//   - A decimal_point of -2 means ".00789"
//   - A decimal_point of -1 means ".0789"
//   - A decimal_point of +0 means ".789"
//   - A decimal_point of +1 means "7.89"
//   - A decimal_point of +2 means "78.9"
//   - A decimal_point of +3 means "789."
//   - A decimal_point of +4 means "7890."
//   - A decimal_point of +5 means "78900."
//
// As above, a decimal_point higher than +1023 means that the overall value is
// infinity, lower than -1023 means zero.
//
// negative is a sign bit. An HPD can distinguish positive and negative zero.
//
// truncated is whether there are more than
// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION digits, and at
// least one of those extra digits are non-zero. The existence of long-tail
// digits can affect rounding.
//
// The "all fields are zero" value is valid, and represents the number +0.
typedef struct {
  uint32_t num_digits;
  int32_t decimal_point;
  bool negative;
  bool truncated;
  uint8_t digits[WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION];
} wuffs_base__private_implementation__high_prec_dec;

// wuffs_base__private_implementation__high_prec_dec__trim trims trailing
// zeroes from the h->digits[.. h->num_digits] slice. They have no benefit,
// since we explicitly track h->decimal_point.
//
// Preconditions:
//  - h is non-NULL.
static inline void  //
wuffs_base__private_implementation__high_prec_dec__trim(
    wuffs_base__private_implementation__high_prec_dec* h) {
  while ((h->num_digits > 0) && (h->digits[h->num_digits - 1] == 0)) {
    h->num_digits--;
  }
}

static wuffs_base__status  //
wuffs_base__private_implementation__high_prec_dec__parse(
    wuffs_base__private_implementation__high_prec_dec* h,
    wuffs_base__slice_u8 s) {
  if (!h) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  h->num_digits = 0;
  h->decimal_point = 0;
  h->negative = false;
  h->truncated = false;

  uint8_t* p = s.ptr;
  uint8_t* q = s.ptr + s.len;

  for (; (p < q) && (*p == '_'); p++) {
  }
  if (p >= q) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }

  // Parse sign.
  do {
    if (*p == '+') {
      p++;
    } else if (*p == '-') {
      h->negative = true;
      p++;
    } else {
      break;
    }
    for (; (p < q) && (*p == '_'); p++) {
    }
  } while (0);

  // Parse digits.
  uint32_t nd = 0;
  int32_t dp = 0;
  bool saw_digits = false;
  bool saw_non_zero_digits = false;
  bool saw_dot = false;
  for (; p < q; p++) {
    if (*p == '_') {
      // No-op.

    } else if ((*p == '.') || (*p == ',')) {
      // As per https://en.wikipedia.org/wiki/Decimal_separator, both '.' or
      // ',' are commonly used. We just parse either, regardless of LOCALE.
      if (saw_dot) {
        return wuffs_base__make_status(wuffs_base__error__bad_argument);
      }
      saw_dot = true;
      dp = (int32_t)nd;

    } else if ('0' == *p) {
      if (!saw_dot && !saw_non_zero_digits && saw_digits) {
        // We don't allow unnecessary leading zeroes: "000123" or "0644".
        return wuffs_base__make_status(wuffs_base__error__bad_argument);
      }
      saw_digits = true;
      if (nd == 0) {
        // Track leading zeroes implicitly.
        dp--;
      } else if (nd <
                 WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
        h->digits[nd++] = 0;
      } else {
        // Long-tail zeroes are ignored.
      }

    } else if (('0' < *p) && (*p <= '9')) {
      if (!saw_dot && !saw_non_zero_digits && saw_digits) {
        // We don't allow unnecessary leading zeroes: "000123" or "0644".
        return wuffs_base__make_status(wuffs_base__error__bad_argument);
      }
      saw_digits = true;
      saw_non_zero_digits = true;
      if (nd < WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
        h->digits[nd++] = (uint8_t)(*p - '0');
      } else {
        // Long-tail non-zeroes set the truncated bit.
        h->truncated = true;
      }

    } else {
      break;
    }
  }

  if (!saw_digits) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if (!saw_dot) {
    dp = (int32_t)nd;
  }

  // Parse exponent.
  if ((p < q) && ((*p == 'E') || (*p == 'e'))) {
    p++;
    for (; (p < q) && (*p == '_'); p++) {
    }
    if (p >= q) {
      return wuffs_base__make_status(wuffs_base__error__bad_argument);
    }

    int32_t exp_sign = +1;
    if (*p == '+') {
      p++;
    } else if (*p == '-') {
      exp_sign = -1;
      p++;
    }

    int32_t exp = 0;
    const int32_t exp_large =
        WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE +
        WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION;
    bool saw_exp_digits = false;
    for (; p < q; p++) {
      if (*p == '_') {
        // No-op.
      } else if (('0' <= *p) && (*p <= '9')) {
        saw_exp_digits = true;
        if (exp < exp_large) {
          exp = (10 * exp) + ((int32_t)(*p - '0'));
        }
      } else {
        break;
      }
    }
    if (!saw_exp_digits) {
      return wuffs_base__make_status(wuffs_base__error__bad_argument);
    }
    dp += exp_sign * exp;
  }

  // Finish.
  if (p != q) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  h->num_digits = nd;
  if (nd == 0) {
    h->decimal_point = 0;
  } else if (dp <
             -WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
    h->decimal_point =
        -WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE - 1;
  } else if (dp >
             +WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
    h->decimal_point =
        +WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE + 1;
  } else {
    h->decimal_point = dp;
  }
  wuffs_base__private_implementation__high_prec_dec__trim(h);
  return wuffs_base__make_status(NULL);
}

// --------

// The etc__hpd_left_shift and etc__powers_of_5 tables were printed by
// script/print-hpd-left-shift.go. That script has an optional -comments flag,
// whose output is not copied here, which prints further detail.
//
// These tables are used in
// wuffs_base__private_implementation__high_prec_dec__lshift_num_new_digits.

// wuffs_base__private_implementation__hpd_left_shift[i] encodes the number of
// new digits created after multiplying a positive integer by (1 << i): the
// additional length in the decimal representation. For example, shifting "234"
// by 3 (equivalent to multiplying by 8) will produce "1872". Going from a
// 3-length string to a 4-length string means that 1 new digit was added (and
// existing digits may have changed).
//
// Shifting by i can add either N or N-1 new digits, depending on whether the
// original positive integer compares >= or < to the i'th power of 5 (as 10
// equals 2 * 5). Comparison is lexicographic, not numerical.
//
// For example, shifting by 4 (i.e. multiplying by 16) can add 1 or 2 new
// digits, depending on a lexicographic comparison to (5 ** 4), i.e. "625":
//  - ("1"      << 4) is "16",       which adds 1 new digit.
//  - ("5678"   << 4) is "90848",    which adds 1 new digit.
//  - ("624"    << 4) is "9984",     which adds 1 new digit.
//  - ("62498"  << 4) is "999968",   which adds 1 new digit.
//  - ("625"    << 4) is "10000",    which adds 2 new digits.
//  - ("625001" << 4) is "10000016", which adds 2 new digits.
//  - ("7008"   << 4) is "112128",   which adds 2 new digits.
//  - ("99"     << 4) is "1584",     which adds 2 new digits.
//
// Thus, when i is 4, N is 2 and (5 ** i) is "625". This etc__hpd_left_shift
// array encodes this as:
//  - etc__hpd_left_shift[4] is 0x1006 = (2 << 11) | 0x0006.
//  - etc__hpd_left_shift[5] is 0x1009 = (? << 11) | 0x0009.
// where the ? isn't relevant for i == 4.
//
// The high 5 bits of etc__hpd_left_shift[i] is N, the higher of the two
// possible number of new digits. The low 11 bits are an offset into the
// etc__powers_of_5 array (of length 0x051C, so offsets fit in 11 bits). When i
// is 4, its offset and the next one is 6 and 9, and etc__powers_of_5[6 .. 9]
// is the string "\x06\x02\x05", so the relevant power of 5 is "625".
//
// Thanks to Ken Thompson for the original idea.
static const uint16_t wuffs_base__private_implementation__hpd_left_shift[65] = {
    0x0000, 0x0800, 0x0801, 0x0803, 0x1006, 0x1009, 0x100D, 0x1812, 0x1817,
    0x181D, 0x2024, 0x202B, 0x2033, 0x203C, 0x2846, 0x2850, 0x285B, 0x3067,
    0x3073, 0x3080, 0x388E, 0x389C, 0x38AB, 0x38BB, 0x40CC, 0x40DD, 0x40EF,
    0x4902, 0x4915, 0x4929, 0x513E, 0x5153, 0x5169, 0x5180, 0x5998, 0x59B0,
    0x59C9, 0x61E3, 0x61FD, 0x6218, 0x6A34, 0x6A50, 0x6A6D, 0x6A8B, 0x72AA,
    0x72C9, 0x72E9, 0x7B0A, 0x7B2B, 0x7B4D, 0x8370, 0x8393, 0x83B7, 0x83DC,
    0x8C02, 0x8C28, 0x8C4F, 0x9477, 0x949F, 0x94C8, 0x9CF2, 0x051C, 0x051C,
    0x051C, 0x051C,
};

// wuffs_base__private_implementation__powers_of_5 contains the powers of 5,
// concatenated together: "5", "25", "125", "625", "3125", etc.
static const uint8_t wuffs_base__private_implementation__powers_of_5[0x051C] = {
    5, 2, 5, 1, 2, 5, 6, 2, 5, 3, 1, 2, 5, 1, 5, 6, 2, 5, 7, 8, 1, 2, 5, 3, 9,
    0, 6, 2, 5, 1, 9, 5, 3, 1, 2, 5, 9, 7, 6, 5, 6, 2, 5, 4, 8, 8, 2, 8, 1, 2,
    5, 2, 4, 4, 1, 4, 0, 6, 2, 5, 1, 2, 2, 0, 7, 0, 3, 1, 2, 5, 6, 1, 0, 3, 5,
    1, 5, 6, 2, 5, 3, 0, 5, 1, 7, 5, 7, 8, 1, 2, 5, 1, 5, 2, 5, 8, 7, 8, 9, 0,
    6, 2, 5, 7, 6, 2, 9, 3, 9, 4, 5, 3, 1, 2, 5, 3, 8, 1, 4, 6, 9, 7, 2, 6, 5,
    6, 2, 5, 1, 9, 0, 7, 3, 4, 8, 6, 3, 2, 8, 1, 2, 5, 9, 5, 3, 6, 7, 4, 3, 1,
    6, 4, 0, 6, 2, 5, 4, 7, 6, 8, 3, 7, 1, 5, 8, 2, 0, 3, 1, 2, 5, 2, 3, 8, 4,
    1, 8, 5, 7, 9, 1, 0, 1, 5, 6, 2, 5, 1, 1, 9, 2, 0, 9, 2, 8, 9, 5, 5, 0, 7,
    8, 1, 2, 5, 5, 9, 6, 0, 4, 6, 4, 4, 7, 7, 5, 3, 9, 0, 6, 2, 5, 2, 9, 8, 0,
    2, 3, 2, 2, 3, 8, 7, 6, 9, 5, 3, 1, 2, 5, 1, 4, 9, 0, 1, 1, 6, 1, 1, 9, 3,
    8, 4, 7, 6, 5, 6, 2, 5, 7, 4, 5, 0, 5, 8, 0, 5, 9, 6, 9, 2, 3, 8, 2, 8, 1,
    2, 5, 3, 7, 2, 5, 2, 9, 0, 2, 9, 8, 4, 6, 1, 9, 1, 4, 0, 6, 2, 5, 1, 8, 6,
    2, 6, 4, 5, 1, 4, 9, 2, 3, 0, 9, 5, 7, 0, 3, 1, 2, 5, 9, 3, 1, 3, 2, 2, 5,
    7, 4, 6, 1, 5, 4, 7, 8, 5, 1, 5, 6, 2, 5, 4, 6, 5, 6, 6, 1, 2, 8, 7, 3, 0,
    7, 7, 3, 9, 2, 5, 7, 8, 1, 2, 5, 2, 3, 2, 8, 3, 0, 6, 4, 3, 6, 5, 3, 8, 6,
    9, 6, 2, 8, 9, 0, 6, 2, 5, 1, 1, 6, 4, 1, 5, 3, 2, 1, 8, 2, 6, 9, 3, 4, 8,
    1, 4, 4, 5, 3, 1, 2, 5, 5, 8, 2, 0, 7, 6, 6, 0, 9, 1, 3, 4, 6, 7, 4, 0, 7,
    2, 2, 6, 5, 6, 2, 5, 2, 9, 1, 0, 3, 8, 3, 0, 4, 5, 6, 7, 3, 3, 7, 0, 3, 6,
    1, 3, 2, 8, 1, 2, 5, 1, 4, 5, 5, 1, 9, 1, 5, 2, 2, 8, 3, 6, 6, 8, 5, 1, 8,
    0, 6, 6, 4, 0, 6, 2, 5, 7, 2, 7, 5, 9, 5, 7, 6, 1, 4, 1, 8, 3, 4, 2, 5, 9,
    0, 3, 3, 2, 0, 3, 1, 2, 5, 3, 6, 3, 7, 9, 7, 8, 8, 0, 7, 0, 9, 1, 7, 1, 2,
    9, 5, 1, 6, 6, 0, 1, 5, 6, 2, 5, 1, 8, 1, 8, 9, 8, 9, 4, 0, 3, 5, 4, 5, 8,
    5, 6, 4, 7, 5, 8, 3, 0, 0, 7, 8, 1, 2, 5, 9, 0, 9, 4, 9, 4, 7, 0, 1, 7, 7,
    2, 9, 2, 8, 2, 3, 7, 9, 1, 5, 0, 3, 9, 0, 6, 2, 5, 4, 5, 4, 7, 4, 7, 3, 5,
    0, 8, 8, 6, 4, 6, 4, 1, 1, 8, 9, 5, 7, 5, 1, 9, 5, 3, 1, 2, 5, 2, 2, 7, 3,
    7, 3, 6, 7, 5, 4, 4, 3, 2, 3, 2, 0, 5, 9, 4, 7, 8, 7, 5, 9, 7, 6, 5, 6, 2,
    5, 1, 1, 3, 6, 8, 6, 8, 3, 7, 7, 2, 1, 6, 1, 6, 0, 2, 9, 7, 3, 9, 3, 7, 9,
    8, 8, 2, 8, 1, 2, 5, 5, 6, 8, 4, 3, 4, 1, 8, 8, 6, 0, 8, 0, 8, 0, 1, 4, 8,
    6, 9, 6, 8, 9, 9, 4, 1, 4, 0, 6, 2, 5, 2, 8, 4, 2, 1, 7, 0, 9, 4, 3, 0, 4,
    0, 4, 0, 0, 7, 4, 3, 4, 8, 4, 4, 9, 7, 0, 7, 0, 3, 1, 2, 5, 1, 4, 2, 1, 0,
    8, 5, 4, 7, 1, 5, 2, 0, 2, 0, 0, 3, 7, 1, 7, 4, 2, 2, 4, 8, 5, 3, 5, 1, 5,
    6, 2, 5, 7, 1, 0, 5, 4, 2, 7, 3, 5, 7, 6, 0, 1, 0, 0, 1, 8, 5, 8, 7, 1, 1,
    2, 4, 2, 6, 7, 5, 7, 8, 1, 2, 5, 3, 5, 5, 2, 7, 1, 3, 6, 7, 8, 8, 0, 0, 5,
    0, 0, 9, 2, 9, 3, 5, 5, 6, 2, 1, 3, 3, 7, 8, 9, 0, 6, 2, 5, 1, 7, 7, 6, 3,
    5, 6, 8, 3, 9, 4, 0, 0, 2, 5, 0, 4, 6, 4, 6, 7, 7, 8, 1, 0, 6, 6, 8, 9, 4,
    5, 3, 1, 2, 5, 8, 8, 8, 1, 7, 8, 4, 1, 9, 7, 0, 0, 1, 2, 5, 2, 3, 2, 3, 3,
    8, 9, 0, 5, 3, 3, 4, 4, 7, 2, 6, 5, 6, 2, 5, 4, 4, 4, 0, 8, 9, 2, 0, 9, 8,
    5, 0, 0, 6, 2, 6, 1, 6, 1, 6, 9, 4, 5, 2, 6, 6, 7, 2, 3, 6, 3, 2, 8, 1, 2,
    5, 2, 2, 2, 0, 4, 4, 6, 0, 4, 9, 2, 5, 0, 3, 1, 3, 0, 8, 0, 8, 4, 7, 2, 6,
    3, 3, 3, 6, 1, 8, 1, 6, 4, 0, 6, 2, 5, 1, 1, 1, 0, 2, 2, 3, 0, 2, 4, 6, 2,
    5, 1, 5, 6, 5, 4, 0, 4, 2, 3, 6, 3, 1, 6, 6, 8, 0, 9, 0, 8, 2, 0, 3, 1, 2,
    5, 5, 5, 5, 1, 1, 1, 5, 1, 2, 3, 1, 2, 5, 7, 8, 2, 7, 0, 2, 1, 1, 8, 1, 5,
    8, 3, 4, 0, 4, 5, 4, 1, 0, 1, 5, 6, 2, 5, 2, 7, 7, 5, 5, 5, 7, 5, 6, 1, 5,
    6, 2, 8, 9, 1, 3, 5, 1, 0, 5, 9, 0, 7, 9, 1, 7, 0, 2, 2, 7, 0, 5, 0, 7, 8,
    1, 2, 5, 1, 3, 8, 7, 7, 7, 8, 7, 8, 0, 7, 8, 1, 4, 4, 5, 6, 7, 5, 5, 2, 9,
    5, 3, 9, 5, 8, 5, 1, 1, 3, 5, 2, 5, 3, 9, 0, 6, 2, 5, 6, 9, 3, 8, 8, 9, 3,
    9, 0, 3, 9, 0, 7, 2, 2, 8, 3, 7, 7, 6, 4, 7, 6, 9, 7, 9, 2, 5, 5, 6, 7, 6,
    2, 6, 9, 5, 3, 1, 2, 5, 3, 4, 6, 9, 4, 4, 6, 9, 5, 1, 9, 5, 3, 6, 1, 4, 1,
    8, 8, 8, 2, 3, 8, 4, 8, 9, 6, 2, 7, 8, 3, 8, 1, 3, 4, 7, 6, 5, 6, 2, 5, 1,
    7, 3, 4, 7, 2, 3, 4, 7, 5, 9, 7, 6, 8, 0, 7, 0, 9, 4, 4, 1, 1, 9, 2, 4, 4,
    8, 1, 3, 9, 1, 9, 0, 6, 7, 3, 8, 2, 8, 1, 2, 5, 8, 6, 7, 3, 6, 1, 7, 3, 7,
    9, 8, 8, 4, 0, 3, 5, 4, 7, 2, 0, 5, 9, 6, 2, 2, 4, 0, 6, 9, 5, 9, 5, 3, 3,
    6, 9, 1, 4, 0, 6, 2, 5,
};

// wuffs_base__private_implementation__high_prec_dec__lshift_num_new_digits
// returns the number of additional decimal digits when left-shifting by shift.
//
// See below for preconditions.
static uint32_t  //
wuffs_base__private_implementation__high_prec_dec__lshift_num_new_digits(
    wuffs_base__private_implementation__high_prec_dec* h,
    uint32_t shift) {
  // Masking with 0x3F should be unnecessary (assuming the preconditions) but
  // it's cheap and ensures that we don't overflow the
  // wuffs_base__private_implementation__hpd_left_shift array.
  shift &= 63;

  uint32_t x_a = wuffs_base__private_implementation__hpd_left_shift[shift];
  uint32_t x_b = wuffs_base__private_implementation__hpd_left_shift[shift + 1];
  uint32_t num_new_digits = x_a >> 11;
  uint32_t pow5_a = 0x7FF & x_a;
  uint32_t pow5_b = 0x7FF & x_b;

  const uint8_t* pow5 =
      &wuffs_base__private_implementation__powers_of_5[pow5_a];
  uint32_t i = 0;
  uint32_t n = pow5_b - pow5_a;
  for (; i < n; i++) {
    if (i >= h->num_digits) {
      return num_new_digits - 1;
    } else if (h->digits[i] == pow5[i]) {
      continue;
    } else if (h->digits[i] < pow5[i]) {
      return num_new_digits - 1;
    } else {
      return num_new_digits;
    }
  }
  return num_new_digits;
}

// --------

// wuffs_base__private_implementation__high_prec_dec__rounded_integer returns
// the integral (non-fractional) part of h, provided that it is 18 or fewer
// decimal digits. For 19 or more digits, it returns UINT64_MAX. Note that:
//   - (1 << 53) is    9007199254740992, which has 16 decimal digits.
//   - (1 << 56) is   72057594037927936, which has 17 decimal digits.
//   - (1 << 59) is  576460752303423488, which has 18 decimal digits.
//   - (1 << 63) is 9223372036854775808, which has 19 decimal digits.
// and that IEEE 754 double precision has 52 mantissa bits.
//
// That integral part is rounded-to-even: rounding 7.5 or 8.5 both give 8.
//
// h's negative bit is ignored: rounding -8.6 returns 9.
//
// See below for preconditions.
static uint64_t  //
wuffs_base__private_implementation__high_prec_dec__rounded_integer(
    wuffs_base__private_implementation__high_prec_dec* h) {
  if ((h->num_digits == 0) || (h->decimal_point < 0)) {
    return 0;
  } else if (h->decimal_point > 18) {
    return UINT64_MAX;
  }

  uint32_t dp = (uint32_t)(h->decimal_point);
  uint64_t n = 0;
  uint32_t i = 0;
  for (; i < dp; i++) {
    n = (10 * n) + ((i < h->num_digits) ? h->digits[i] : 0);
  }

  bool round_up = false;
  if (dp < h->num_digits) {
    round_up = h->digits[dp] >= 5;
    if ((h->digits[dp] == 5) && (dp + 1 == h->num_digits)) {
      // We are exactly halfway. If we're truncated, round up, otherwise round
      // to even.
      round_up = h->truncated ||  //
                 ((dp > 0) && (1 & h->digits[dp - 1]));
    }
  }
  if (round_up) {
    n++;
  }

  return n;
}

// wuffs_base__private_implementation__high_prec_dec__small_xshift shifts h's
// number (where 'x' is 'l' or 'r' for left or right) by a small shift value.
//
// Preconditions:
//  - h is non-NULL.
//  - h->decimal_point is "not extreme".
//  - shift is non-zero.
//  - shift is "a small shift".
//
// "Not extreme" means within
// ±WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE.
//
// "A small shift" means not more than
// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL.
//
// wuffs_base__private_implementation__high_prec_dec__rounded_integer and
// wuffs_base__private_implementation__high_prec_dec__lshift_num_new_digits
// have the same preconditions.

static void  //
wuffs_base__private_implementation__high_prec_dec__small_lshift(
    wuffs_base__private_implementation__high_prec_dec* h,
    uint32_t shift) {
  if (h->num_digits == 0) {
    return;
  }
  uint32_t num_new_digits =
      wuffs_base__private_implementation__high_prec_dec__lshift_num_new_digits(
          h, shift);
  uint32_t rx = h->num_digits - 1;                   // Read  index.
  uint32_t wx = h->num_digits - 1 + num_new_digits;  // Write index.
  uint64_t n = 0;

  // Repeat: pick up a digit, put down a digit, right to left.
  while (((int32_t)rx) >= 0) {
    n += ((uint64_t)(h->digits[rx])) << shift;
    uint64_t quo = n / 10;
    uint64_t rem = n - (10 * quo);
    if (wx < WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
      h->digits[wx] = (uint8_t)rem;
    } else if (rem > 0) {
      h->truncated = true;
    }
    n = quo;
    wx--;
    rx--;
  }

  // Put down leading digits, right to left.
  while (n > 0) {
    uint64_t quo = n / 10;
    uint64_t rem = n - (10 * quo);
    if (wx < WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
      h->digits[wx] = (uint8_t)rem;
    } else if (rem > 0) {
      h->truncated = true;
    }
    n = quo;
    wx--;
  }

  // Finish.
  h->num_digits += num_new_digits;
  if (h->num_digits >
      WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
    h->num_digits = WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION;
  }
  h->decimal_point += (int32_t)num_new_digits;
  wuffs_base__private_implementation__high_prec_dec__trim(h);
}

static void  //
wuffs_base__private_implementation__high_prec_dec__small_rshift(
    wuffs_base__private_implementation__high_prec_dec* h,
    uint32_t shift) {
  uint32_t rx = 0;  // Read  index.
  uint32_t wx = 0;  // Write index.
  uint64_t n = 0;

  // Pick up enough leading digits to cover the first shift.
  while ((n >> shift) == 0) {
    if (rx < h->num_digits) {
      // Read a digit.
      n = (10 * n) + h->digits[rx++];
    } else if (n == 0) {
      // h's number used to be zero and remains zero.
      return;
    } else {
      // Read sufficient implicit trailing zeroes.
      while ((n >> shift) == 0) {
        n = 10 * n;
        rx++;
      }
      break;
    }
  }
  h->decimal_point -= ((int32_t)(rx - 1));
  if (h->decimal_point <
      -WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
    // After the shift, h's number is effectively zero.
    h->num_digits = 0;
    h->decimal_point = 0;
    h->negative = false;
    h->truncated = false;
    return;
  }

  // Repeat: pick up a digit, put down a digit, left to right.
  uint64_t mask = (((uint64_t)(1)) << shift) - 1;
  while (rx < h->num_digits) {
    uint8_t new_digit = ((uint8_t)(n >> shift));
    n = (10 * (n & mask)) + h->digits[rx++];
    h->digits[wx++] = new_digit;
  }

  // Put down trailing digits, left to right.
  while (n > 0) {
    uint8_t new_digit = ((uint8_t)(n >> shift));
    n = 10 * (n & mask);
    if (wx < WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
      h->digits[wx++] = new_digit;
    } else if (new_digit > 0) {
      h->truncated = true;
    }
  }

  // Finish.
  h->num_digits = wx;
  wuffs_base__private_implementation__high_prec_dec__trim(h);
}

// --------

// The wuffs_base__private_implementation__etc_powers_of_10 tables were printed
// by script/print-mpb-powers-of-10.go. That script has an optional -comments
// flag, whose output is not copied here, which prints further detail.
//
// These tables are used in
// wuffs_base__private_implementation__medium_prec_bin__assign_from_hpd.

// wuffs_base__private_implementation__big_powers_of_10 contains approximations
// to the powers of 10, ranging from 1e-348 to 1e+340, with the exponent
// stepping by 8: -348, -340, -332, ..., -12, -4, +4, +12, ..., +340. Each step
// consists of three uint32_t elements. There are 87 triples, 87 * 3 = 261.
//
// For example, the third approximation, for 1e-332, consists of the uint32_t
// triple (0x3055AC76, 0x8B16FB20, 0xFFFFFB72). The first two of that triple
// are a little-endian uint64_t value: 0x8B16FB203055AC76. The last one is an
// int32_t value: -1166. Together, they represent the approximation:
//   1e-332 ≈ 0x8B16FB203055AC76 * (2 ** -1166)
// Similarly, the (0x00000000, 0x9C400000, 0xFFFFFFCE) uint32_t triple means:
//   1e+4   ≈ 0x9C40000000000000 * (2 **   -50)  // This approx'n is exact.
// Similarly, the (0xD4C4FB27, 0xED63A231, 0x000000A2) uint32_t triple means:
//   1e+68  ≈ 0xED63A231D4C4FB27 * (2 **   162)
static const uint32_t
    wuffs_base__private_implementation__big_powers_of_10[261] = {
        0x081C0288, 0xFA8FD5A0, 0xFFFFFB3C, 0xA23EBF76, 0xBAAEE17F, 0xFFFFFB57,
        0x3055AC76, 0x8B16FB20, 0xFFFFFB72, 0x5DCE35EA, 0xCF42894A, 0xFFFFFB8C,
        0x55653B2D, 0x9A6BB0AA, 0xFFFFFBA7, 0x3D1A45DF, 0xE61ACF03, 0xFFFFFBC1,
        0xC79AC6CA, 0xAB70FE17, 0xFFFFFBDC, 0xBEBCDC4F, 0xFF77B1FC, 0xFFFFFBF6,
        0x416BD60C, 0xBE5691EF, 0xFFFFFC11, 0x907FFC3C, 0x8DD01FAD, 0xFFFFFC2C,
        0x31559A83, 0xD3515C28, 0xFFFFFC46, 0xADA6C9B5, 0x9D71AC8F, 0xFFFFFC61,
        0x23EE8BCB, 0xEA9C2277, 0xFFFFFC7B, 0x4078536D, 0xAECC4991, 0xFFFFFC96,
        0x5DB6CE57, 0x823C1279, 0xFFFFFCB1, 0x4DFB5637, 0xC2109436, 0xFFFFFCCB,
        0x3848984F, 0x9096EA6F, 0xFFFFFCE6, 0x25823AC7, 0xD77485CB, 0xFFFFFD00,
        0x97BF97F4, 0xA086CFCD, 0xFFFFFD1B, 0x172AACE5, 0xEF340A98, 0xFFFFFD35,
        0x2A35B28E, 0xB23867FB, 0xFFFFFD50, 0xD2C63F3B, 0x84C8D4DF, 0xFFFFFD6B,
        0x1AD3CDBA, 0xC5DD4427, 0xFFFFFD85, 0xBB25C996, 0x936B9FCE, 0xFFFFFDA0,
        0x7D62A584, 0xDBAC6C24, 0xFFFFFDBA, 0x0D5FDAF6, 0xA3AB6658, 0xFFFFFDD5,
        0xDEC3F126, 0xF3E2F893, 0xFFFFFDEF, 0xAAFF80B8, 0xB5B5ADA8, 0xFFFFFE0A,
        0x6C7C4A8B, 0x87625F05, 0xFFFFFE25, 0x34C13053, 0xC9BCFF60, 0xFFFFFE3F,
        0x91BA2655, 0x964E858C, 0xFFFFFE5A, 0x70297EBD, 0xDFF97724, 0xFFFFFE74,
        0xB8E5B88F, 0xA6DFBD9F, 0xFFFFFE8F, 0x88747D94, 0xF8A95FCF, 0xFFFFFEA9,
        0x8FA89BCF, 0xB9447093, 0xFFFFFEC4, 0xBF0F156B, 0x8A08F0F8, 0xFFFFFEDF,
        0x653131B6, 0xCDB02555, 0xFFFFFEF9, 0xD07B7FAC, 0x993FE2C6, 0xFFFFFF14,
        0x2A2B3B06, 0xE45C10C4, 0xFFFFFF2E, 0x697392D3, 0xAA242499, 0xFFFFFF49,
        0x8300CA0E, 0xFD87B5F2, 0xFFFFFF63, 0x92111AEB, 0xBCE50864, 0xFFFFFF7E,
        0x6F5088CC, 0x8CBCCC09, 0xFFFFFF99, 0xE219652C, 0xD1B71758, 0xFFFFFFB3,
        0x00000000, 0x9C400000, 0xFFFFFFCE, 0x00000000, 0xE8D4A510, 0xFFFFFFE8,
        0xAC620000, 0xAD78EBC5, 0x00000003, 0xF8940984, 0x813F3978, 0x0000001E,
        0xC90715B3, 0xC097CE7B, 0x00000038, 0x7BEA5C70, 0x8F7E32CE, 0x00000053,
        0xABE98068, 0xD5D238A4, 0x0000006D, 0x179A2245, 0x9F4F2726, 0x00000088,
        0xD4C4FB27, 0xED63A231, 0x000000A2, 0x8CC8ADA8, 0xB0DE6538, 0x000000BD,
        0x1AAB65DB, 0x83C7088E, 0x000000D8, 0x42711D9A, 0xC45D1DF9, 0x000000F2,
        0xA61BE758, 0x924D692C, 0x0000010D, 0x1A708DEA, 0xDA01EE64, 0x00000127,
        0x9AEF774A, 0xA26DA399, 0x00000142, 0xB47D6B85, 0xF209787B, 0x0000015C,
        0x79DD1877, 0xB454E4A1, 0x00000177, 0x5B9BC5C2, 0x865B8692, 0x00000192,
        0xC8965D3D, 0xC83553C5, 0x000001AC, 0xFA97A0B3, 0x952AB45C, 0x000001C7,
        0x99A05FE3, 0xDE469FBD, 0x000001E1, 0xDB398C25, 0xA59BC234, 0x000001FC,
        0xA3989F5C, 0xF6C69A72, 0x00000216, 0x54E9BECE, 0xB7DCBF53, 0x00000231,
        0xF22241E2, 0x88FCF317, 0x0000024C, 0xD35C78A5, 0xCC20CE9B, 0x00000266,
        0x7B2153DF, 0x98165AF3, 0x00000281, 0x971F303A, 0xE2A0B5DC, 0x0000029B,
        0x5CE3B396, 0xA8D9D153, 0x000002B6, 0xA4A7443C, 0xFB9B7CD9, 0x000002D0,
        0xA7A44410, 0xBB764C4C, 0x000002EB, 0xB6409C1A, 0x8BAB8EEF, 0x00000306,
        0xA657842C, 0xD01FEF10, 0x00000320, 0xE9913129, 0x9B10A4E5, 0x0000033B,
        0xA19C0C9D, 0xE7109BFB, 0x00000355, 0x623BF429, 0xAC2820D9, 0x00000370,
        0x7AA7CF85, 0x80444B5E, 0x0000038B, 0x03ACDD2D, 0xBF21E440, 0x000003A5,
        0x5E44FF8F, 0x8E679C2F, 0x000003C0, 0x9C8CB841, 0xD433179D, 0x000003DA,
        0xB4E31BA9, 0x9E19DB92, 0x000003F5, 0xBADF77D9, 0xEB96BF6E, 0x0000040F,
        0x9BF0EE6B, 0xAF87023B, 0x0000042A,
};

// wuffs_base__private_implementation__small_powers_of_10 contains
// approximations to the powers of 10, ranging from 1e+0 to 1e+7, with the
// exponent stepping by 1. Each step consists of three uint32_t elements.
//
// For example, the third approximation, for 1e+2, consists of the uint32_t
// triple (0x00000000, 0xC8000000, 0xFFFFFFC7). The first two of that triple
// are a little-endian uint64_t value: 0xC800000000000000. The last one is an
// int32_t value: -57. Together, they represent the approximation:
//   1e+2   ≈ 0xC800000000000000 * (2 **   -57)  // This approx'n is exact.
// Similarly, the (0x00000000, 0x9C400000, 0xFFFFFFCE) uint32_t triple means:
//   1e+4   ≈ 0x9C40000000000000 * (2 **   -50)  // This approx'n is exact.
static const uint32_t
    wuffs_base__private_implementation__small_powers_of_10[24] = {
        0x00000000, 0x80000000, 0xFFFFFFC1, 0x00000000, 0xA0000000, 0xFFFFFFC4,
        0x00000000, 0xC8000000, 0xFFFFFFC7, 0x00000000, 0xFA000000, 0xFFFFFFCA,
        0x00000000, 0x9C400000, 0xFFFFFFCE, 0x00000000, 0xC3500000, 0xFFFFFFD1,
        0x00000000, 0xF4240000, 0xFFFFFFD4, 0x00000000, 0x98968000, 0xFFFFFFD8,
};

// wuffs_base__private_implementation__f64_powers_of_10 holds powers of 10 that
// can be exactly represented by a float64 (what C calls a double).
static const double wuffs_base__private_implementation__f64_powers_of_10[23] = {
    1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
    1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22,
};

// --------

// wuffs_base__private_implementation__medium_prec_bin (abbreviated as MPB) is
// a fixed precision floating point binary number. Unlike IEEE 754 Floating
// Point, it cannot represent infinity or NaN (Not a Number).
//
// "Medium precision" means that the mantissa holds 64 binary digits, a little
// more than "double precision", and sizeof(MPB) > sizeof(double). 64 is
// obviously the number of bits in a uint64_t.
//
// An MPB isn't for general purpose arithmetic, only for conversions to and
// from IEEE 754 double-precision floating point.
//
// There is no implicit mantissa bit. The mantissa field is zero if and only if
// the overall floating point value is ±0. An MPB is normalized if the mantissa
// is zero or its high bit (the 1<<63 bit) is set.
//
// There is no negative bit. An MPB can only represent non-negative numbers.
//
// The "all fields are zero" value is valid, and represents the number +0.
//
// This is the "Do It Yourself Floating Point" data structure from Loitsch,
// "Printing Floating-Point Numbers Quickly and Accurately with Integers"
// (https://www.cs.tufts.edu/~nr/cs257/archive/florian-loitsch/printf.pdf).
//
// Florian Loitsch is also the primary contributor to
// https://github.com/google/double-conversion
typedef struct {
  uint64_t mantissa;
  int32_t exp2;
} wuffs_base__private_implementation__medium_prec_bin;

static uint32_t  //
wuffs_base__private_implementation__medium_prec_bin__normalize(
    wuffs_base__private_implementation__medium_prec_bin* m) {
  if (m->mantissa == 0) {
    return 0;
  }
  uint32_t shift = wuffs_base__count_leading_zeroes_u64(m->mantissa);
  m->mantissa <<= shift;
  m->exp2 -= (int32_t)shift;
  return shift;
}

// wuffs_base__private_implementation__medium_prec_bin__mul_pow_10 sets m to be
// (m * pow), where pow comes from an etc_powers_of_10 triple starting at p.
//
// The result is rounded, but not necessarily normalized.
//
// Preconditions:
//  - m is non-NULL.
//  - m->mantissa is non-zero.
//  - m->mantissa's high bit is set (i.e. m is normalized).
//
// The etc_powers_of_10 triple is already normalized.
static void  //
wuffs_base__private_implementation__medium_prec_bin__mul_pow_10(
    wuffs_base__private_implementation__medium_prec_bin* m,
    const uint32_t* p) {
  uint64_t p_mantissa = ((uint64_t)p[0]) | (((uint64_t)p[1]) << 32);
  int32_t p_exp2 = (int32_t)p[2];

  wuffs_base__multiply_u64__output o =
      wuffs_base__multiply_u64(m->mantissa, p_mantissa);
  // Round the mantissa up. It cannot overflow because the maximum possible
  // value of o.hi is 0xFFFFFFFFFFFFFFFE.
  m->mantissa = o.hi + (o.lo >> 63);
  m->exp2 = m->exp2 + p_exp2 + 64;
}

// wuffs_base__private_implementation__medium_prec_bin__as_f64 converts m to a
// double (what C calls a double-precision float64).
//
// Preconditions:
//  - m is non-NULL.
//  - m->mantissa is non-zero.
//  - m->mantissa's high bit is set (i.e. m is normalized).
static double  //
wuffs_base__private_implementation__medium_prec_bin__as_f64(
    const wuffs_base__private_implementation__medium_prec_bin* m,
    bool negative) {
  uint64_t mantissa64 = m->mantissa;
  // An mpb's mantissa has the implicit (binary) decimal point at the right
  // hand end of the mantissa's explicit digits. A double-precision's mantissa
  // has that decimal point near the left hand end. There's also an explicit
  // versus implicit leading 1 bit (binary digit). Together, the difference in
  // semantics corresponds to adding 63.
  int32_t exp2 = m->exp2 + 63;

  // Ensure that exp2 is at least -1022, the minimum double-precision exponent
  // for normal (as opposed to subnormal) numbers.
  if (-1022 > exp2) {
    uint32_t n = (uint32_t)(-1022 - exp2);
    mantissa64 >>= n;
    exp2 += (int32_t)n;
  }

  // Extract the (1 + 52) bits from the 64-bit mantissa64. 52 is the number of
  // explicit mantissa bits in a double-precision f64.
  //
  // Before, we have 64 bits and due to normalization, the high bit 'H' is 1.
  // 63        55        47       etc     15        7
  // H210_9876_5432_1098_7654_etc_etc_etc_5432_1098_7654_3210
  // ++++_++++_++++_++++_++++_etc_etc_etc_++++_+..._...._....  Kept bits.
  // ...._...._...H_2109_8765_etc_etc_etc_6543_2109_8765_4321  After shifting.
  // After, we have 53 bits (and bit #52 is this 'H' bit).
  uint64_t mantissa53 = mantissa64 >> 11;

  // Round up if the old bit #10 (the highest bit dropped by shifting) was set.
  // We also fix any overflow from rounding up.
  if (mantissa64 & 1024) {
    mantissa53++;
    if ((mantissa53 >> 53) != 0) {
      mantissa53 >>= 1;
      exp2++;
    }
  }

  // Handle double-precision infinity (a nominal exponent of 1024) and
  // subnormals (an exponent of -1023 and no implicit mantissa bit, bit #52).
  if (exp2 >= 1024) {
    mantissa53 = 0;
    exp2 = 1024;
  } else if ((mantissa53 >> 52) == 0) {
    exp2 = -1023;
  }

  // Pack the bits and return.
  const int32_t f64_bias = -1023;
  uint64_t exp2_bits =
      (uint64_t)((exp2 - f64_bias) & 0x07FF);           // (1 << 11) - 1.
  uint64_t bits = (mantissa53 & 0x000FFFFFFFFFFFFF) |   // (1 << 52) - 1.
                  (exp2_bits << 52) |                   //
                  (negative ? 0x8000000000000000 : 0);  // (1 << 63).
  return wuffs_base__ieee_754_bit_representation__to_f64(bits);
}

// wuffs_base__private_implementation__medium_prec_bin__parse_number_f64
// converts from an HPD to a double, using an MPB as scratch space. It returns
// a NULL status.repr if there is no ambiguity in the truncation or rounding to
// a float64 (an IEEE 754 double-precision floating point value).
//
// It may modify m even if it returns a non-NULL status.repr.
static wuffs_base__result_f64  //
wuffs_base__private_implementation__medium_prec_bin__parse_number_f64(
    wuffs_base__private_implementation__medium_prec_bin* m,
    const wuffs_base__private_implementation__high_prec_dec* h,
    bool skip_fast_path_for_tests) {
  do {
    // m->mantissa is a uint64_t, which is an integer approximation to a
    // rational value - h's underlying digits after m's normalization. This
    // error is an upper bound on the difference between the approximate and
    // actual value.
    //
    // The DiyFpStrtod function in https://github.com/google/double-conversion
    // uses a finer grain (1/8th of the ULP, Unit in the Last Place) when
    // tracking error. This implementation is coarser (1 ULP) but simpler.
    //
    // It is an error in the "numerical approximation" sense, not in the
    // typical programming sense (as in "bad input" or "a result type").
    uint64_t error = 0;

    // Convert up to 19 decimal digits (in h->digits) to 64 binary digits (in
    // m->mantissa): (1e19 < (1<<64)) and ((1<<64) < 1e20). If we have more
    // than 19 digits, we're truncating (with error).
    uint32_t i;
    uint32_t i_end = h->num_digits;
    if (i_end > 19) {
      i_end = 19;
      error = 1;
    }
    uint64_t mantissa = 0;
    for (i = 0; i < i_end; i++) {
      mantissa = (10 * mantissa) + h->digits[i];
    }
    m->mantissa = mantissa;
    m->exp2 = 0;

    // Check that exp10 lies in the (big_powers_of_10 + small_powers_of_10)
    // range, -348 ..= +347, stepping big_powers_of_10 by 8 (which is 87
    // triples) and small_powers_of_10 by 1 (which is 8 triples).
    int32_t exp10 = h->decimal_point - ((int32_t)(i_end));
    if (exp10 < -348) {
      goto fail;
    }
    uint32_t bpo10 = ((uint32_t)(exp10 + 348)) / 8;
    uint32_t spo10 = ((uint32_t)(exp10 + 348)) % 8;
    if (bpo10 >= 87) {
      goto fail;
    }

    // Try a fast path, if float64 math would be exact.
    //
    // 15 is such that 1e15 can be losslessly represented in a float64
    // mantissa: (1e15 < (1<<53)) and ((1<<53) < 1e16).
    //
    // 22 is the maximum valid index for the
    // wuffs_base__private_implementation__f64_powers_of_10 array.
    do {
      if (skip_fast_path_for_tests || ((mantissa >> 52) != 0)) {
        break;
      }
      double d = (double)mantissa;

      if (exp10 == 0) {
        wuffs_base__result_f64 ret;
        ret.status.repr = NULL;
        ret.value = h->negative ? -d : +d;
        return ret;

      } else if (exp10 > 0) {
        if (exp10 > 22) {
          if (exp10 > (15 + 22)) {
            break;
          }
          // If exp10 is in the range 23 ..= 37, try moving a few of the zeroes
          // from the exponent to the mantissa. If we're still under 1e15, we
          // haven't truncated any mantissa bits.
          if (exp10 > 22) {
            d *= wuffs_base__private_implementation__f64_powers_of_10[exp10 -
                                                                      22];
            exp10 = 22;
            if (d >= 1e15) {
              break;
            }
          }
        }
        d *= wuffs_base__private_implementation__f64_powers_of_10[exp10];
        wuffs_base__result_f64 ret;
        ret.status.repr = NULL;
        ret.value = h->negative ? -d : +d;
        return ret;

      } else {  // "if (exp10 < 0)" is effectively "if (true)" here.
        if (exp10 < -22) {
          break;
        }
        d /= wuffs_base__private_implementation__f64_powers_of_10[-exp10];
        wuffs_base__result_f64 ret;
        ret.status.repr = NULL;
        ret.value = h->negative ? -d : +d;
        return ret;
      }
    } while (0);

    // Normalize (and scale the error).
    error <<= wuffs_base__private_implementation__medium_prec_bin__normalize(m);

    // Multiplying two MPB values nominally multiplies two mantissas, call them
    // A and B, which are integer approximations to the precise values (A+a)
    // and (B+b) for some error terms a and b.
    //
    // MPB multiplication calculates (((A+a) * (B+b)) >> 64) to be ((A*B) >>
    // 64). Shifting (truncating) and rounding introduces further error. The
    // difference between the calculated result:
    //  ((A*B                  ) >> 64)
    // and the true result:
    //  ((A*B + A*b + a*B + a*b) >> 64)   + rounding_error
    // is:
    //  ((      A*b + a*B + a*b) >> 64)   + rounding_error
    // which can be re-grouped as:
    //  ((A*b) >> 64) + ((a*(B+b)) >> 64) + rounding_error
    //
    // Now, let A and a be "m->mantissa" and "error", and B and b be the
    // pre-calculated power of 10. A and B are both less than (1 << 64), a is
    // the "error" local variable and b is less than 1.
    //
    // An upper bound (in absolute value) on ((A*b) >> 64) is therefore 1.
    //
    // An upper bound on ((a*(B+b)) >> 64) is a, also known as error.
    //
    // Finally, the rounding_error is at most 1.
    //
    // In total, calling mpb__mul_pow_10 will raise the worst-case error by 2.
    // The subsequent re-normalization can multiply that by a further factor.

    // Multiply by small_powers_of_10[etc].
    wuffs_base__private_implementation__medium_prec_bin__mul_pow_10(
        m, &wuffs_base__private_implementation__small_powers_of_10[3 * spo10]);
    error += 2;
    error <<= wuffs_base__private_implementation__medium_prec_bin__normalize(m);

    // Multiply by big_powers_of_10[etc].
    wuffs_base__private_implementation__medium_prec_bin__mul_pow_10(
        m, &wuffs_base__private_implementation__big_powers_of_10[3 * bpo10]);
    error += 2;
    error <<= wuffs_base__private_implementation__medium_prec_bin__normalize(m);

    // We have a good approximation of h, but we still have to check whether
    // the error is small enough. Equivalently, whether the number of surplus
    // mantissa bits (the bits dropped when going from m's 64 mantissa bits to
    // the smaller number of double-precision mantissa bits) would always round
    // up or down, even when perturbed by ±error. We start at 11 surplus bits
    // (m has 64, double-precision has 1+52), but it can be higher for
    // subnormals.
    //
    // In many cases, the error is small enough and we return true.
    const int32_t f64_bias = -1023;
    int32_t subnormal_exp2 = f64_bias - 63;
    uint32_t surplus_bits = 11;
    if (subnormal_exp2 >= m->exp2) {
      surplus_bits += 1 + ((uint32_t)(subnormal_exp2 - m->exp2));
    }

    uint64_t surplus_mask =
        (((uint64_t)1) << surplus_bits) - 1;  // e.g. 0x07FF.
    uint64_t surplus = m->mantissa & surplus_mask;
    uint64_t halfway = ((uint64_t)1) << (surplus_bits - 1);  // e.g. 0x0400.

    // Do the final calculation in *signed* arithmetic.
    int64_t i_surplus = (int64_t)surplus;
    int64_t i_halfway = (int64_t)halfway;
    int64_t i_error = (int64_t)error;

    if ((i_surplus > (i_halfway - i_error)) &&
        (i_surplus < (i_halfway + i_error))) {
      goto fail;
    }

    wuffs_base__result_f64 ret;
    ret.status.repr = NULL;
    ret.value = wuffs_base__private_implementation__medium_prec_bin__as_f64(
        m, h->negative);
    return ret;
  } while (0);

fail:
  do {
    wuffs_base__result_f64 ret;
    ret.status.repr = "#base: mpb__parse_number_f64 failed";
    ret.value = 0;
    return ret;
  } while (0);
}

// --------

static wuffs_base__result_f64  //
wuffs_base__parse_number_f64_special(wuffs_base__slice_u8 s,
                                     const char* fallback_status_repr) {
  do {
    uint8_t* p = s.ptr;
    uint8_t* q = s.ptr + s.len;

    for (; (p < q) && (*p == '_'); p++) {
    }
    if (p >= q) {
      goto fallback;
    }

    // Parse sign.
    bool negative = false;
    do {
      if (*p == '+') {
        p++;
      } else if (*p == '-') {
        negative = true;
        p++;
      } else {
        break;
      }
      for (; (p < q) && (*p == '_'); p++) {
      }
    } while (0);
    if (p >= q) {
      goto fallback;
    }

    bool nan = false;
    switch (p[0]) {
      case 'I':
      case 'i':
        if (((q - p) < 3) ||                     //
            ((p[1] != 'N') && (p[1] != 'n')) ||  //
            ((p[2] != 'F') && (p[2] != 'f'))) {
          goto fallback;
        }
        p += 3;

        if ((p >= q) || (*p == '_')) {
          break;
        } else if (((q - p) < 5) ||                     //
                   ((p[0] != 'I') && (p[0] != 'i')) ||  //
                   ((p[1] != 'N') && (p[1] != 'n')) ||  //
                   ((p[2] != 'I') && (p[2] != 'i')) ||  //
                   ((p[3] != 'T') && (p[3] != 't')) ||  //
                   ((p[4] != 'Y') && (p[4] != 'y'))) {
          goto fallback;
        }
        p += 5;

        if ((p >= q) || (*p == '_')) {
          break;
        }
        goto fallback;

      case 'N':
      case 'n':
        if (((q - p) < 3) ||                     //
            ((p[1] != 'A') && (p[1] != 'a')) ||  //
            ((p[2] != 'N') && (p[2] != 'n'))) {
          goto fallback;
        }
        p += 3;

        if ((p >= q) || (*p == '_')) {
          nan = true;
          break;
        }
        goto fallback;

      default:
        goto fallback;
    }

    // Finish.
    for (; (p < q) && (*p == '_'); p++) {
    }
    if (p != q) {
      goto fallback;
    }
    wuffs_base__result_f64 ret;
    ret.status.repr = NULL;
    ret.value = wuffs_base__ieee_754_bit_representation__to_f64(
        (nan ? 0x7FFFFFFFFFFFFFFF : 0x7FF0000000000000) |
        (negative ? 0x8000000000000000 : 0));
    return ret;
  } while (0);

fallback:
  do {
    wuffs_base__result_f64 ret;
    ret.status.repr = fallback_status_repr;
    ret.value = 0;
    return ret;
  } while (0);
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__result_f64  //
wuffs_base__parse_number_f64(wuffs_base__slice_u8 s) {
  wuffs_base__private_implementation__medium_prec_bin m;
  wuffs_base__private_implementation__high_prec_dec h;

  do {
    // powers converts decimal powers of 10 to binary powers of 2. For example,
    // (10000 >> 13) is 1. It stops before the elements exceed 60, also known
    // as WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL.
    static const uint32_t num_powers = 19;
    static const uint8_t powers[19] = {
        0,  3,  6,  9,  13, 16, 19, 23, 26, 29,  //
        33, 36, 39, 43, 46, 49, 53, 56, 59,      //
    };

    wuffs_base__status status =
        wuffs_base__private_implementation__high_prec_dec__parse(&h, s);
    if (status.repr) {
      return wuffs_base__parse_number_f64_special(s, status.repr);
    }

    // Handle zero and obvious extremes. The largest and smallest positive
    // finite f64 values are approximately 1.8e+308 and 4.9e-324.
    if ((h.num_digits == 0) || (h.decimal_point < -326)) {
      goto zero;
    } else if (h.decimal_point > 310) {
      goto infinity;
    }

    wuffs_base__result_f64 mpb_result =
        wuffs_base__private_implementation__medium_prec_bin__parse_number_f64(
            &m, &h, false);
    if (mpb_result.status.repr == NULL) {
      return mpb_result;
    }

    // Scale by powers of 2 until we're in the range [½ .. 1], which gives us
    // our exponent (in base-2). First we shift right, possibly a little too
    // far, ending with a value certainly below 1 and possibly below ½...
    const int32_t f64_bias = -1023;
    int32_t exp2 = 0;
    while (h.decimal_point > 0) {
      uint32_t n = (uint32_t)(+h.decimal_point);
      uint32_t shift =
          (n < num_powers)
              ? powers[n]
              : WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL;

      wuffs_base__private_implementation__high_prec_dec__small_rshift(&h,
                                                                      shift);
      if (h.decimal_point <
          -WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
        goto zero;
      }
      exp2 += (int32_t)shift;
    }
    // ...then we shift left, putting us in [½ .. 1].
    while (h.decimal_point <= 0) {
      uint32_t shift;
      if (h.decimal_point == 0) {
        if (h.digits[0] >= 5) {
          break;
        }
        shift = (h.digits[0] <= 2) ? 2 : 1;
      } else {
        uint32_t n = (uint32_t)(-h.decimal_point);
        shift = (n < num_powers)
                    ? powers[n]
                    : WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL;
      }

      wuffs_base__private_implementation__high_prec_dec__small_lshift(&h,
                                                                      shift);
      if (h.decimal_point >
          +WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
        goto infinity;
      }
      exp2 -= (int32_t)shift;
    }

    // We're in the range [½ .. 1] but f64 uses [1 .. 2].
    exp2--;

    // The minimum normal exponent is (f64_bias + 1).
    while ((f64_bias + 1) > exp2) {
      uint32_t n = (uint32_t)((f64_bias + 1) - exp2);
      if (n > WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL) {
        n = WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL;
      }
      wuffs_base__private_implementation__high_prec_dec__small_rshift(&h, n);
      exp2 += (int32_t)n;
    }

    // Check for overflow.
    if ((exp2 - f64_bias) >= 0x07FF) {  // (1 << 11) - 1.
      goto infinity;
    }

    // Extract 53 bits for the mantissa (in base-2).
    wuffs_base__private_implementation__high_prec_dec__small_lshift(&h, 53);
    uint64_t man2 =
        wuffs_base__private_implementation__high_prec_dec__rounded_integer(&h);

    // Rounding might have added one bit. If so, shift and re-check overflow.
    if ((man2 >> 53) != 0) {
      man2 >>= 1;
      exp2++;
      if ((exp2 - f64_bias) >= 0x07FF) {  // (1 << 11) - 1.
        goto infinity;
      }
    }

    // Handle subnormal numbers.
    if ((man2 >> 52) == 0) {
      exp2 = f64_bias;
    }

    // Pack the bits and return.
    uint64_t exp2_bits =
        (uint64_t)((exp2 - f64_bias) & 0x07FF);             // (1 << 11) - 1.
    uint64_t bits = (man2 & 0x000FFFFFFFFFFFFF) |           // (1 << 52) - 1.
                    (exp2_bits << 52) |                     //
                    (h.negative ? 0x8000000000000000 : 0);  // (1 << 63).

    wuffs_base__result_f64 ret;
    ret.status.repr = NULL;
    ret.value = wuffs_base__ieee_754_bit_representation__to_f64(bits);
    return ret;
  } while (0);

zero:
  do {
    uint64_t bits = h.negative ? 0x8000000000000000 : 0;

    wuffs_base__result_f64 ret;
    ret.status.repr = NULL;
    ret.value = wuffs_base__ieee_754_bit_representation__to_f64(bits);
    return ret;
  } while (0);

infinity:
  do {
    uint64_t bits = h.negative ? 0xFFF0000000000000 : 0x7FF0000000000000;

    wuffs_base__result_f64 ret;
    ret.status.repr = NULL;
    ret.value = wuffs_base__ieee_754_bit_representation__to_f64(bits);
    return ret;
  } while (0);
}

#endif  // !defined(WUFFS_CONFIG__MODULES) ||
        // defined(WUFFS_CONFIG__MODULE__BASE) ||
        // defined(WUFFS_CONFIG__MODULE__BASE__F64CONV)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__BASE) || \
    defined(WUFFS_CONFIG__MODULE__BASE__PIXCONV)

// ---------------- Pixel Swizzler

static inline uint32_t  //
wuffs_base__swap_u32_argb_abgr(uint32_t u) {
  uint32_t o = u & 0xFF00FF00;
  uint32_t r = u & 0x00FF0000;
  uint32_t b = u & 0x000000FF;
  return o | (r >> 16) | (b << 16);
}

// --------

WUFFS_BASE__MAYBE_STATIC wuffs_base__color_u32_argb_premul  //
wuffs_base__pixel_buffer__color_u32_at(const wuffs_base__pixel_buffer* pb,
                                       uint32_t x,
                                       uint32_t y) {
  if (!pb || (x >= pb->pixcfg.private_impl.width) ||
      (y >= pb->pixcfg.private_impl.height)) {
    return 0;
  }

  if (wuffs_base__pixel_format__is_planar(&pb->pixcfg.private_impl.pixfmt)) {
    // TODO: support planar formats.
    return 0;
  }

  size_t stride = pb->private_impl.planes[0].stride;
  const uint8_t* row = pb->private_impl.planes[0].ptr + (stride * ((size_t)y));

  switch (pb->pixcfg.private_impl.pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
      return wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x)));

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY: {
      uint8_t* palette = pb->private_impl.planes[3].ptr;
      return wuffs_base__load_u32le__no_bounds_check(palette +
                                                     (4 * ((size_t)row[x])));
    }

      // Common formats above. Rarer formats below.

    case WUFFS_BASE__PIXEL_FORMAT__Y:
      return 0xFF000000 | (0x00010101 * ((uint32_t)(row[x])));

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL: {
      uint8_t* palette = pb->private_impl.planes[3].ptr;
      return wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
          wuffs_base__load_u32le__no_bounds_check(palette +
                                                  (4 * ((size_t)row[x]))));
    }

    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      return wuffs_base__color_u16_rgb_565__as__color_u32_argb_premul(
          wuffs_base__load_u16le__no_bounds_check(row + (2 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      return 0xFF000000 |
             wuffs_base__load_u24le__no_bounds_check(row + (3 * ((size_t)x)));
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      return wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      return 0xFF000000 |
             wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x)));

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      return wuffs_base__swap_u32_argb_abgr(
          0xFF000000 |
          wuffs_base__load_u24le__no_bounds_check(row + (3 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
      return wuffs_base__swap_u32_argb_abgr(
          wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
              wuffs_base__load_u32le__no_bounds_check(row +
                                                      (4 * ((size_t)x)))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
      return wuffs_base__swap_u32_argb_abgr(
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      return wuffs_base__swap_u32_argb_abgr(
          0xFF000000 |
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));

    default:
      // TODO: support more formats.
      break;
  }

  return 0;
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__status  //
wuffs_base__pixel_buffer__set_color_u32_at(
    wuffs_base__pixel_buffer* pb,
    uint32_t x,
    uint32_t y,
    wuffs_base__color_u32_argb_premul color) {
  if (!pb) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if ((x >= pb->pixcfg.private_impl.width) ||
      (y >= pb->pixcfg.private_impl.height)) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }

  if (wuffs_base__pixel_format__is_planar(&pb->pixcfg.private_impl.pixfmt)) {
    // TODO: support planar formats.
    return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }

  size_t stride = pb->private_impl.planes[0].stride;
  uint8_t* row = pb->private_impl.planes[0].ptr + (stride * ((size_t)y));

  switch (pb->pixcfg.private_impl.pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      wuffs_base__store_u32le__no_bounds_check(row + (4 * ((size_t)x)), color);
      break;

      // Common formats above. Rarer formats below.

    case WUFFS_BASE__PIXEL_FORMAT__Y:
      wuffs_base__store_u8__no_bounds_check(
          row + ((size_t)x),
          wuffs_base__color_u32_argb_premul__as__color_u8_gray(color));
      break;

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
      wuffs_base__store_u8__no_bounds_check(
          row + ((size_t)x), wuffs_base__pixel_palette__closest_element(
                                 wuffs_base__pixel_buffer__palette(pb),
                                 pb->pixcfg.private_impl.pixfmt, color));
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      wuffs_base__store_u16le__no_bounds_check(
          row + (2 * ((size_t)x)),
          wuffs_base__color_u32_argb_premul__as__color_u16_rgb_565(color));
      break;
    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      wuffs_base__store_u24le__no_bounds_check(row + (3 * ((size_t)x)), color);
      break;
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)),
          wuffs_base__color_u32_argb_premul__as__color_u32_argb_nonpremul(
              color));
      break;

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      wuffs_base__store_u24le__no_bounds_check(
          row + (3 * ((size_t)x)), wuffs_base__swap_u32_argb_abgr(color));
      break;
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)),
          wuffs_base__color_u32_argb_premul__as__color_u32_argb_nonpremul(
              wuffs_base__swap_u32_argb_abgr(color)));
      break;
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)), wuffs_base__swap_u32_argb_abgr(color));
      break;

    default:
      // TODO: support more formats.
      return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }

  return wuffs_base__make_status(NULL);
}

// --------

WUFFS_BASE__MAYBE_STATIC uint8_t  //
wuffs_base__pixel_palette__closest_element(
    wuffs_base__slice_u8 palette_slice,
    wuffs_base__pixel_format palette_format,
    wuffs_base__color_u32_argb_premul c) {
  size_t n = palette_slice.len / 4;
  if (n > 256) {
    n = 256;
  }
  size_t best_index = 0;
  uint64_t best_score = 0xFFFFFFFFFFFFFFFF;

  // Work in 16-bit color.
  uint32_t ca = 0x101 * (0xFF & (c >> 24));
  uint32_t cr = 0x101 * (0xFF & (c >> 16));
  uint32_t cg = 0x101 * (0xFF & (c >> 8));
  uint32_t cb = 0x101 * (0xFF & (c >> 0));

  switch (palette_format.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY: {
      bool nonpremul = palette_format.repr ==
                       WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL;

      size_t i;
      for (i = 0; i < n; i++) {
        // Work in 16-bit color.
        uint32_t pb = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 0]));
        uint32_t pg = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 1]));
        uint32_t pr = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 2]));
        uint32_t pa = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 3]));

        // Convert to premultiplied alpha.
        if (nonpremul && (pa != 0xFFFF)) {
          pb = (pb * pa) / 0xFFFF;
          pg = (pg * pa) / 0xFFFF;
          pr = (pr * pa) / 0xFFFF;
        }

        // These deltas are conceptually int32_t (signed) but after squaring,
        // it's equivalent to work in uint32_t (unsigned).
        pb -= cb;
        pg -= cg;
        pr -= cr;
        pa -= ca;
        uint64_t score = ((uint64_t)(pb * pb)) + ((uint64_t)(pg * pg)) +
                         ((uint64_t)(pr * pr)) + ((uint64_t)(pa * pa));
        if (best_score > score) {
          best_score = score;
          best_index = i;
        }
      }
      break;
    }
  }

  return (uint8_t)best_index;
}

// --------

static inline uint32_t  //
wuffs_base__composite_nonpremul_nonpremul_u32_axxx(uint32_t dst_nonpremul,
                                                   uint32_t src_nonpremul) {
  // Convert from 8-bit color to 16-bit color.
  uint32_t sa = 0x101 * (0xFF & (src_nonpremul >> 24));
  uint32_t sr = 0x101 * (0xFF & (src_nonpremul >> 16));
  uint32_t sg = 0x101 * (0xFF & (src_nonpremul >> 8));
  uint32_t sb = 0x101 * (0xFF & (src_nonpremul >> 0));
  uint32_t da = 0x101 * (0xFF & (dst_nonpremul >> 24));
  uint32_t dr = 0x101 * (0xFF & (dst_nonpremul >> 16));
  uint32_t dg = 0x101 * (0xFF & (dst_nonpremul >> 8));
  uint32_t db = 0x101 * (0xFF & (dst_nonpremul >> 0));

  // Convert dst from nonpremul to premul.
  dr = (dr * da) / 0xFFFF;
  dg = (dg * da) / 0xFFFF;
  db = (db * da) / 0xFFFF;

  // Calculate the inverse of the src-alpha: how much of the dst to keep.
  uint32_t ia = 0xFFFF - sa;

  // Composite src (nonpremul) over dst (premul).
  da = sa + ((da * ia) / 0xFFFF);
  dr = ((sr * sa) + (dr * ia)) / 0xFFFF;
  dg = ((sg * sa) + (dg * ia)) / 0xFFFF;
  db = ((sb * sa) + (db * ia)) / 0xFFFF;

  // Convert dst from premul to nonpremul.
  if (da != 0) {
    dr = (dr * 0xFFFF) / da;
    dg = (dg * 0xFFFF) / da;
    db = (db * 0xFFFF) / da;
  }

  // Convert from 16-bit color to 8-bit color and combine the components.
  da >>= 8;
  dr >>= 8;
  dg >>= 8;
  db >>= 8;
  return (db << 0) | (dg << 8) | (dr << 16) | (da << 24);
}

static inline uint32_t  //
wuffs_base__composite_nonpremul_premul_u32_axxx(uint32_t dst_nonpremul,
                                                uint32_t src_premul) {
  // Convert from 8-bit color to 16-bit color.
  uint32_t sa = 0x101 * (0xFF & (src_premul >> 24));
  uint32_t sr = 0x101 * (0xFF & (src_premul >> 16));
  uint32_t sg = 0x101 * (0xFF & (src_premul >> 8));
  uint32_t sb = 0x101 * (0xFF & (src_premul >> 0));
  uint32_t da = 0x101 * (0xFF & (dst_nonpremul >> 24));
  uint32_t dr = 0x101 * (0xFF & (dst_nonpremul >> 16));
  uint32_t dg = 0x101 * (0xFF & (dst_nonpremul >> 8));
  uint32_t db = 0x101 * (0xFF & (dst_nonpremul >> 0));

  // Convert dst from nonpremul to premul.
  dr = (dr * da) / 0xFFFF;
  dg = (dg * da) / 0xFFFF;
  db = (db * da) / 0xFFFF;

  // Calculate the inverse of the src-alpha: how much of the dst to keep.
  uint32_t ia = 0xFFFF - sa;

  // Composite src (premul) over dst (premul).
  da = sa + ((da * ia) / 0xFFFF);
  dr = sr + ((dr * ia) / 0xFFFF);
  dg = sg + ((dg * ia) / 0xFFFF);
  db = sb + ((db * ia) / 0xFFFF);

  // Convert dst from premul to nonpremul.
  if (da != 0) {
    dr = (dr * 0xFFFF) / da;
    dg = (dg * 0xFFFF) / da;
    db = (db * 0xFFFF) / da;
  }

  // Convert from 16-bit color to 8-bit color and combine the components.
  da >>= 8;
  dr >>= 8;
  dg >>= 8;
  db >>= 8;
  return (db << 0) | (dg << 8) | (dr << 16) | (da << 24);
}

static inline uint32_t  //
wuffs_base__composite_premul_nonpremul_u32_axxx(uint32_t dst_premul,
                                                uint32_t src_nonpremul) {
  // Convert from 8-bit color to 16-bit color.
  uint32_t sa = 0x101 * (0xFF & (src_nonpremul >> 24));
  uint32_t sr = 0x101 * (0xFF & (src_nonpremul >> 16));
  uint32_t sg = 0x101 * (0xFF & (src_nonpremul >> 8));
  uint32_t sb = 0x101 * (0xFF & (src_nonpremul >> 0));
  uint32_t da = 0x101 * (0xFF & (dst_premul >> 24));
  uint32_t dr = 0x101 * (0xFF & (dst_premul >> 16));
  uint32_t dg = 0x101 * (0xFF & (dst_premul >> 8));
  uint32_t db = 0x101 * (0xFF & (dst_premul >> 0));

  // Calculate the inverse of the src-alpha: how much of the dst to keep.
  uint32_t ia = 0xFFFF - sa;

  // Composite src (nonpremul) over dst (premul).
  da = sa + ((da * ia) / 0xFFFF);
  dr = ((sr * sa) + (dr * ia)) / 0xFFFF;
  dg = ((sg * sa) + (dg * ia)) / 0xFFFF;
  db = ((sb * sa) + (db * ia)) / 0xFFFF;

  // Convert from 16-bit color to 8-bit color and combine the components.
  da >>= 8;
  dr >>= 8;
  dg >>= 8;
  db >>= 8;
  return (db << 0) | (dg << 8) | (dr << 16) | (da << 24);
}

static inline uint32_t  //
wuffs_base__composite_premul_premul_u32_axxx(uint32_t dst_premul,
                                             uint32_t src_premul) {
  // Convert from 8-bit color to 16-bit color.
  uint32_t sa = 0x101 * (0xFF & (src_premul >> 24));
  uint32_t sr = 0x101 * (0xFF & (src_premul >> 16));
  uint32_t sg = 0x101 * (0xFF & (src_premul >> 8));
  uint32_t sb = 0x101 * (0xFF & (src_premul >> 0));
  uint32_t da = 0x101 * (0xFF & (dst_premul >> 24));
  uint32_t dr = 0x101 * (0xFF & (dst_premul >> 16));
  uint32_t dg = 0x101 * (0xFF & (dst_premul >> 8));
  uint32_t db = 0x101 * (0xFF & (dst_premul >> 0));

  // Calculate the inverse of the src-alpha: how much of the dst to keep.
  uint32_t ia = 0xFFFF - sa;

  // Composite src (premul) over dst (premul).
  da = sa + ((da * ia) / 0xFFFF);
  dr = sr + ((dr * ia) / 0xFFFF);
  dg = sg + ((dg * ia) / 0xFFFF);
  db = sb + ((db * ia) / 0xFFFF);

  // Convert from 16-bit color to 8-bit color and combine the components.
  da >>= 8;
  dr >>= 8;
  dg >>= 8;
  db >>= 8;
  return (db << 0) | (dg << 8) | (dr << 16) | (da << 24);
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__squash_bgr_565_888(wuffs_base__slice_u8 dst,
                                               wuffs_base__slice_u8 src) {
  size_t len4 = (dst.len < src.len ? dst.len : src.len) / 4;
  uint8_t* d = dst.ptr;
  const uint8_t* s = src.ptr;

  size_t n = len4;
  while (n--) {
    uint32_t argb = wuffs_base__load_u32le__no_bounds_check(s);
    uint32_t b5 = 0x1F & (argb >> (8 - 5));
    uint32_t g6 = 0x3F & (argb >> (16 - 6));
    uint32_t r5 = 0x1F & (argb >> (24 - 5));
    uint32_t alpha = argb & 0xFF000000;
    wuffs_base__store_u32le__no_bounds_check(
        d, alpha | (r5 << 11) | (g6 << 5) | (b5 << 0));
    s += 4;
    d += 4;
  }
  return len4 * 4;
}

static uint64_t  //
wuffs_base__pixel_swizzler__swap_rgbx_bgrx(wuffs_base__slice_u8 dst,
                                           wuffs_base__slice_u8 src) {
  size_t len4 = (dst.len < src.len ? dst.len : src.len) / 4;
  uint8_t* d = dst.ptr;
  const uint8_t* s = src.ptr;

  size_t n = len4;
  while (n--) {
    uint8_t b0 = s[0];
    uint8_t b1 = s[1];
    uint8_t b2 = s[2];
    uint8_t b3 = s[3];
    d[0] = b2;
    d[1] = b1;
    d[2] = b0;
    d[3] = b3;
    s += 4;
    d += 4;
  }
  return len4 * 4;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__copy_1_1(uint8_t* dst_ptr,
                                     size_t dst_len,
                                     uint8_t* dst_palette_ptr,
                                     size_t dst_palette_len,
                                     const uint8_t* src_ptr,
                                     size_t src_len) {
  size_t len = (dst_len < src_len) ? dst_len : src_len;
  if (len > 0) {
    memmove(dst_ptr, src_ptr, len);
  }
  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__copy_3_3(uint8_t* dst_ptr,
                                     size_t dst_len,
                                     uint8_t* dst_palette_ptr,
                                     size_t dst_palette_len,
                                     const uint8_t* src_ptr,
                                     size_t src_len) {
  size_t dst_len3 = dst_len / 3;
  size_t src_len3 = src_len / 3;
  size_t len = (dst_len3 < src_len3) ? dst_len3 : src_len3;
  if (len > 0) {
    memmove(dst_ptr, src_ptr, len * 3);
  }
  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__copy_4_4(uint8_t* dst_ptr,
                                     size_t dst_len,
                                     uint8_t* dst_palette_ptr,
                                     size_t dst_palette_len,
                                     const uint8_t* src_ptr,
                                     size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len4 < src_len4) ? dst_len4 : src_len4;
  if (len > 0) {
    memmove(dst_ptr, src_ptr, len * 4);
  }
  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__bgr(uint8_t* dst_ptr,
                                         size_t dst_len,
                                         uint8_t* dst_palette_ptr,
                                         size_t dst_palette_len,
                                         const uint8_t* src_ptr,
                                         size_t src_len) {
  size_t dst_len2 = dst_len / 2;
  size_t src_len3 = src_len / 3;
  size_t len = (dst_len2 < src_len3) ? dst_len2 : src_len3;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t b5 = s[0] >> 3;
    uint32_t g6 = s[1] >> 2;
    uint32_t r5 = s[2] >> 3;
    uint32_t rgb_565 = (r5 << 11) | (g6 << 5) | (b5 << 0);
    wuffs_base__store_u16le__no_bounds_check(d + (0 * 2), (uint16_t)rgb_565);

    s += 1 * 3;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__bgra_nonpremul__src(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len2 = dst_len / 2;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len2 < src_len4) ? dst_len2 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    wuffs_base__store_u16le__no_bounds_check(
        d + (0 * 2),
        wuffs_base__color_u32_argb_premul__as__color_u16_rgb_565(
            wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
                wuffs_base__load_u32le__no_bounds_check(s + (0 * 4)))));

    s += 1 * 4;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__bgra_nonpremul__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len2 = dst_len / 2;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len2 < src_len4) ? dst_len2 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    // Convert from 8-bit color to 16-bit color.
    uint32_t sa = 0x101 * ((uint32_t)s[3]);
    uint32_t sr = 0x101 * ((uint32_t)s[2]);
    uint32_t sg = 0x101 * ((uint32_t)s[1]);
    uint32_t sb = 0x101 * ((uint32_t)s[0]);

    // Convert from 565 color to 16-bit color.
    uint32_t old_rgb_565 = wuffs_base__load_u16le__no_bounds_check(d + (0 * 2));
    uint32_t old_r5 = 0x1F & (old_rgb_565 >> 11);
    uint32_t dr = (0x8421 * old_r5) >> 4;
    uint32_t old_g6 = 0x3F & (old_rgb_565 >> 5);
    uint32_t dg = (0x1041 * old_g6) >> 2;
    uint32_t old_b5 = 0x1F & (old_rgb_565 >> 0);
    uint32_t db = (0x8421 * old_b5) >> 4;

    // Calculate the inverse of the src-alpha: how much of the dst to keep.
    uint32_t ia = 0xFFFF - sa;

    // Composite src (nonpremul) over dst (premul).
    dr = ((sr * sa) + (dr * ia)) / 0xFFFF;
    dg = ((sg * sa) + (dg * ia)) / 0xFFFF;
    db = ((sb * sa) + (db * ia)) / 0xFFFF;

    // Convert from 16-bit color to 565 color and combine the components.
    uint32_t new_r5 = 0x1F & (dr >> 11);
    uint32_t new_g6 = 0x3F & (dg >> 10);
    uint32_t new_b5 = 0x1F & (db >> 11);
    uint32_t new_rgb_565 = (new_r5 << 11) | (new_g6 << 5) | (new_b5 << 0);
    wuffs_base__store_u16le__no_bounds_check(d + (0 * 2),
                                             (uint16_t)new_rgb_565);

    s += 1 * 4;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__y(uint8_t* dst_ptr,
                                       size_t dst_len,
                                       uint8_t* dst_palette_ptr,
                                       size_t dst_palette_len,
                                       const uint8_t* src_ptr,
                                       size_t src_len) {
  size_t dst_len2 = dst_len / 2;
  size_t len = (dst_len2 < src_len) ? dst_len2 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t y5 = s[0] >> 3;
    uint32_t y6 = s[0] >> 2;
    uint32_t rgb_565 = (y5 << 11) | (y6 << 5) | (y5 << 0);
    wuffs_base__store_u16le__no_bounds_check(d + (0 * 2), (uint16_t)rgb_565);

    s += 1 * 1;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__index__src(uint8_t* dst_ptr,
                                                size_t dst_len,
                                                uint8_t* dst_palette_ptr,
                                                size_t dst_palette_len,
                                                const uint8_t* src_ptr,
                                                size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len2 = dst_len / 2;
  size_t len = (dst_len2 < src_len) ? dst_len2 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    wuffs_base__store_u16le__no_bounds_check(
        d + (0 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (1 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (2 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (3 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 2;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    wuffs_base__store_u16le__no_bounds_check(
        d + (0 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[0] * 4)));

    s += 1 * 1;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__index_binary_alpha__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len2 = dst_len / 2;
  size_t len = (dst_len2 < src_len) ? dst_len2 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u16le__no_bounds_check(d + (0 * 2), (uint16_t)s0);
    }

    s += 1 * 1;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__bgr__bgra_nonpremul__src(uint8_t* dst_ptr,
                                                     size_t dst_len,
                                                     uint8_t* dst_palette_ptr,
                                                     size_t dst_palette_len,
                                                     const uint8_t* src_ptr,
                                                     size_t src_len) {
  size_t dst_len3 = dst_len / 3;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len3 < src_len4) ? dst_len3 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t s0 =
        wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
            wuffs_base__load_u32le__no_bounds_check(s + (0 * 4)));
    wuffs_base__store_u24le__no_bounds_check(d + (0 * 3), s0);

    s += 1 * 4;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr__bgra_nonpremul__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len3 = dst_len / 3;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len3 < src_len4) ? dst_len3 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    // Convert from 8-bit color to 16-bit color.
    uint32_t sa = 0x101 * ((uint32_t)s[3]);
    uint32_t sr = 0x101 * ((uint32_t)s[2]);
    uint32_t sg = 0x101 * ((uint32_t)s[1]);
    uint32_t sb = 0x101 * ((uint32_t)s[0]);
    uint32_t dr = 0x101 * ((uint32_t)d[2]);
    uint32_t dg = 0x101 * ((uint32_t)d[1]);
    uint32_t db = 0x101 * ((uint32_t)d[0]);

    // Calculate the inverse of the src-alpha: how much of the dst to keep.
    uint32_t ia = 0xFFFF - sa;

    // Composite src (nonpremul) over dst (premul).
    dr = ((sr * sa) + (dr * ia)) / 0xFFFF;
    dg = ((sg * sa) + (dg * ia)) / 0xFFFF;
    db = ((sb * sa) + (db * ia)) / 0xFFFF;

    // Convert from 16-bit color to 8-bit color.
    d[0] = (uint8_t)(db >> 8);
    d[1] = (uint8_t)(dg >> 8);
    d[2] = (uint8_t)(dr >> 8);

    s += 1 * 4;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__bgra_nonpremul__bgra_nonpremul__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len4 < src_len4) ? dst_len4 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t d0 = wuffs_base__load_u32le__no_bounds_check(d + (0 * 4));
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(s + (0 * 4));
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4),
        wuffs_base__composite_nonpremul_nonpremul_u32_axxx(d0, s0));

    s += 1 * 4;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len4 < src_len4) ? dst_len4 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(s + (0 * 4));
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4),
        wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(s0));

    s += 1 * 4;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len4 < src_len4) ? dst_len4 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t d0 = wuffs_base__load_u32le__no_bounds_check(d + (0 * 4));
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(s + (0 * 4));
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), wuffs_base__composite_premul_nonpremul_u32_axxx(d0, s0));

    s += 1 * 4;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__xxx__index__src(uint8_t* dst_ptr,
                                            size_t dst_len,
                                            uint8_t* dst_palette_ptr,
                                            size_t dst_palette_len,
                                            const uint8_t* src_ptr,
                                            size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len3 = dst_len / 3;
  size_t len = (dst_len3 < src_len) ? dst_len3 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  // The comparison in the while condition is ">", not ">=", because with
  // ">=", the last 4-byte store could write past the end of the dst slice.
  //
  // Each 4-byte store writes one too many bytes, but a subsequent store
  // will overwrite that with the correct byte. There is always another
  // store, whether a 4-byte store in this loop or a 1-byte store in the
  // next loop.
  while (n > loop_unroll_count) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (1 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (2 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (3 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 3;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    wuffs_base__store_u24le__no_bounds_check(d + (0 * 3), s0);

    s += 1 * 1;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxx__index_binary_alpha__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len3 = dst_len / 3;
  size_t len = (dst_len3 < src_len) ? dst_len3 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u24le__no_bounds_check(d + (0 * 3), s0);
    }
    uint32_t s1 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[1] * 4));
    if (s1) {
      wuffs_base__store_u24le__no_bounds_check(d + (1 * 3), s1);
    }
    uint32_t s2 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[2] * 4));
    if (s2) {
      wuffs_base__store_u24le__no_bounds_check(d + (2 * 3), s2);
    }
    uint32_t s3 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[3] * 4));
    if (s3) {
      wuffs_base__store_u24le__no_bounds_check(d + (3 * 3), s3);
    }

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 3;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u24le__no_bounds_check(d + (0 * 3), s0);
    }

    s += 1 * 1;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxx__y(uint8_t* dst_ptr,
                                   size_t dst_len,
                                   uint8_t* dst_palette_ptr,
                                   size_t dst_palette_len,
                                   const uint8_t* src_ptr,
                                   size_t src_len) {
  size_t dst_len3 = dst_len / 3;
  size_t len = (dst_len3 < src_len) ? dst_len3 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint8_t s0 = s[0];
    d[0] = s0;
    d[1] = s0;
    d[2] = s0;

    s += 1 * 1;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__index__src(uint8_t* dst_ptr,
                                             size_t dst_len,
                                             uint8_t* dst_palette_ptr,
                                             size_t dst_palette_len,
                                             const uint8_t* src_ptr,
                                             size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len4 = dst_len / 4;
  size_t len = (dst_len4 < src_len) ? dst_len4 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (1 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (2 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (3 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 4;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[0] * 4)));

    s += 1 * 1;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__index_binary_alpha__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len4 = dst_len / 4;
  size_t len = (dst_len4 < src_len) ? dst_len4 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u32le__no_bounds_check(d + (0 * 4), s0);
    }
    uint32_t s1 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[1] * 4));
    if (s1) {
      wuffs_base__store_u32le__no_bounds_check(d + (1 * 4), s1);
    }
    uint32_t s2 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[2] * 4));
    if (s2) {
      wuffs_base__store_u32le__no_bounds_check(d + (2 * 4), s2);
    }
    uint32_t s3 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[3] * 4));
    if (s3) {
      wuffs_base__store_u32le__no_bounds_check(d + (3 * 4), s3);
    }

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 4;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u32le__no_bounds_check(d + (0 * 4), s0);
    }

    s += 1 * 1;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__xxx(uint8_t* dst_ptr,
                                      size_t dst_len,
                                      uint8_t* dst_palette_ptr,
                                      size_t dst_palette_len,
                                      const uint8_t* src_ptr,
                                      size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t src_len3 = src_len / 3;
  size_t len = (dst_len4 < src_len3) ? dst_len4 : src_len3;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4),
        0xFF000000 | wuffs_base__load_u24le__no_bounds_check(s + (0 * 3)));

    s += 1 * 3;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__y(uint8_t* dst_ptr,
                                    size_t dst_len,
                                    uint8_t* dst_palette_ptr,
                                    size_t dst_palette_len,
                                    const uint8_t* src_ptr,
                                    size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t len = (dst_len4 < src_len) ? dst_len4 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), 0xFF000000 | (0x010101 * (uint32_t)s[0]));

    s += 1 * 1;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

// --------

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__y(wuffs_base__pixel_swizzler* p,
                                       wuffs_base__pixel_format dst_pixfmt,
                                       wuffs_base__slice_u8 dst_palette,
                                       wuffs_base__slice_u8 src_palette,
                                       wuffs_base__pixel_blend blend) {
  switch (dst_pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      return wuffs_base__pixel_swizzler__bgr_565__y;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      return wuffs_base__pixel_swizzler__xxx__y;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      return wuffs_base__pixel_swizzler__xxxx__y;
  }
  return NULL;
}

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__indexed__bgra_binary(
    wuffs_base__pixel_swizzler* p,
    wuffs_base__pixel_format dst_pixfmt,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src_palette,
    wuffs_base__pixel_blend blend) {
  switch (dst_pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
      if (wuffs_base__slice_u8__copy_from_slice(dst_palette, src_palette) !=
          1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__copy_1_1;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      if (wuffs_base__pixel_swizzler__squash_bgr_565_888(dst_palette,
                                                         src_palette) != 1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__bgr_565__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__bgr_565__index_binary_alpha__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      if (wuffs_base__slice_u8__copy_from_slice(dst_palette, src_palette) !=
          1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxx__index_binary_alpha__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
      if (wuffs_base__slice_u8__copy_from_slice(dst_palette, src_palette) !=
          1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxxx__index_binary_alpha__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      if (wuffs_base__pixel_swizzler__swap_rgbx_bgrx(dst_palette,
                                                     src_palette) != 1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxx__index_binary_alpha__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
      if (wuffs_base__pixel_swizzler__swap_rgbx_bgrx(dst_palette,
                                                     src_palette) != 1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxxx__index_binary_alpha__src_over;
      }
      return NULL;
  }
  return NULL;
}

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__bgr(wuffs_base__pixel_swizzler* p,
                                         wuffs_base__pixel_format dst_pixfmt,
                                         wuffs_base__slice_u8 dst_palette,
                                         wuffs_base__slice_u8 src_palette,
                                         wuffs_base__pixel_blend blend) {
  switch (dst_pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      return wuffs_base__pixel_swizzler__bgr_565__bgr;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      return wuffs_base__pixel_swizzler__copy_3_3;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      return wuffs_base__pixel_swizzler__xxxx__xxx;

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      // TODO.
      break;
  }
  return NULL;
}

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__bgra_nonpremul(
    wuffs_base__pixel_swizzler* p,
    wuffs_base__pixel_format dst_pixfmt,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src_palette,
    wuffs_base__pixel_blend blend) {
  switch (dst_pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__bgr_565__bgra_nonpremul__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__bgr_565__bgra_nonpremul__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__bgr__bgra_nonpremul__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__bgr__bgra_nonpremul__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__copy_4_4;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__bgra_nonpremul__bgra_nonpremul__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      // TODO.
      break;

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      // TODO.
      break;
  }
  return NULL;
}

// --------

WUFFS_BASE__MAYBE_STATIC wuffs_base__status  //
wuffs_base__pixel_swizzler__prepare(wuffs_base__pixel_swizzler* p,
                                    wuffs_base__pixel_format dst_pixfmt,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__pixel_format src_pixfmt,
                                    wuffs_base__slice_u8 src_palette,
                                    wuffs_base__pixel_blend blend) {
  if (!p) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  p->private_impl.func = NULL;
  p->private_impl.src_pixfmt_bytes_per_pixel = 0;

  wuffs_base__pixel_swizzler__func func = NULL;
  uint32_t src_pixfmt_bits_per_pixel =
      wuffs_base__pixel_format__bits_per_pixel(&src_pixfmt);
  if ((src_pixfmt_bits_per_pixel == 0) ||
      ((src_pixfmt_bits_per_pixel & 7) != 0)) {
    return wuffs_base__make_status(
        wuffs_base__error__unsupported_pixel_swizzler_option);
  }

  // TODO: support many more formats.

  switch (src_pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__Y:
      func = wuffs_base__pixel_swizzler__prepare__y(p, dst_pixfmt, dst_palette,
                                                    src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
      func = wuffs_base__pixel_swizzler__prepare__indexed__bgra_binary(
          p, dst_pixfmt, dst_palette, src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      func = wuffs_base__pixel_swizzler__prepare__bgr(
          p, dst_pixfmt, dst_palette, src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      func = wuffs_base__pixel_swizzler__prepare__bgra_nonpremul(
          p, dst_pixfmt, dst_palette, src_palette, blend);
      break;
  }

  p->private_impl.func = func;
  p->private_impl.src_pixfmt_bytes_per_pixel = src_pixfmt_bits_per_pixel / 8;
  return wuffs_base__make_status(
      func ? NULL : wuffs_base__error__unsupported_pixel_swizzler_option);
}

WUFFS_BASE__MAYBE_STATIC uint64_t  //
wuffs_base__pixel_swizzler__swizzle_interleaved_from_reader(
    const wuffs_base__pixel_swizzler* p,
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    const uint8_t** ptr_iop_r,
    const uint8_t* io2_r) {
  if (p && p->private_impl.func) {
    const uint8_t* iop_r = *ptr_iop_r;
    uint64_t n = (*p->private_impl.func)(dst.ptr, dst.len, dst_palette.ptr,
                                         dst_palette.len, iop_r,
                                         (size_t)(io2_r - iop_r));
    *ptr_iop_r += n * p->private_impl.src_pixfmt_bytes_per_pixel;
    return n;
  }
  return 0;
}

WUFFS_BASE__MAYBE_STATIC uint64_t  //
wuffs_base__pixel_swizzler__swizzle_interleaved_from_slice(
    const wuffs_base__pixel_swizzler* p,
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  if (p && p->private_impl.func) {
    return (*p->private_impl.func)(dst.ptr, dst.len, dst_palette.ptr,
                                   dst_palette.len, src.ptr, src.len);
  }
  return 0;
}

#endif  // !defined(WUFFS_CONFIG__MODULES) ||
        // defined(WUFFS_CONFIG__MODULE__BASE) ||
        // defined(WUFFS_CONFIG__MODULE__BASE__PIXCONV)

#ifdef __cplusplus
}  // extern "C"
#endif

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__ADLER32)

// ---------------- Status Codes Implementations

// ---------------- Private Consts

// ---------------- Private Initializer Prototypes

// ---------------- Private Function Prototypes

// ---------------- VTables

const wuffs_base__hasher_u32__func_ptrs
wuffs_adler32__hasher__func_ptrs_for__wuffs_base__hasher_u32 = {
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_adler32__hasher__set_quirk_enabled),
  (uint32_t(*)(void*,
      wuffs_base__slice_u8))(&wuffs_adler32__hasher__update_u32),
};

// ---------------- Initializer Implementations

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_adler32__hasher__initialize(
    wuffs_adler32__hasher* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags){
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (sizeof(*self) != sizeof_star_self) {
    return wuffs_base__make_status(wuffs_base__error__bad_sizeof_receiver);
  }
  if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) ||
      (((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {
    return wuffs_base__make_status(wuffs_base__error__bad_wuffs_version);
  }

  if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {
    // The whole point of this if-check is to detect an uninitialized *self.
    // We disable the warning on GCC. Clang-5.0 does not have this warning.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if (self->private_impl.magic != 0) {
      return wuffs_base__make_status(wuffs_base__error__initialize_falsely_claimed_already_zeroed);
    }
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  } else {
    if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {
      memset(self, 0, sizeof(*self));
      initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;
    } else {
      memset(&(self->private_impl), 0, sizeof(self->private_impl));
    }
  }

  self->private_impl.magic = WUFFS_BASE__MAGIC;
  self->private_impl.vtable_for__wuffs_base__hasher_u32.vtable_name =
      wuffs_base__hasher_u32__vtable_name;
  self->private_impl.vtable_for__wuffs_base__hasher_u32.function_pointers =
      (const void*)(&wuffs_adler32__hasher__func_ptrs_for__wuffs_base__hasher_u32);
  return wuffs_base__make_status(NULL);
}

wuffs_adler32__hasher*
wuffs_adler32__hasher__alloc() {
  wuffs_adler32__hasher* x =
      (wuffs_adler32__hasher*)(calloc(sizeof(wuffs_adler32__hasher), 1));
  if (!x) {
    return NULL;
  }
  if (wuffs_adler32__hasher__initialize(
      x, sizeof(wuffs_adler32__hasher), WUFFS_VERSION, WUFFS_INITIALIZE__ALREADY_ZEROED).repr) {
    free(x);
    return NULL;
  }
  return x;
}

size_t
sizeof__wuffs_adler32__hasher() {
  return sizeof(wuffs_adler32__hasher);
}

// ---------------- Function Implementations

// -------- func adler32.hasher.set_quirk_enabled

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_adler32__hasher__set_quirk_enabled(
    wuffs_adler32__hasher* self,
    uint32_t a_quirk,
    bool a_enabled) {
  return wuffs_base__make_empty_struct();
}

// -------- func adler32.hasher.update_u32

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_adler32__hasher__update_u32(
    wuffs_adler32__hasher* self,
    wuffs_base__slice_u8 a_x) {
  if (!self) {
    return 0;
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return 0;
  }

  uint32_t v_s1 = 0;
  uint32_t v_s2 = 0;
  wuffs_base__slice_u8 v_remaining = {0};
  wuffs_base__slice_u8 v_p = {0};

  if ( ! self->private_impl.f_started) {
    self->private_impl.f_started = true;
    self->private_impl.f_state = 1;
  }
  v_s1 = ((self->private_impl.f_state) & 0xFFFF);
  v_s2 = ((self->private_impl.f_state) >> (32 - (16)));
  while (((uint64_t)(a_x.len)) > 0) {
    v_remaining = wuffs_base__slice_u8__subslice_j(a_x, 0);
    if (((uint64_t)(a_x.len)) > 5552) {
      v_remaining = wuffs_base__slice_u8__subslice_i(a_x, 5552);
      a_x = wuffs_base__slice_u8__subslice_j(a_x, 5552);
    }
    {
      wuffs_base__slice_u8 i_slice_p = a_x;
      v_p = i_slice_p;
      v_p.len = 1;
      uint8_t* i_end0_p = i_slice_p.ptr + (i_slice_p.len / 8) * 8;
      while (v_p.ptr < i_end0_p) {
        v_s1 += ((uint32_t)(v_p.ptr[0]));
        v_s2 += v_s1;
        v_p.ptr += 1;
        v_s1 += ((uint32_t)(v_p.ptr[0]));
        v_s2 += v_s1;
        v_p.ptr += 1;
        v_s1 += ((uint32_t)(v_p.ptr[0]));
        v_s2 += v_s1;
        v_p.ptr += 1;
        v_s1 += ((uint32_t)(v_p.ptr[0]));
        v_s2 += v_s1;
        v_p.ptr += 1;
        v_s1 += ((uint32_t)(v_p.ptr[0]));
        v_s2 += v_s1;
        v_p.ptr += 1;
        v_s1 += ((uint32_t)(v_p.ptr[0]));
        v_s2 += v_s1;
        v_p.ptr += 1;
        v_s1 += ((uint32_t)(v_p.ptr[0]));
        v_s2 += v_s1;
        v_p.ptr += 1;
        v_s1 += ((uint32_t)(v_p.ptr[0]));
        v_s2 += v_s1;
        v_p.ptr += 1;
      }
      v_p.len = 1;
      uint8_t* i_end1_p = i_slice_p.ptr + (i_slice_p.len / 1) * 1;
      while (v_p.ptr < i_end1_p) {
        v_s1 += ((uint32_t)(v_p.ptr[0]));
        v_s2 += v_s1;
        v_p.ptr += 1;
      }
    }
    v_s1 %= 65521;
    v_s2 %= 65521;
    a_x = v_remaining;
  }
  self->private_impl.f_state = (((v_s2 & 65535) << 16) | (v_s1 & 65535));
  return self->private_impl.f_state;
}

#endif  // !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__ADLER32)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__BMP)

// ---------------- Status Codes Implementations

const char* wuffs_bmp__error__bad_header = "#bmp: bad header";
const char* wuffs_bmp__error__unsupported_bmp_file = "#bmp: unsupported BMP file";
const char* wuffs_bmp__note__internal_note_short_read = "@bmp: internal note: short read";

// ---------------- Private Consts

// ---------------- Private Initializer Prototypes

// ---------------- Private Function Prototypes

static wuffs_base__status
wuffs_bmp__decoder__swizzle(
    wuffs_bmp__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_bmp__decoder__skip_frame(
    wuffs_bmp__decoder* self,
    wuffs_base__io_buffer* a_src);

// ---------------- VTables

const wuffs_base__image_decoder__func_ptrs
wuffs_bmp__decoder__func_ptrs_for__wuffs_base__image_decoder = {
  (wuffs_base__status(*)(void*,
      wuffs_base__pixel_buffer*,
      wuffs_base__io_buffer*,
      wuffs_base__pixel_blend,
      wuffs_base__slice_u8,
      wuffs_base__decode_frame_options*))(&wuffs_bmp__decoder__decode_frame),
  (wuffs_base__status(*)(void*,
      wuffs_base__frame_config*,
      wuffs_base__io_buffer*))(&wuffs_bmp__decoder__decode_frame_config),
  (wuffs_base__status(*)(void*,
      wuffs_base__image_config*,
      wuffs_base__io_buffer*))(&wuffs_bmp__decoder__decode_image_config),
  (wuffs_base__rect_ie_u32(*)(const void*))(&wuffs_bmp__decoder__frame_dirty_rect),
  (uint32_t(*)(const void*))(&wuffs_bmp__decoder__num_animation_loops),
  (uint64_t(*)(const void*))(&wuffs_bmp__decoder__num_decoded_frame_configs),
  (uint64_t(*)(const void*))(&wuffs_bmp__decoder__num_decoded_frames),
  (wuffs_base__status(*)(void*,
      uint64_t,
      uint64_t))(&wuffs_bmp__decoder__restart_frame),
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_bmp__decoder__set_quirk_enabled),
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_bmp__decoder__set_report_metadata),
  (wuffs_base__status(*)(void*,
      wuffs_base__io_buffer*,
      wuffs_base__more_information*,
      wuffs_base__io_buffer*))(&wuffs_bmp__decoder__tell_me_more),
  (wuffs_base__range_ii_u64(*)(const void*))(&wuffs_bmp__decoder__workbuf_len),
};

// ---------------- Initializer Implementations

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_bmp__decoder__initialize(
    wuffs_bmp__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags){
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (sizeof(*self) != sizeof_star_self) {
    return wuffs_base__make_status(wuffs_base__error__bad_sizeof_receiver);
  }
  if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) ||
      (((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {
    return wuffs_base__make_status(wuffs_base__error__bad_wuffs_version);
  }

  if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {
    // The whole point of this if-check is to detect an uninitialized *self.
    // We disable the warning on GCC. Clang-5.0 does not have this warning.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if (self->private_impl.magic != 0) {
      return wuffs_base__make_status(wuffs_base__error__initialize_falsely_claimed_already_zeroed);
    }
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  } else {
    if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {
      memset(self, 0, sizeof(*self));
      initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;
    } else {
      memset(&(self->private_impl), 0, sizeof(self->private_impl));
    }
  }

  self->private_impl.magic = WUFFS_BASE__MAGIC;
  self->private_impl.vtable_for__wuffs_base__image_decoder.vtable_name =
      wuffs_base__image_decoder__vtable_name;
  self->private_impl.vtable_for__wuffs_base__image_decoder.function_pointers =
      (const void*)(&wuffs_bmp__decoder__func_ptrs_for__wuffs_base__image_decoder);
  return wuffs_base__make_status(NULL);
}

wuffs_bmp__decoder*
wuffs_bmp__decoder__alloc() {
  wuffs_bmp__decoder* x =
      (wuffs_bmp__decoder*)(calloc(sizeof(wuffs_bmp__decoder), 1));
  if (!x) {
    return NULL;
  }
  if (wuffs_bmp__decoder__initialize(
      x, sizeof(wuffs_bmp__decoder), WUFFS_VERSION, WUFFS_INITIALIZE__ALREADY_ZEROED).repr) {
    free(x);
    return NULL;
  }
  return x;
}

size_t
sizeof__wuffs_bmp__decoder() {
  return sizeof(wuffs_bmp__decoder);
}

// ---------------- Function Implementations

// -------- func bmp.decoder.set_quirk_enabled

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_bmp__decoder__set_quirk_enabled(
    wuffs_bmp__decoder* self,
    uint32_t a_quirk,
    bool a_enabled) {
  return wuffs_base__make_empty_struct();
}

// -------- func bmp.decoder.decode_image_config

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_bmp__decoder__decode_image_config(
    wuffs_bmp__decoder* self,
    wuffs_base__image_config* a_dst,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 1)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t v_magic = 0;
  uint32_t v_bitmap_info_len = 0;
  uint32_t v_width = 0;
  uint32_t v_height = 0;
  uint32_t v_planes = 0;
  uint32_t v_bits_per_pixel = 0;
  uint32_t v_compression = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_image_config[0];
  if (coro_susp_point) {
    v_bitmap_info_len = self->private_data.s_decode_image_config[0].v_bitmap_info_len;
    v_bits_per_pixel = self->private_data.s_decode_image_config[0].v_bits_per_pixel;
    v_compression = self->private_data.s_decode_image_config[0].v_compression;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if ((self->private_impl.f_call_sequence != 0) || (self->private_impl.f_io_redirect_fourcc == 1)) {
      status = wuffs_base__make_status(wuffs_base__error__bad_call_sequence);
      goto exit;
    } else if (self->private_impl.f_io_redirect_fourcc != 0) {
      status = wuffs_base__make_status(wuffs_base__note__i_o_redirect);
      goto ok;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      uint32_t t_0;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_0 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_image_config[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
          uint32_t num_bits_0 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_0;
          if (num_bits_0 == 8) {
            t_0 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_0 += 8;
          *scratch |= ((uint64_t)(num_bits_0)) << 56;
        }
      }
      v_magic = t_0;
    }
    if (v_magic != 19778) {
      status = wuffs_base__make_status(wuffs_bmp__error__bad_header);
      goto exit;
    }
    self->private_data.s_decode_image_config[0].scratch = 8;
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
    if (self->private_data.s_decode_image_config[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
      self->private_data.s_decode_image_config[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
      iop_a_src = io2_a_src;
      status = wuffs_base__make_status(wuffs_base__suspension__short_read);
      goto suspend;
    }
    iop_a_src += self->private_data.s_decode_image_config[0].scratch;
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
      uint32_t t_1;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
        t_1 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
        iop_a_src += 4;
      } else {
        self->private_data.s_decode_image_config[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(5);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
          uint32_t num_bits_1 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_1;
          if (num_bits_1 == 24) {
            t_1 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_1 += 8;
          *scratch |= ((uint64_t)(num_bits_1)) << 56;
        }
      }
      self->private_impl.f_padding = t_1;
    }
    if (self->private_impl.f_padding < 14) {
      status = wuffs_base__make_status(wuffs_bmp__error__bad_header);
      goto exit;
    }
    self->private_impl.f_padding -= 14;
    self->private_impl.f_io_redirect_pos = wuffs_base__u64__sat_add(((uint64_t)(self->private_impl.f_padding)), wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src))));
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
      uint32_t t_2;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
        t_2 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
        iop_a_src += 4;
      } else {
        self->private_data.s_decode_image_config[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(7);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
          uint32_t num_bits_2 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_2;
          if (num_bits_2 == 24) {
            t_2 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_2 += 8;
          *scratch |= ((uint64_t)(num_bits_2)) << 56;
        }
      }
      v_bitmap_info_len = t_2;
    }
    if ((v_bitmap_info_len != 40) && (v_bitmap_info_len != 108) && (v_bitmap_info_len != 124)) {
      status = wuffs_base__make_status(wuffs_bmp__error__unsupported_bmp_file);
      goto exit;
    }
    if (self->private_impl.f_padding < v_bitmap_info_len) {
      status = wuffs_base__make_status(wuffs_bmp__error__bad_header);
      goto exit;
    }
    self->private_impl.f_padding -= v_bitmap_info_len;
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(8);
      uint32_t t_3;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
        t_3 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
        iop_a_src += 4;
      } else {
        self->private_data.s_decode_image_config[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(9);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
          uint32_t num_bits_3 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_3;
          if (num_bits_3 == 24) {
            t_3 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_3 += 8;
          *scratch |= ((uint64_t)(num_bits_3)) << 56;
        }
      }
      v_width = t_3;
    }
    if (v_width >= 2147483648) {
      status = wuffs_base__make_status(wuffs_bmp__error__bad_header);
      goto exit;
    }
    self->private_impl.f_width = v_width;
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(10);
      uint32_t t_4;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
        t_4 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
        iop_a_src += 4;
      } else {
        self->private_data.s_decode_image_config[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(11);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
          uint32_t num_bits_4 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_4;
          if (num_bits_4 == 24) {
            t_4 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_4 += 8;
          *scratch |= ((uint64_t)(num_bits_4)) << 56;
        }
      }
      v_height = t_4;
    }
    if (v_height == 2147483648) {
      status = wuffs_base__make_status(wuffs_bmp__error__bad_header);
      goto exit;
    } else if (v_height >= 2147483648) {
      self->private_impl.f_height = ((0 - v_height) & 2147483647);
      self->private_impl.f_top_down = true;
    } else {
      self->private_impl.f_height = v_height;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(12);
      uint32_t t_5;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_5 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_image_config[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(13);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
          uint32_t num_bits_5 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_5;
          if (num_bits_5 == 8) {
            t_5 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_5 += 8;
          *scratch |= ((uint64_t)(num_bits_5)) << 56;
        }
      }
      v_planes = t_5;
    }
    if (v_planes != 1) {
      status = wuffs_base__make_status(wuffs_bmp__error__unsupported_bmp_file);
      goto exit;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(14);
      uint32_t t_6;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_6 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_image_config[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(15);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
          uint32_t num_bits_6 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_6;
          if (num_bits_6 == 8) {
            t_6 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_6 += 8;
          *scratch |= ((uint64_t)(num_bits_6)) << 56;
        }
      }
      v_bits_per_pixel = t_6;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(16);
      uint32_t t_7;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
        t_7 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
        iop_a_src += 4;
      } else {
        self->private_data.s_decode_image_config[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(17);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
          uint32_t num_bits_7 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_7;
          if (num_bits_7 == 24) {
            t_7 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_7 += 8;
          *scratch |= ((uint64_t)(num_bits_7)) << 56;
        }
      }
      v_compression = t_7;
    }
    if (v_bits_per_pixel == 0) {
      if (v_compression == 4) {
        self->private_impl.f_io_redirect_fourcc = 1246774599;
        status = wuffs_base__make_status(wuffs_base__note__i_o_redirect);
        goto ok;
      } else if (v_compression == 5) {
        self->private_impl.f_io_redirect_fourcc = 1347307296;
        status = wuffs_base__make_status(wuffs_base__note__i_o_redirect);
        goto ok;
      }
      status = wuffs_base__make_status(wuffs_bmp__error__unsupported_bmp_file);
      goto exit;
    } else if (v_bits_per_pixel == 24) {
      self->private_impl.f_bytes_per_row = ((((((uint64_t)(self->private_impl.f_width)) * 3) + 3) >> 2) << 2);
      self->private_impl.f_pad_per_row = (self->private_impl.f_width & 3);
      self->private_impl.f_pixfmt = wuffs_base__utility__make_pixel_format(2147485832);
    } else if (v_bits_per_pixel == 32) {
      self->private_impl.f_bytes_per_row = (((uint64_t)(self->private_impl.f_width)) * 4);
      self->private_impl.f_pad_per_row = 0;
      self->private_impl.f_pixfmt = wuffs_base__utility__make_pixel_format(2164295816);
    } else {
      status = wuffs_base__make_status(wuffs_bmp__error__unsupported_bmp_file);
      goto exit;
    }
    self->private_data.s_decode_image_config[0].scratch = 20;
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(18);
    if (self->private_data.s_decode_image_config[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
      self->private_data.s_decode_image_config[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
      iop_a_src = io2_a_src;
      status = wuffs_base__make_status(wuffs_base__suspension__short_read);
      goto suspend;
    }
    iop_a_src += self->private_data.s_decode_image_config[0].scratch;
    if (v_bitmap_info_len >= 108) {
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(19);
        uint32_t t_8;
        if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
          t_8 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
          iop_a_src += 4;
        } else {
          self->private_data.s_decode_image_config[0].scratch = 0;
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(20);
          while (true) {
            if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              goto suspend;
            }
            uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
            uint32_t num_bits_8 = ((uint32_t)(*scratch >> 56));
            *scratch <<= 8;
            *scratch >>= 8;
            *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_8;
            if (num_bits_8 == 24) {
              t_8 = ((uint32_t)(*scratch));
              break;
            }
            num_bits_8 += 8;
            *scratch |= ((uint64_t)(num_bits_8)) << 56;
          }
        }
        self->private_impl.f_mask_r = t_8;
      }
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(21);
        uint32_t t_9;
        if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
          t_9 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
          iop_a_src += 4;
        } else {
          self->private_data.s_decode_image_config[0].scratch = 0;
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(22);
          while (true) {
            if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              goto suspend;
            }
            uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
            uint32_t num_bits_9 = ((uint32_t)(*scratch >> 56));
            *scratch <<= 8;
            *scratch >>= 8;
            *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_9;
            if (num_bits_9 == 24) {
              t_9 = ((uint32_t)(*scratch));
              break;
            }
            num_bits_9 += 8;
            *scratch |= ((uint64_t)(num_bits_9)) << 56;
          }
        }
        self->private_impl.f_mask_g = t_9;
      }
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(23);
        uint32_t t_10;
        if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
          t_10 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
          iop_a_src += 4;
        } else {
          self->private_data.s_decode_image_config[0].scratch = 0;
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(24);
          while (true) {
            if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              goto suspend;
            }
            uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
            uint32_t num_bits_10 = ((uint32_t)(*scratch >> 56));
            *scratch <<= 8;
            *scratch >>= 8;
            *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_10;
            if (num_bits_10 == 24) {
              t_10 = ((uint32_t)(*scratch));
              break;
            }
            num_bits_10 += 8;
            *scratch |= ((uint64_t)(num_bits_10)) << 56;
          }
        }
        self->private_impl.f_mask_b = t_10;
      }
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(25);
        uint32_t t_11;
        if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
          t_11 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
          iop_a_src += 4;
        } else {
          self->private_data.s_decode_image_config[0].scratch = 0;
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(26);
          while (true) {
            if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              goto suspend;
            }
            uint64_t* scratch = &self->private_data.s_decode_image_config[0].scratch;
            uint32_t num_bits_11 = ((uint32_t)(*scratch >> 56));
            *scratch <<= 8;
            *scratch >>= 8;
            *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_11;
            if (num_bits_11 == 24) {
              t_11 = ((uint32_t)(*scratch));
              break;
            }
            num_bits_11 += 8;
            *scratch |= ((uint64_t)(num_bits_11)) << 56;
          }
        }
        self->private_impl.f_mask_a = t_11;
      }
      if ((v_compression == 3) &&
          (self->private_impl.f_mask_r == 16711680) &&
          (self->private_impl.f_mask_g == 65280) &&
          (self->private_impl.f_mask_b == 255) &&
          (self->private_impl.f_mask_a == 4278190080)) {
        v_compression = 0;
      }
      self->private_data.s_decode_image_config[0].scratch = (v_bitmap_info_len - 56);
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(27);
      if (self->private_data.s_decode_image_config[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
        self->private_data.s_decode_image_config[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
        iop_a_src = io2_a_src;
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      iop_a_src += self->private_data.s_decode_image_config[0].scratch;
    }
    if (v_compression != 0) {
      status = wuffs_base__make_status(wuffs_bmp__error__unsupported_bmp_file);
      goto exit;
    }
    self->private_impl.f_frame_config_io_position = wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)));
    if (a_dst != NULL) {
      wuffs_base__image_config__set(
          a_dst,
          2164295816,
          0,
          self->private_impl.f_width,
          self->private_impl.f_height,
          self->private_impl.f_frame_config_io_position,
          true);
    }
    self->private_impl.f_call_sequence = 1;

    goto ok;
    ok:
    self->private_impl.p_decode_image_config[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_image_config[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 1 : 0;
  self->private_data.s_decode_image_config[0].v_bitmap_info_len = v_bitmap_info_len;
  self->private_data.s_decode_image_config[0].v_bits_per_pixel = v_bits_per_pixel;
  self->private_data.s_decode_image_config[0].v_compression = v_compression;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func bmp.decoder.decode_frame_config

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_bmp__decoder__decode_frame_config(
    wuffs_bmp__decoder* self,
    wuffs_base__frame_config* a_dst,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 2)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_frame_config[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if (self->private_impl.f_call_sequence < 1) {
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      status = wuffs_bmp__decoder__decode_image_config(self, NULL, a_src);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
    } else if (self->private_impl.f_call_sequence == 1) {
      if (self->private_impl.f_frame_config_io_position != wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)))) {
        status = wuffs_base__make_status(wuffs_base__error__bad_restart);
        goto exit;
      }
    } else if (self->private_impl.f_call_sequence == 2) {
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      status = wuffs_bmp__decoder__skip_frame(self, a_src);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
      status = wuffs_base__make_status(wuffs_base__note__end_of_data);
      goto ok;
    } else {
      status = wuffs_base__make_status(wuffs_base__note__end_of_data);
      goto ok;
    }
    if (a_dst != NULL) {
      wuffs_base__frame_config__set(
          a_dst,
          wuffs_base__utility__make_rect_ie_u32(
          0,
          0,
          self->private_impl.f_width,
          self->private_impl.f_height),
          ((wuffs_base__flicks)(0)),
          0,
          self->private_impl.f_frame_config_io_position,
          0,
          true,
          false,
          4278190080);
    }
    self->private_impl.f_call_sequence = 2;

    goto ok;
    ok:
    self->private_impl.p_decode_frame_config[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_frame_config[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 2 : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func bmp.decoder.decode_frame

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_bmp__decoder__decode_frame(
    wuffs_bmp__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend,
    wuffs_base__slice_u8 a_workbuf,
    wuffs_base__decode_frame_options* a_opts) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 3)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  wuffs_base__status v_status = wuffs_base__make_status(NULL);

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_frame[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if (self->private_impl.f_call_sequence < 2) {
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      status = wuffs_bmp__decoder__decode_frame_config(self, NULL, a_src);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
    } else if (self->private_impl.f_call_sequence == 2) {
    } else {
      status = wuffs_base__make_status(wuffs_base__note__end_of_data);
      goto ok;
    }
    self->private_data.s_decode_frame[0].scratch = self->private_impl.f_padding;
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
    if (self->private_data.s_decode_frame[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
      self->private_data.s_decode_frame[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
      iop_a_src = io2_a_src;
      status = wuffs_base__make_status(wuffs_base__suspension__short_read);
      goto suspend;
    }
    iop_a_src += self->private_data.s_decode_frame[0].scratch;
    if ((self->private_impl.f_width > 0) && (self->private_impl.f_height > 0)) {
      self->private_impl.f_dst_x = 0;
      if (self->private_impl.f_top_down) {
        self->private_impl.f_dst_y = 0;
        self->private_impl.f_dst_y_end = self->private_impl.f_height;
        self->private_impl.f_dst_y_inc = 1;
      } else {
        self->private_impl.f_dst_y = (self->private_impl.f_height - 1);
        self->private_impl.f_dst_y_end = 4294967295;
        self->private_impl.f_dst_y_inc = 4294967295;
      }
      v_status = wuffs_base__pixel_swizzler__prepare(&self->private_impl.f_swizzler,
          wuffs_base__pixel_buffer__pixel_format(a_dst),
          wuffs_base__pixel_buffer__palette(a_dst),
          self->private_impl.f_pixfmt,
          wuffs_base__utility__empty_slice_u8(),
          a_blend);
      if ( ! wuffs_base__status__is_ok(&v_status)) {
        status = v_status;
        if (wuffs_base__status__is_error(&status)) {
          goto exit;
        } else if (wuffs_base__status__is_suspension(&status)) {
          status = wuffs_base__make_status(wuffs_base__error__cannot_return_a_suspension);
          goto exit;
        }
        goto ok;
      }
      while (true) {
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        v_status = wuffs_bmp__decoder__swizzle(self, a_dst, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (wuffs_base__status__is_ok(&v_status)) {
          goto label__0__break;
        } else if (v_status.repr != wuffs_bmp__note__internal_note_short_read) {
          status = v_status;
          if (wuffs_base__status__is_error(&status)) {
            goto exit;
          } else if (wuffs_base__status__is_suspension(&status)) {
            status = wuffs_base__make_status(wuffs_base__error__cannot_return_a_suspension);
            goto exit;
          }
          goto ok;
        }
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(3);
      }
      label__0__break:;
    }
    self->private_impl.f_call_sequence = 3;

    goto ok;
    ok:
    self->private_impl.p_decode_frame[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_frame[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 3 : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func bmp.decoder.swizzle

static wuffs_base__status
wuffs_bmp__decoder__swizzle(
    wuffs_bmp__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  wuffs_base__pixel_format v_dst_pixfmt = {0};
  uint32_t v_dst_bits_per_pixel = 0;
  uint64_t v_dst_bytes_per_pixel = 0;
  uint64_t v_dst_bytes_per_row = 0;
  wuffs_base__table_u8 v_tab = {0};
  wuffs_base__slice_u8 v_dst = {0};
  uint64_t v_i = 0;
  uint64_t v_n = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  v_dst_pixfmt = wuffs_base__pixel_buffer__pixel_format(a_dst);
  v_dst_bits_per_pixel = wuffs_base__pixel_format__bits_per_pixel(&v_dst_pixfmt);
  if ((v_dst_bits_per_pixel & 7) != 0) {
    status = wuffs_base__make_status(wuffs_base__error__unsupported_option);
    goto exit;
  }
  v_dst_bytes_per_pixel = ((uint64_t)((v_dst_bits_per_pixel / 8)));
  v_dst_bytes_per_row = (((uint64_t)(self->private_impl.f_width)) * v_dst_bytes_per_pixel);
  v_tab = wuffs_base__pixel_buffer__plane(a_dst, 0);
  label__outer__continue:;
  while (true) {
    while (self->private_impl.f_pending_pad > 0) {
      if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
        status = wuffs_base__make_status(wuffs_bmp__note__internal_note_short_read);
        goto ok;
      }
      self->private_impl.f_pending_pad -= 1;
      (iop_a_src += 1, wuffs_base__make_empty_struct());
    }
    label__inner__continue:;
    while (true) {
      if (self->private_impl.f_dst_x == self->private_impl.f_width) {
        self->private_impl.f_dst_x = 0;
        self->private_impl.f_dst_y += self->private_impl.f_dst_y_inc;
        if (self->private_impl.f_dst_y == self->private_impl.f_dst_y_end) {
          goto label__outer__break;
        } else if (self->private_impl.f_pad_per_row != 0) {
          self->private_impl.f_pending_pad = self->private_impl.f_pad_per_row;
          goto label__outer__continue;
        }
      }
      v_dst = wuffs_base__table_u8__row(v_tab, self->private_impl.f_dst_y);
      if (v_dst_bytes_per_row < ((uint64_t)(v_dst.len))) {
        v_dst = wuffs_base__slice_u8__subslice_j(v_dst, v_dst_bytes_per_row);
      }
      v_i = (((uint64_t)(self->private_impl.f_dst_x)) * v_dst_bytes_per_pixel);
      if (v_i >= ((uint64_t)(v_dst.len))) {
        goto label__inner__continue;
      }
      v_n = wuffs_base__pixel_swizzler__swizzle_interleaved_from_reader(
          &self->private_impl.f_swizzler,
          wuffs_base__slice_u8__subslice_i(v_dst, v_i),
          wuffs_base__utility__empty_slice_u8(),
          &iop_a_src,
          io2_a_src);
      if (v_n == 0) {
        status = wuffs_base__make_status(wuffs_bmp__note__internal_note_short_read);
        goto ok;
      }
      wuffs_base__u32__sat_add_indirect(&self->private_impl.f_dst_x, ((uint32_t)((v_n & 4294967295))));
    }
  }
  label__outer__break:;
  status = wuffs_base__make_status(NULL);
  goto ok;

  goto ok;
  ok:
  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func bmp.decoder.skip_frame

static wuffs_base__status
wuffs_bmp__decoder__skip_frame(
    wuffs_bmp__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_skip_frame[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    self->private_data.s_skip_frame[0].scratch = self->private_impl.f_padding;
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
    if (self->private_data.s_skip_frame[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
      self->private_data.s_skip_frame[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
      iop_a_src = io2_a_src;
      status = wuffs_base__make_status(wuffs_base__suspension__short_read);
      goto suspend;
    }
    iop_a_src += self->private_data.s_skip_frame[0].scratch;
    self->private_data.s_skip_frame[0].scratch = (self->private_impl.f_bytes_per_row * ((uint64_t)(self->private_impl.f_height)));
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
    if (self->private_data.s_skip_frame[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
      self->private_data.s_skip_frame[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
      iop_a_src = io2_a_src;
      status = wuffs_base__make_status(wuffs_base__suspension__short_read);
      goto suspend;
    }
    iop_a_src += self->private_data.s_skip_frame[0].scratch;
    self->private_impl.f_call_sequence = 3;

    goto ok;
    ok:
    self->private_impl.p_skip_frame[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_skip_frame[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func bmp.decoder.frame_dirty_rect

WUFFS_BASE__MAYBE_STATIC wuffs_base__rect_ie_u32
wuffs_bmp__decoder__frame_dirty_rect(
    const wuffs_bmp__decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_rect_ie_u32();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_rect_ie_u32();
  }

  return wuffs_base__utility__make_rect_ie_u32(
      0,
      0,
      self->private_impl.f_width,
      self->private_impl.f_height);
}

// -------- func bmp.decoder.num_animation_loops

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_bmp__decoder__num_animation_loops(
    const wuffs_bmp__decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  return 0;
}

// -------- func bmp.decoder.num_decoded_frame_configs

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_bmp__decoder__num_decoded_frame_configs(
    const wuffs_bmp__decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  if (self->private_impl.f_call_sequence > 1) {
    return 1;
  }
  return 0;
}

// -------- func bmp.decoder.num_decoded_frames

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_bmp__decoder__num_decoded_frames(
    const wuffs_bmp__decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  if (self->private_impl.f_call_sequence > 2) {
    return 1;
  }
  return 0;
}

// -------- func bmp.decoder.restart_frame

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_bmp__decoder__restart_frame(
    wuffs_bmp__decoder* self,
    uint64_t a_index,
    uint64_t a_io_position) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }

  if (self->private_impl.f_call_sequence == 0) {
    return wuffs_base__make_status(wuffs_base__error__bad_call_sequence);
  }
  if (a_index != 0) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  self->private_impl.f_call_sequence = 1;
  self->private_impl.f_frame_config_io_position = a_io_position;
  return wuffs_base__make_status(NULL);
}

// -------- func bmp.decoder.set_report_metadata

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_bmp__decoder__set_report_metadata(
    wuffs_bmp__decoder* self,
    uint32_t a_fourcc,
    bool a_report) {
  return wuffs_base__make_empty_struct();
}

// -------- func bmp.decoder.tell_me_more

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_bmp__decoder__tell_me_more(
    wuffs_bmp__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__more_information* a_minfo,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 4)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  if (self->private_impl.f_io_redirect_fourcc <= 1) {
    status = wuffs_base__make_status(wuffs_base__error__no_more_information);
    goto exit;
  }
  if (a_minfo != NULL) {
    wuffs_base__more_information__set(a_minfo,
        1,
        self->private_impl.f_io_redirect_fourcc,
        0,
        self->private_impl.f_io_redirect_pos,
        18446744073709551615u);
  }
  self->private_impl.f_io_redirect_fourcc = 1;

  goto ok;
  ok:
  goto exit;
  exit:
  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func bmp.decoder.workbuf_len

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_bmp__decoder__workbuf_len(
    const wuffs_bmp__decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  return wuffs_base__utility__make_range_ii_u64(0, 0);
}

#endif  // !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__BMP)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__CRC32)

// ---------------- Status Codes Implementations

// ---------------- Private Consts

static const uint32_t
WUFFS_CRC32__IEEE_TABLE[16][256]WUFFS_BASE__POTENTIALLY_UNUSED = {
  {
    0, 1996959894, 3993919788, 2567524794, 124634137, 1886057615, 3915621685, 2657392035,
    249268274, 2044508324, 3772115230, 2547177864, 162941995, 2125561021, 3887607047, 2428444049,
    498536548, 1789927666, 4089016648, 2227061214, 450548861, 1843258603, 4107580753, 2211677639,
    325883990, 1684777152, 4251122042, 2321926636, 335633487, 1661365465, 4195302755, 2366115317,
    997073096, 1281953886, 3579855332, 2724688242, 1006888145, 1258607687, 3524101629, 2768942443,
    901097722, 1119000684, 3686517206, 2898065728, 853044451, 1172266101, 3705015759, 2882616665,
    651767980, 1373503546, 3369554304, 3218104598, 565507253, 1454621731, 3485111705, 3099436303,
    671266974, 1594198024, 3322730930, 2970347812, 795835527, 1483230225, 3244367275, 3060149565,
    1994146192, 31158534, 2563907772, 4023717930, 1907459465, 112637215, 2680153253, 3904427059,
    2013776290, 251722036, 2517215374, 3775830040, 2137656763, 141376813, 2439277719, 3865271297,
    1802195444, 476864866, 2238001368, 4066508878, 1812370925, 453092731, 2181625025, 4111451223,
    1706088902, 314042704, 2344532202, 4240017532, 1658658271, 366619977, 2362670323, 4224994405,
    1303535960, 984961486, 2747007092, 3569037538, 1256170817, 1037604311, 2765210733, 3554079995,
    1131014506, 879679996, 2909243462, 3663771856, 1141124467, 855842277, 2852801631, 3708648649,
    1342533948, 654459306, 3188396048, 3373015174, 1466479909, 544179635, 3110523913, 3462522015,
    1591671054, 702138776, 2966460450, 3352799412, 1504918807, 783551873, 3082640443, 3233442989,
    3988292384, 2596254646, 62317068, 1957810842, 3939845945, 2647816111, 81470997, 1943803523,
    3814918930, 2489596804, 225274430, 2053790376, 3826175755, 2466906013, 167816743, 2097651377,
    4027552580, 2265490386, 503444072, 1762050814, 4150417245, 2154129355, 426522225, 1852507879,
    4275313526, 2312317920, 282753626, 1742555852, 4189708143, 2394877945, 397917763, 1622183637,
    3604390888, 2714866558, 953729732, 1340076626, 3518719985, 2797360999, 1068828381, 1219638859,
    3624741850, 2936675148, 906185462, 1090812512, 3747672003, 2825379669, 829329135, 1181335161,
    3412177804, 3160834842, 628085408, 1382605366, 3423369109, 3138078467, 570562233, 1426400815,
    3317316542, 2998733608, 733239954, 1555261956, 3268935591, 3050360625, 752459403, 1541320221,
    2607071920, 3965973030, 1969922972, 40735498, 2617837225, 3943577151, 1913087877, 83908371,
    2512341634, 3803740692, 2075208622, 213261112, 2463272603, 3855990285, 2094854071, 198958881,
    2262029012, 4057260610, 1759359992, 534414190, 2176718541, 4139329115, 1873836001, 414664567,
    2282248934, 4279200368, 1711684554, 285281116, 2405801727, 4167216745, 1634467795, 376229701,
    2685067896, 3608007406, 1308918612, 956543938, 2808555105, 3495958263, 1231636301, 1047427035,
    2932959818, 3654703836, 1088359270, 936918000, 2847714899, 3736837829, 1202900863, 817233897,
    3183342108, 3401237130, 1404277552, 615818150, 3134207493, 3453421203, 1423857449, 601450431,
    3009837614, 3294710456, 1567103746, 711928724, 3020668471, 3272380065, 1510334235, 755167117,
  }, {
    0, 421212481, 842424962, 724390851, 1684849924, 2105013317, 1448781702, 1329698503,
    3369699848, 3519200073, 4210026634, 3824474571, 2897563404, 3048111693, 2659397006, 2274893007,
    1254232657, 1406739216, 2029285587, 1643069842, 783210325, 934667796, 479770071, 92505238,
    2182846553, 2600511768, 2955803355, 2838940570, 3866582365, 4285295644, 3561045983, 3445231262,
    2508465314, 2359236067, 2813478432, 3198777185, 4058571174, 3908292839, 3286139684, 3670389349,
    1566420650, 1145479147, 1869335592, 1987116393, 959540142, 539646703, 185010476, 303839341,
    3745920755, 3327985586, 3983561841, 4100678960, 3140154359, 2721170102, 2300350837, 2416418868,
    396344571, 243568058, 631889529, 1018359608, 1945336319, 1793607870, 1103436669, 1490954812,
    4034481925, 3915546180, 3259968903, 3679722694, 2484439553, 2366552896, 2787371139, 3208174018,
    950060301, 565965900, 177645455, 328046286, 1556873225, 1171730760, 1861902987, 2011255754,
    3132841300, 2745199637, 2290958294, 2442530455, 3738671184, 3352078609, 3974232786, 4126854035,
    1919080284, 1803150877, 1079293406, 1498383519, 370020952, 253043481, 607678682, 1025720731,
    1711106983, 2095471334, 1472923941, 1322268772, 26324643, 411738082, 866634785, 717028704,
    2904875439, 3024081134, 2668790573, 2248782444, 3376948395, 3495106026, 4219356713, 3798300520,
    792689142, 908347575, 487136116, 68299317, 1263779058, 1380486579, 2036719216, 1618931505,
    3890672638, 4278043327, 3587215740, 3435896893, 2206873338, 2593195963, 2981909624, 2829542713,
    998479947, 580430090, 162921161, 279890824, 1609522511, 1190423566, 1842954189, 1958874764,
    4082766403, 3930137346, 3245109441, 3631694208, 2536953671, 2385372678, 2768287173, 3155920004,
    1900120602, 1750776667, 1131931800, 1517083097, 355290910, 204897887, 656092572, 1040194781,
    3113746450, 2692952403, 2343461520, 2461357009, 3723805974, 3304059991, 4022511508, 4141455061,
    2919742697, 3072101800, 2620513899, 2234183466, 3396041197, 3547351212, 4166851439, 3779471918,
    1725839073, 2143618976, 1424512099, 1307796770, 45282277, 464110244, 813994343, 698327078,
    3838160568, 4259225593, 3606301754, 3488152955, 2158586812, 2578602749, 2996767038, 2877569151,
    740041904, 889656817, 506086962, 120682355, 1215357364, 1366020341, 2051441462, 1667084919,
    3422213966, 3538019855, 4190942668, 3772220557, 2945847882, 3062702859, 2644537544, 2226864521,
    52649286, 439905287, 823476164, 672009861, 1733269570, 2119477507, 1434057408, 1281543041,
    2167981343, 2552493150, 3004082077, 2853541596, 3847487515, 4233048410, 3613549209, 3464057816,
    1239502615, 1358593622, 2077699477, 1657543892, 764250643, 882293586, 532408465, 111204816,
    1585378284, 1197851309, 1816695150, 1968414767, 974272232, 587794345, 136598634, 289367339,
    2527558116, 2411481253, 2760973158, 3179948583, 4073438432, 3956313505, 3237863010, 3655790371,
    347922877, 229101820, 646611775, 1066513022, 1892689081, 1774917112, 1122387515, 1543337850,
    3697634229, 3313392372, 3998419255, 4148705398, 3087642289, 2702352368, 2319436851, 2468674930,
  }, {
    0, 29518391, 59036782, 38190681, 118073564, 114017003, 76381362, 89069189,
    236147128, 265370511, 228034006, 206958561, 152762724, 148411219, 178138378, 190596925,
    472294256, 501532999, 530741022, 509615401, 456068012, 451764635, 413917122, 426358261,
    305525448, 334993663, 296822438, 275991697, 356276756, 352202787, 381193850, 393929805,
    944588512, 965684439, 1003065998, 973863097, 1061482044, 1049003019, 1019230802, 1023561829,
    912136024, 933002607, 903529270, 874031361, 827834244, 815125939, 852716522, 856752605,
    611050896, 631869351, 669987326, 640506825, 593644876, 580921211, 551983394, 556069653,
    712553512, 733666847, 704405574, 675154545, 762387700, 749958851, 787859610, 792175277,
    1889177024, 1901651959, 1931368878, 1927033753, 2006131996, 1985040171, 1947726194, 1976933189,
    2122964088, 2135668303, 2098006038, 2093965857, 2038461604, 2017599123, 2047123658, 2076625661,
    1824272048, 1836991623, 1866005214, 1861914857, 1807058540, 1786244187, 1748062722, 1777547317,
    1655668488, 1668093247, 1630251878, 1625932113, 1705433044, 1684323811, 1713505210, 1742760333,
    1222101792, 1226154263, 1263738702, 1251046777, 1339974652, 1310460363, 1281013650, 1301863845,
    1187289752, 1191637167, 1161842422, 1149379777, 1103966788, 1074747507, 1112139306, 1133218845,
    1425107024, 1429406311, 1467333694, 1454888457, 1408811148, 1379576507, 1350309090, 1371438805,
    1524775400, 1528845279, 1499917702, 1487177649, 1575719220, 1546255107, 1584350554, 1605185389,
    3778354048, 3774312887, 3803303918, 3816007129, 3862737756, 3892238699, 3854067506, 3833203973,
    4012263992, 4007927823, 3970080342, 3982554209, 3895452388, 3924658387, 3953866378, 3932773565,
    4245928176, 4241609415, 4271336606, 4283762345, 4196012076, 4225268251, 4187931714, 4166823541,
    4076923208, 4072833919, 4035198246, 4047918865, 4094247316, 4123732899, 4153251322, 4132437965,
    3648544096, 3636082519, 3673983246, 3678331705, 3732010428, 3753090955, 3723829714, 3694611429,
    3614117080, 3601426159, 3572488374, 3576541825, 3496125444, 3516976691, 3555094634, 3525581405,
    3311336976, 3298595879, 3336186494, 3340255305, 3260503756, 3281337595, 3251864226, 3222399125,
    3410866088, 3398419871, 3368647622, 3372945905, 3427010420, 3448139075, 3485520666, 3456284973,
    2444203584, 2423127159, 2452308526, 2481530905, 2527477404, 2539934891, 2502093554, 2497740997,
    2679949304, 2659102159, 2620920726, 2650438049, 2562027300, 2574714131, 2603727690, 2599670141,
    2374579504, 2353749767, 2383274334, 2412743529, 2323684844, 2336421851, 2298759554, 2294686645,
    2207933576, 2186809023, 2149495014, 2178734801, 2224278612, 2236720739, 2266437690, 2262135309,
    2850214048, 2820717207, 2858812622, 2879680249, 2934667388, 2938704459, 2909776914, 2897069605,
    2817622296, 2788420399, 2759153014, 2780249921, 2700618180, 2704950259, 2742877610, 2730399645,
    3049550800, 3020298727, 3057690558, 3078802825, 2999835404, 3004150075, 2974355298, 2961925461,
    3151438440, 3121956959, 3092510214, 3113327665, 3168701108, 3172786307, 3210370778, 3197646061,
  }, {
    0, 3099354981, 2852767883, 313896942, 2405603159, 937357362, 627793884, 2648127673,
    3316918511, 2097696650, 1874714724, 3607201537, 1255587768, 4067088605, 3772741427, 1482887254,
    1343838111, 3903140090, 4195393300, 1118632049, 3749429448, 1741137837, 1970407491, 3452858150,
    2511175536, 756094997, 1067759611, 2266550430, 449832999, 2725482306, 2965774508, 142231497,
    2687676222, 412010587, 171665333, 2995192016, 793786473, 2548850444, 2237264098, 1038456711,
    1703315409, 3711623348, 3482275674, 1999841343, 3940814982, 1381529571, 1089329165, 4166106984,
    4029413537, 1217896388, 1512189994, 3802027855, 2135519222, 3354724499, 3577784189, 1845280792,
    899665998, 2367928107, 2677414085, 657096608, 3137160985, 37822588, 284462994, 2823350519,
    2601801789, 598228824, 824021174, 2309093331, 343330666, 2898962447, 3195996129, 113467524,
    1587572946, 3860600759, 4104763481, 1276501820, 3519211397, 1769898208, 2076913422, 3279374443,
    3406630818, 1941006535, 1627703081, 3652755532, 1148164341, 4241751952, 3999682686, 1457141531,
    247015245, 3053797416, 2763059142, 470583459, 2178658330, 963106687, 735213713, 2473467892,
    992409347, 2207944806, 2435792776, 697522413, 3024379988, 217581361, 508405983, 2800865210,
    4271038444, 1177467017, 1419450215, 3962007554, 1911572667, 3377213406, 3690561584, 1665525589,
    1799331996, 3548628985, 3241568279, 2039091058, 3831314379, 1558270126, 1314193216, 4142438437,
    2928380019, 372764438, 75645176, 3158189981, 568925988, 2572515393, 2346768303, 861712586,
    3982079547, 1441124702, 1196457648, 4293663189, 1648042348, 3666298377, 3358779879, 1888390786,
    686661332, 2421291441, 2196002399, 978858298, 2811169155, 523464422, 226935048, 3040519789,
    3175145892, 100435649, 390670639, 2952089162, 841119475, 2325614998, 2553003640, 546822429,
    2029308235, 3225988654, 3539796416, 1782671013, 4153826844, 1328167289, 1570739863, 3844338162,
    1298864389, 4124540512, 3882013070, 1608431339, 3255406162, 2058742071, 1744848601, 3501990332,
    2296328682, 811816591, 584513889, 2590678532, 129869501, 3204563416, 2914283062, 352848211,
    494030490, 2781751807, 3078325777, 264757620, 2450577869, 715964072, 941166918, 2158327331,
    3636881013, 1618608400, 1926213374, 3396585883, 1470427426, 4011365959, 4255988137, 1158766284,
    1984818694, 3471935843, 3695453837, 1693991400, 4180638033, 1100160564, 1395044826, 3952793279,
    3019491049, 189112716, 435162722, 2706139399, 1016811966, 2217162459, 2526189877, 774831696,
    643086745, 2666061564, 2354934034, 887166583, 2838900430, 294275499, 54519365, 3145957664,
    3823145334, 1532818963, 1240029693, 4048895640, 1820460577, 3560857924, 3331051178, 2117577167,
    3598663992, 1858283101, 2088143283, 3301633750, 1495127663, 3785470218, 4078182116, 1269332353,
    332098007, 2876706482, 3116540252, 25085497, 2628386432, 605395429, 916469259, 2384220526,
    2254837415, 1054503362, 745528876, 2496903497, 151290352, 2981684885, 2735556987, 464596510,
    1137851976, 4218313005, 3923506883, 1365741990, 3434129695, 1946996346, 1723425172, 3724871409,
  }, {
    0, 1029712304, 2059424608, 1201699536, 4118849216, 3370159984, 2403399072, 2988497936,
    812665793, 219177585, 1253054625, 2010132753, 3320900865, 4170237105, 3207642721, 2186319825,
    1625331586, 1568718386, 438355170, 658566482, 2506109250, 2818578674, 4020265506, 3535817618,
    1351670851, 1844508147, 709922595, 389064339, 2769320579, 2557498163, 3754961379, 3803185235,
    3250663172, 4238411444, 3137436772, 2254525908, 876710340, 153198708, 1317132964, 1944187668,
    4054934725, 3436268917, 2339452837, 3054575125, 70369797, 961670069, 2129760613, 1133623509,
    2703341702, 2621542710, 3689016294, 3867263574, 1419845190, 1774270454, 778128678, 318858390,
    2438067015, 2888948471, 3952189479, 3606153623, 1691440519, 1504803895, 504432359, 594620247,
    1492342857, 1704161785, 573770537, 525542041, 2910060169, 2417219385, 3618876905, 3939730521,
    1753420680, 1440954936, 306397416, 790849880, 2634265928, 2690882808, 3888375336, 3668168600,
    940822475, 91481723, 1121164459, 2142483739, 3448989963, 4042473659, 3075684971, 2318603227,
    140739594, 889433530, 1923340138, 1338244826, 4259521226, 3229813626, 2267247018, 3124975642,
    2570221389, 2756861693, 3824297005, 3734113693, 1823658381, 1372780605, 376603373, 722643805,
    2839690380, 2485261628, 3548540908, 4007806556, 1556257356, 1638052860, 637716780, 459464860,
    4191346895, 3300051327, 2199040943, 3195181599, 206718479, 825388991, 1989285231, 1274166495,
    3382881038, 4106388158, 3009607790, 2382549470, 1008864718, 21111934, 1189240494, 2072147742,
    2984685714, 2357631266, 3408323570, 4131834434, 1147541074, 2030452706, 1051084082, 63335554,
    2174155603, 3170292451, 4216760371, 3325460867, 1947622803, 1232499747, 248909555, 867575619,
    3506841360, 3966111392, 2881909872, 2527485376, 612794832, 434546784, 1581699760, 1663499008,
    3782634705, 3692447073, 2612412337, 2799048193, 351717905, 697754529, 1849071985, 1398190273,
    1881644950, 1296545318, 182963446, 931652934, 2242328918, 3100053734, 4284967478, 3255255942,
    1079497815, 2100821479, 983009079, 133672583, 3050795671, 2293717799, 3474399735, 4067887175,
    281479188, 765927844, 1778867060, 1466397380, 3846680276, 3626469220, 2676489652, 2733102084,
    548881365, 500656741, 1517752501, 1729575173, 3577210133, 3898068133, 2952246901, 2459410373,
    3910527195, 3564487019, 2480257979, 2931134987, 479546907, 569730987, 1716854139, 1530213579,
    3647316762, 3825568426, 2745561210, 2663766474, 753206746, 293940330, 1445287610, 1799716618,
    2314567513, 3029685993, 4080348217, 3461678473, 2088098201, 1091956777, 112560889, 1003856713,
    3112514712, 2229607720, 3276105720, 4263857736, 1275433560, 1902492648, 918929720, 195422344,
    685033439, 364179055, 1377080511, 1869921551, 3713294623, 3761522863, 2811507327, 2599689167,
    413436958, 633644462, 1650777982, 1594160846, 3978570462, 3494118254, 2548332990, 2860797966,
    1211387997, 1968470509, 854852413, 261368461, 3182753437, 2161434413, 3346310653, 4195650637,
    2017729436, 1160000044, 42223868, 1071931724, 2378480988, 2963576044, 4144295484, 3395602316,
  }, {
    0, 3411858341, 1304994059, 2257875630, 2609988118, 1355649459, 3596215069, 486879416,
    3964895853, 655315400, 2711298918, 1791488195, 2009251963, 3164476382, 973758832, 4048990933,
    64357019, 3364540734, 1310630800, 2235723829, 2554806413, 1394316072, 3582976390, 517157411,
    4018503926, 618222419, 2722963965, 1762783832, 1947517664, 3209171269, 970744811, 4068520014,
    128714038, 3438335635, 1248109629, 2167961496, 2621261600, 1466012805, 3522553387, 447296910,
    3959392091, 547575038, 2788632144, 1835791861, 1886307661, 3140622056, 1034314822, 4143626211,
    75106221, 3475428360, 1236444838, 2196665603, 2682996155, 1421317662, 3525567664, 427767573,
    3895035328, 594892389, 2782995659, 1857943406, 1941489622, 3101955187, 1047553757, 4113347960,
    257428076, 3288652233, 1116777319, 2311878850, 2496219258, 1603640287, 3640781169, 308099796,
    3809183745, 676813732, 2932025610, 1704983215, 2023410199, 3016104370, 894593820, 4262377657,
    210634999, 3352484690, 1095150076, 2316991065, 2535410401, 1547934020, 3671583722, 294336591,
    3772615322, 729897279, 2903845777, 1716123700, 2068629644, 2953845545, 914647431, 4258839074,
    150212442, 3282623743, 1161604689, 2388688372, 2472889676, 1480171241, 3735940167, 368132066,
    3836185911, 805002898, 2842635324, 1647574937, 2134298401, 3026852996, 855535146, 4188192143,
    186781121, 3229539940, 1189784778, 2377547631, 2427670487, 1542429810, 3715886812, 371670393,
    3882979244, 741170185, 2864262823, 1642462466, 2095107514, 3082559007, 824732849, 4201955092,
    514856152, 3589064573, 1400419795, 2552522358, 2233554638, 1316849003, 3370776517, 62202976,
    4075001525, 968836368, 3207280574, 1954014235, 1769133219, 2720925446, 616199592, 4024870413,
    493229635, 3594175974, 1353627464, 2616354029, 2264355925, 1303087088, 3409966430, 6498043,
    4046820398, 979978123, 3170710821, 2007099008, 1789187640, 2717386141, 661419827, 3962610838,
    421269998, 3527459403, 1423225061, 2676515648, 2190300152, 1238466653, 3477467891, 68755798,
    4115633027, 1041448998, 3095868040, 1943789869, 1860096405, 2776760880, 588673182, 3897205563,
    449450869, 3516317904, 1459794558, 2623431131, 2170245475, 1242006214, 3432247400, 131015629,
    4137259288, 1036337853, 3142660115, 1879958454, 1829294862, 2790523051, 549483013, 3952910752,
    300424884, 3669282065, 1545650111, 2541513754, 2323209378, 1092980487, 3350330793, 216870412,
    4256931033, 921128828, 2960342482, 2066738807, 1714085583, 2910195050, 736264132, 3770592353,
    306060335, 3647131530, 1610005796, 2494197377, 2309971513, 1123257756, 3295149874, 255536279,
    4268596802, 892423655, 3013951305, 2029645036, 1711070292, 2929725425, 674528607, 3815288570,
    373562242, 3709388839, 1535949449, 2429577516, 2379569556, 1183418929, 3223189663, 188820282,
    4195850735, 827017802, 3084859620, 2089020225, 1636228089, 2866415708, 743340786, 3876759895,
    361896217, 3738094268, 1482340370, 2466671543, 2382584591, 1163888810, 3284924932, 144124321,
    4190215028, 849168593, 3020503679, 2136336858, 1649465698, 2836138695, 798521449, 3838094284,
  }, {
    0, 2792819636, 2543784233, 837294749, 4098827283, 1379413927, 1674589498, 3316072078,
    871321191, 2509784531, 2758827854, 34034938, 3349178996, 1641505216, 1346337629, 4131942633,
    1742642382, 3249117050, 4030828007, 1446413907, 2475800797, 904311657, 68069876, 2725880384,
    1412551337, 4064729373, 3283010432, 1708771380, 2692675258, 101317902, 937551763, 2442587175,
    3485284764, 1774858792, 1478633653, 4266992385, 1005723023, 2642744891, 2892827814, 169477906,
    4233263099, 1512406095, 1808623314, 3451546982, 136139752, 2926205020, 2676114113, 972376437,
    2825102674, 236236518, 1073525883, 2576072655, 1546420545, 4200303349, 3417542760, 1841601500,
    2609703733, 1039917185, 202635804, 2858742184, 1875103526, 3384067218, 4166835727, 1579931067,
    1141601657, 3799809741, 3549717584, 1977839588, 2957267306, 372464350, 668680259, 2175552503,
    2011446046, 3516084394, 3766168119, 1175200131, 2209029901, 635180217, 338955812, 2990736784,
    601221559, 2242044419, 3024812190, 306049834, 3617246628, 1911408144, 1074125965, 3866285881,
    272279504, 3058543716, 2275784441, 567459149, 3832906691, 1107462263, 1944752874, 3583875422,
    2343980261, 767641425, 472473036, 3126744696, 2147051766, 3649987394, 3899029983, 1309766251,
    3092841090, 506333494, 801510315, 2310084639, 1276520081, 3932237093, 3683203000, 2113813516,
    3966292011, 1243601823, 2079834370, 3716205238, 405271608, 3192979340, 2411259153, 701492901,
    3750207052, 2045810168, 1209569125, 4000285905, 734575199, 2378150379, 3159862134, 438345922,
    2283203314, 778166598, 529136603, 3120492655, 2086260449, 3660498261, 3955679176, 1303499900,
    3153699989, 495890209, 744928700, 2316418568, 1337360518, 3921775410, 3626602927, 2120129051,
    4022892092, 1237286280, 2018993941, 3726666913, 461853231, 3186645403, 2350400262, 711936178,
    3693557851, 2052076527, 1270360434, 3989775046, 677911624, 2384402428, 3220639073, 427820757,
    1202443118, 3789347034, 3493118535, 1984154099, 3018127229, 362020041, 612099668, 2181885408,
    1950653705, 3526596285, 3822816288, 1168934804, 2148251930, 645706414, 395618355, 2984485767,
    544559008, 2248295444, 3085590153, 295523645, 3560598451, 1917673479, 1134918298, 3855773998,
    328860103, 3052210803, 2214924526, 577903450, 3889505748, 1101147744, 1883911421, 3594338121,
    3424493451, 1785369663, 1535282850, 4260726038, 944946072, 2653270060, 2949491377, 163225861,
    4294103532, 1501944408, 1752023237, 3457862513, 196998655, 2915761739, 2619532502, 978710370,
    2881684293, 229902577, 1012666988, 2586515928, 1603020630, 4193987810, 3356702335, 1852063179,
    2553040162, 1046169238, 263412747, 2848217023, 1818454321, 3390333573, 4227627032, 1569420204,
    60859927, 2782375331, 2487203646, 843627658, 4159668740, 1368951216, 1617990445, 3322386585,
    810543216, 2520310724, 2815490393, 27783917, 3288386659, 1652017111, 1402985802, 4125677310,
    1685994201, 3255382381, 4091620336, 1435902020, 2419138250, 910562686, 128847843, 2715354199,
    1469150398, 4058414858, 3222168983, 1719234083, 2749255853, 94984985, 876691844, 2453031472,
  }, {
    0, 3433693342, 1109723005, 2391738339, 2219446010, 1222643300, 3329165703, 180685081,
    3555007413, 525277995, 2445286600, 1567235158, 1471092047, 2600801745, 361370162, 3642757804,
    2092642603, 2953916853, 1050555990, 4063508168, 4176560081, 878395215, 3134470316, 1987983410,
    2942184094, 1676945920, 3984272867, 567356797, 722740324, 3887998202, 1764827929, 2778407815,
    4185285206, 903635656, 3142804779, 2012833205, 2101111980, 2979425330, 1058630609, 4088621903,
    714308067, 3862526333, 1756790430, 2753330688, 2933487385, 1651734407, 3975966820, 542535930,
    2244825981, 1231508451, 3353891840, 188896414, 25648519, 3442302233, 1134713594, 2399689316,
    1445480648, 2592229462, 336416693, 3634843435, 3529655858, 516441772, 2420588879, 1559052753,
    698204909, 3845636723, 1807271312, 2803025166, 2916600855, 1635634313, 4025666410, 593021940,
    4202223960, 919787974, 3093159461, 1962401467, 2117261218, 2996361020, 1008193759, 4038971457,
    1428616134, 2576151384, 386135227, 3685348389, 3513580860, 499580322, 2471098945, 1608776415,
    2260985971, 1248454893, 3303468814, 139259792, 42591881, 3458459159, 1085071860, 2349261162,
    3505103035, 474062885, 2463016902, 1583654744, 1419882049, 2550902495, 377792828, 3660491170,
    51297038, 3483679632, 1093385331, 2374089965, 2269427188, 1273935210, 3311514249, 164344343,
    2890961296, 1627033870, 4000683757, 585078387, 672833386, 3836780532, 1782552599, 2794821769,
    2142603813, 3005188795, 1032883544, 4047146438, 4227826911, 928351297, 3118105506, 1970307900,
    1396409818, 2677114180, 287212199, 3719594553, 3614542624, 467372990, 2505346141, 1509854403,
    2162073199, 1282711281, 3271268626, 240228748, 76845205, 3359543307, 1186043880, 2317064054,
    796964081, 3811226735, 1839575948, 2702160658, 2882189835, 1734392469, 3924802934, 625327592,
    4234522436, 818917338, 3191908409, 1927981223, 2016387518, 3028656416, 973776579, 4137723485,
    2857232268, 1726474002, 3899187441, 616751215, 772270454, 3803048424, 1814228491, 2693328533,
    2041117753, 3036871847, 999160644, 4146592730, 4259508931, 826864221, 3217552830, 1936586016,
    3606501031, 442291769, 2496909786, 1484378436, 1388107869, 2652297411, 278519584, 3694387134,
    85183762, 3384397196, 1194773103, 2342308593, 2170143720, 1307820918, 3279733909, 265733131,
    2057717559, 3054258089, 948125770, 4096344276, 4276898253, 843467091, 3167309488, 1885556270,
    2839764098, 1709792284, 3949353983, 667704161, 755585656, 3785577190, 1865176325, 2743489947,
    102594076, 3401021058, 1144549729, 2291298815, 2186770662, 1325234296, 3228729243, 215514885,
    3589828009, 424832311, 2547870420, 1534552650, 1370645331, 2635621325, 328688686, 3745342640,
    2211456353, 1333405183, 3254067740, 224338562, 127544219, 3408931589, 1170156774, 2299866232,
    1345666772, 2627681866, 303053225, 3736746295, 3565105198, 416624816, 2522494803, 1525692365,
    4285207626, 868291796, 3176010551, 1910772649, 2065767088, 3079346734, 956571085, 4121828691,
    747507711, 3760459617, 1856702594, 2717976604, 2831417605, 1684930971, 3940615800, 642451174,
  },
  {
    0, 393942083, 787884166, 965557445, 1575768332, 1251427663, 1931114890, 1684106697,
    3151536664, 2896410203, 2502855326, 2186649309, 3862229780, 4048545623, 3368213394, 3753496529,
    2898281073, 3149616690, 2184604407, 2504883892, 4046197629, 3864463166, 3755621371, 3366006712,
    387506281, 6550570, 971950319, 781573292, 1257550181, 1569695014, 1677892067, 1937345952,
    2196865699, 2508887776, 2886183461, 3145514598, 3743273903, 3362179052, 4058774313, 3868258154,
    958996667, 777139448, 400492605, 10755198, 1690661303, 1941857780, 1244879153, 1565019506,
    775012562, 961205393, 13101140, 398261271, 1943900638, 1688634781, 1563146584, 1246801179,
    2515100362, 2190636681, 3139390028, 2892258831, 3355784134, 3749586821, 3874691904, 4052225795,
    3734110983, 3387496260, 4033096577, 3877584834, 2206093835, 2483373640, 2911402637, 3136515790,
    1699389727, 1915860316, 1270647193, 1556585946, 950464531, 803071056, 374397077, 19647702,
    1917993334, 1697207605, 1554278896, 1272937907, 800985210, 952435769, 21510396, 372452543,
    3381322606, 3740399405, 3883715560, 4027047851, 2489758306, 2199758369, 3130039012, 2917895847,
    1550025124, 1259902439, 1922410786, 1710144865, 26202280, 385139947, 796522542, 939715693,
    3887801276, 4039129087, 3377269562, 3728088953, 3126293168, 2905368307, 2493602358, 2212122229,
    4037264341, 3889747862, 3730172755, 3375300368, 2907673305, 3124004506, 2209987167, 2495786524,
    1266377165, 1543533966, 1703758155, 1928748296, 379007169, 32253058, 945887303, 790236164,
    1716846671, 1898845196, 1218652361, 1608006794, 1002000707, 750929152, 357530053, 36990342,
    3717046871, 3405166100, 4084959953, 3825245842, 2153902939, 2535122712, 2929187805, 3119304606,
    3398779454, 3723384445, 3831720632, 4078468859, 2541294386, 2147616625, 3113171892, 2935238647,
    1900929062, 1714877541, 1606142112, 1220599011, 748794154, 1004184937, 39295404, 355241455,
    3835986668, 4091516591, 3394415210, 3710500393, 3108557792, 2922629027, 2545875814, 2160455461,
    1601970420, 1208431799, 1904871538, 1727077425, 43020792, 367748539, 744905086, 991776061,
    1214562461, 1595921630, 1720903707, 1911159896, 361271697, 49513938, 998160663, 738569556,
    4089209477, 3838277318, 3712633347, 3392233024, 2924491657, 3106613194, 2158369551, 2547846988,
    3100050248, 2948339467, 2519804878, 2169126797, 3844821572, 4065347079, 3420289730, 3701894785,
    52404560, 342144275, 770279894, 982687125, 1593045084, 1233708063, 1879431386, 1736363161,
    336019769, 58479994, 988899775, 764050940, 1240141877, 1586496630, 1729968307, 1885744368,
    2950685473, 3097818978, 2166999975, 2522013668, 4063474221, 3846743662, 3703937707, 3418263272,
    976650731, 760059304, 348170605, 62635310, 1742393575, 1889649828, 1227683937, 1582820386,
    2179867635, 2526361520, 2937588597, 3093503798, 3691148031, 3413731004, 4076100217, 3851374138,
    2532754330, 2173556697, 3087067932, 2944139103, 3407516310, 3697379029, 3857496592, 4070026835,
    758014338, 978679233, 64506116, 346250567, 1891774606, 1740186829, 1580472328, 1229917259,
  }, {
    0, 4022496062, 83218493, 3946298115, 166436986, 3861498692, 220098631, 3806075769,
    332873972, 4229245898, 388141257, 4175494135, 440197262, 4127099824, 516501683, 4044053389,
    665747944, 3362581206, 593187285, 3432594155, 776282514, 3246869164, 716239279, 3312622225,
    880394524, 3686509090, 814485793, 3746462239, 1033003366, 3528460888, 963096923, 3601193573,
    1331495888, 2694801646, 1269355501, 2758457555, 1186374570, 2843003028, 1111716759, 2910918825,
    1552565028, 3007850522, 1484755737, 3082680359, 1432478558, 3131279456, 1368666979, 3193329757,
    1760789048, 2268195078, 1812353541, 2210675003, 1628971586, 2396670332, 1710092927, 2318375233,
    2066006732, 2498144754, 2144408305, 2417195471, 1926193846, 2634877320, 1983558283, 2583222709,
    2662991776, 1903717534, 2588923805, 1972223139, 2538711002, 2022952164, 2477029351, 2087066841,
    2372749140, 1655647338, 2308478825, 1717238871, 2223433518, 1799654416, 2155034387, 1873894445,
    3105130056, 1456926070, 3185661557, 1378041163, 2969511474, 1597852940, 3020617231, 1539874097,
    2864957116, 1157737858, 2922780289, 1106542015, 2737333958, 1290407416, 2816325371, 1210047941,
    3521578096, 1042640718, 3574781005, 986759027, 3624707082, 936300340, 3707335735, 859512585,
    3257943172, 770846650, 3334837433, 688390023, 3420185854, 605654976, 3475911875, 552361981,
    4132013464, 428600998, 4072428965, 494812827, 4288816610, 274747100, 4216845791, 345349857,
    3852387692, 173846098, 3781891409, 245988975, 3967116566, 62328360, 3900749099, 121822741,
    3859089665, 164061759, 3807435068, 221426178, 4025395579, 2933317, 3944446278, 81334904,
    4124199413, 437265099, 4045904328, 518386422, 4231653775, 335250097, 4174133682, 386814604,
    3249244393, 778691543, 3311294676, 714879978, 3359647891, 662848429, 3434477742, 595039120,
    3531393053, 1035903779, 3599308832, 961245982, 3684132967, 877986649, 3747788890, 815846244,
    2841119441, 1184522735, 2913852140, 1114616274, 2696129195, 1332855189, 2756082326, 1266946472,
    3129952805, 1431118107, 3195705880, 1371074854, 3009735263, 1554415969, 3079748194, 1481855324,
    2398522169, 1630855175, 2315475716, 1707159610, 2266835779, 1759461501, 2213084030, 1814728768,
    2636237773, 1927520499, 2580814832, 1981182158, 2496293815, 2064121993, 2420095882, 2147340468,
    2025787041, 2541577631, 2085281436, 2475210146, 1901375195, 2660681189, 1973518054, 2590184920,
    1801997909, 2225743211, 1872600680, 2153772374, 1652813359, 2369881361, 1719025170, 2310296876,
    1594986313, 2966676599, 1541693300, 3022402634, 1459236659, 3107472397, 1376780046, 3184366640,
    1288097725, 2734990467, 1211309952, 2817619134, 1160605639, 2867791097, 1104723962, 2920993988,
    937561457, 3626001999, 857201996, 3704993394, 1040821515, 3519792693, 989625654, 3577615880,
    607473029, 3421972155, 549494200, 3473077894, 769584639, 3256649409, 690699714, 3337180924,
    273452185, 4287555495, 347692196, 4219156378, 430386403, 4133832669, 491977950, 4069562336,
    60542061, 3965298515, 124656720, 3903616878, 175139863, 3853649705, 243645482, 3779581716,
  }, {
    0, 3247366080, 1483520449, 2581751297, 2967040898, 1901571138, 3904227907, 691737987,
    3133399365, 2068659845, 3803142276, 589399876, 169513671, 3415493895, 1383475974, 2482566342,
    2935407819, 1870142219, 4137319690, 924099274, 506443593, 3751897225, 1178799752, 2278412616,
    339027342, 3585866318, 1280941135, 2379694991, 2766951948, 1700956620, 4236308429, 1024339981,
    2258407383, 1192382487, 3740284438, 528411094, 910556245, 4157285269, 1848198548, 2946996820,
    1012887186, 4258378066, 1681119059, 2780629139, 2357599504, 1292419792, 3572147409, 358906641,
    678054684, 3924071644, 1879503581, 2978491677, 2561882270, 1497229150, 3235873119, 22109855,
    2460592729, 1395094937, 3401913240, 189516888, 577821147, 3825075739, 2048679962, 3146956762,
    3595049455, 398902831, 2384764974, 1336573934, 1720805997, 2803873197, 1056822188, 4285729900,
    1821112490, 2902796138, 887570795, 4117339819, 3696397096, 500978920, 2218668777, 1169222953,
    2025774372, 3106931428, 550659301, 3780950821, 3362238118, 166293862, 2416645991, 1367722151,
    3262987361, 66315169, 2584839584, 1537170016, 1923370979, 3005911075, 717813282, 3947244002,
    1356109368, 2438613496, 146288633, 3375820857, 3759007162, 562248314, 3093388411, 2045739963,
    3927406461, 731490493, 2994458300, 1945440636, 1523451135, 2604718911, 44219710, 3274466046,
    4263662323, 1068272947, 2790189874, 1740649714, 1325080945, 2406874801, 379033776, 3608758128,
    1155642294, 2238671990, 479005303, 3708016055, 4097359924, 901128180, 2891217397, 1843045941,
    2011248031, 3060787807, 797805662, 3993195422, 3342353949, 112630237, 2673147868, 1591353372,
    3441611994, 212601626, 2504944923, 1421914843, 2113644376, 3161815192, 630660761, 3826893145,
    3642224980, 412692116, 2172340373, 1089836885, 1775141590, 2822790422, 832715543, 4029474007,
    1674842129, 2723860433, 1001957840, 4197873168, 3540870035, 310623315, 2338445906, 1257178514,
    4051548744, 821257608, 2836464521, 1755307081, 1101318602, 2150241802, 432566283, 3628511179,
    1270766349, 2318435533, 332587724, 3529260300, 4217841807, 988411727, 2735444302, 1652903566,
    1602977411, 2651169091, 132630338, 3328776322, 4015131905, 786223809, 3074340032, 1991273216,
    3846741958, 616972294, 3173262855, 2091579847, 1435626564, 2485072772, 234706309, 3430124101,
    2712218736, 1613231024, 4190475697, 944458353, 292577266, 3506339890, 1226630707, 2291284467,
    459984181, 3672380149, 1124496628, 2189994804, 2880683703, 1782407543, 4091479926, 844224694,
    257943739, 3469817723, 1462980986, 2529005242, 3213269817, 2114471161, 3890881272, 644152632,
    3046902270, 1947391550, 3991973951, 746483711, 88439420, 3301680572, 1563018173, 2628197501,
    657826727, 3871046759, 2136545894, 3201811878, 2548879397, 1449267173, 3481299428, 235845156,
    2650161890, 1551408418, 3315268387, 68429027, 758067552, 3970035360, 1967360161, 3033356129,
    2311284588, 1213053100, 3517963949, 270598509, 958010606, 4170500910, 1635167535, 2700636911,
    855672361, 4069415401, 1802256360, 2866995240, 2212099499, 1113008747, 3686091882, 440112042,
  }, {
    0, 2611301487, 3963330207, 2006897392, 50740095, 2560849680, 4013794784, 1956178319,
    101480190, 2645113489, 3929532513, 1905435662, 84561281, 2662269422, 3912356638, 1922342769,
    202960380, 2545787283, 3760419683, 2072395532, 253679235, 2495322860, 3810871324, 2021655667,
    169122562, 2444351341, 3861841309, 2106214898, 152215677, 2461527058, 3844685538, 2123133581,
    405920760, 2207553431, 4094313831, 1873742088, 456646791, 2157096168, 4144791064, 1823027831,
    507358470, 2241388905, 4060492697, 1772322806, 490444409, 2258557462, 4043311334, 1789215881,
    338245124, 2408348267, 4161972379, 1672996084, 388959611, 2357870868, 4212429796, 1622269835,
    304431354, 2306870421, 4263435877, 1706791434, 287538053, 2324051946, 4246267162, 1723705717,
    811841520, 2881944479, 3696765295, 1207788800, 862293135, 2831204576, 3747484176, 1157324415,
    913293582, 2915732833, 3662962577, 1106318334, 896137841, 2932651550, 3646055662, 1123494017,
    1014716940, 2816349795, 3493905555, 1273334012, 1065181555, 2765630748, 3544645612, 1222882179,
    980888818, 2714919069, 3595350637, 1307180546, 963712909, 2731826146, 3578431762, 1324336509,
    676490248, 3019317351, 3295277719, 1607253752, 726947703, 2968591128, 3345992168, 1556776327,
    777919222, 3053147801, 3261432937, 1505806342, 760750473, 3070062054, 3244539670, 1522987897,
    608862708, 3220163995, 3362856811, 1406423812, 659339915, 3169449700, 3413582868, 1355966587,
    575076106, 3118709605, 3464325525, 1440228858, 557894773, 3135602714, 3447411434, 1457397381,
    1623683040, 4217512847, 2365387135, 391757072, 1673614495, 4167309552, 2415577600, 341804655,
    1724586270, 4251866481, 2331019137, 290835438, 1707942497, 4268256782, 2314648830, 307490961,
    1826587164, 4152020595, 2162433155, 457265388, 1876539747, 4101829900, 2212636668, 407333779,
    1792275682, 4051089549, 2263378557, 491595282, 1775619997, 4067460082, 2246988034, 508239213,
    2029433880, 3813931127, 2496473735, 258500328, 2079362919, 3763716872, 2546668024, 208559511,
    2130363110, 3848244873, 2462145657, 157552662, 2113730969, 3864638966, 2445764358, 174205801,
    1961777636, 4014675339, 2564147067, 57707284, 2011718299, 3964481268, 2614361092, 7778411,
    1927425818, 3913769845, 2665066885, 92077546, 1910772837, 3930150922, 2648673018, 108709525,
    1352980496, 3405878399, 3164554895, 658115296, 1403183983, 3355946752, 3214507504, 607924639,
    1453895406, 3440239233, 3130208369, 557218846, 1437504913, 3456883198, 3113552654, 573589345,
    1555838444, 3340335491, 2961681267, 723707676, 1606028947, 3290383100, 3011612684, 673504355,
    1521500946, 3239382909, 3062619533, 758026722, 1505130605, 3256038402, 3045975794, 774417053,
    1217725416, 3543158663, 2762906999, 1057739032, 1267939479, 3493229816, 2812847624, 1007544935,
    1318679830, 3577493881, 2728586121, 956803046, 1302285929, 3594125830, 2711933174, 973184153,
    1150152212, 3743982203, 2830528651, 856898788, 1200346475, 3694041348, 2880457716, 806684571,
    1115789546, 3643069573, 2931426933, 891243034, 1099408277, 3659722746, 2914794762, 907637093,
  }, {
    0, 3717650821, 1616688459, 3184159950, 3233376918, 489665299, 2699419613, 2104690264,
    1510200173, 2274691816, 979330598, 3888758691, 2595928571, 1194090622, 4209380528, 661706037,
    3020400346, 1771143007, 3562738577, 164481556, 1958661196, 2837976521, 350386439, 3379863682,
    3993269687, 865250354, 2388181244, 1406015865, 784146209, 4079732388, 1323412074, 2474079215,
    3011398645, 1860735600, 3542286014, 246687547, 1942430051, 2924607718, 328963112, 3456978349,
    3917322392, 887832861, 2300653011, 1421341782, 700772878, 4099025803, 1234716485, 2483986112,
    125431087, 3673109674, 1730500708, 3132326369, 3351283641, 441867836, 2812031730, 2047535991,
    1568292418, 2163009479, 1025936137, 3769651852, 2646824148, 1079348561, 4255113631, 537475098,
    3180171691, 1612400686, 3721471200, 4717925, 2100624189, 2694980280, 493375094, 3237910515,
    3884860102, 974691139, 2278750093, 1514417672, 657926224, 4204917205, 1198234907, 2600289438,
    160053105, 3558665972, 1775665722, 3024116671, 3375586791, 346391650, 2842683564, 1962488105,
    1401545756, 2384412057, 869618007, 3997403346, 2469432970, 1319524111, 4083956673, 788193860,
    250862174, 3546612699, 1856990997, 3006903952, 3461001416, 333211981, 2920678787, 1937824774,
    1425017139, 2305216694, 883735672, 3912918525, 2487837605, 1239398944, 4095071982, 696455019,
    3136584836, 1734518017, 3668494799, 121507914, 2051872274, 2816200599, 437363545, 3347544796,
    3774328809, 1029797484, 2158697122, 1564328743, 542033279, 4258798842, 1074950196, 2642717105,
    2691310871, 2113731730, 3224801372, 497043929, 1624461185, 3175454212, 9435850, 3709412175,
    4201248378, 671035391, 2587181873, 1201904308, 986750188, 3880142185, 1519135143, 2266689570,
    342721485, 3388693064, 1949382278, 2846355203, 3570723163, 155332830, 3028835344, 1763607957,
    1315852448, 2482538789, 775087595, 4087626862, 2396469814, 1396827059, 4002123645, 857560824,
    320106210, 3464673127, 1934154665, 2933785132, 3551331444, 238804465, 3018961215, 1852270778,
    1226292623, 2491507722, 692783300, 4108177729, 2309936921, 1412959900, 3924976210, 879016919,
    2803091512, 2055541181, 3343875443, 450471158, 1739236014, 3124525867, 133568485, 3663777376,
    4245691221, 545702608, 2639048222, 1088059291, 1034514883, 3762268230, 1576387720, 2153979149,
    501724348, 3228659001, 2109407735, 2687359090, 3713981994, 13109167, 3171052385, 1620357860,
    1206151121, 2591211092, 666423962, 4197321503, 2271022407, 1523307714, 3875649548, 982999433,
    2850034278, 1953942499, 3384583981, 338329256, 1767471344, 3033506165, 151375291, 3566408766,
    4091789579, 779425934, 2478797888, 1311354309, 861580189, 4006375960, 1392910038, 2391852883,
    2929327945, 1930372812, 3469036034, 324244359, 1847629279, 3015068762, 243015828, 3555391761,
    4103744548, 688715169, 2496043375, 1229996266, 874727090, 3920994103, 1417671673, 2313759356,
    446585235, 3339223062, 2059594968, 2807313757, 3660002053, 129100416, 3128657486, 1743609803,
    1084066558, 2634765179, 549535669, 4250396208, 2149900392, 1571961325, 3765982499, 1039043750,
  }, {
    0, 2635063670, 3782132909, 2086741467, 430739227, 2225303149, 4173482934, 1707977408,
    861478454, 2924937024, 3526875803, 1329085421, 720736557, 3086643291, 3415954816, 1452586230,
    1722956908, 4223524122, 2279405761, 450042295, 2132718455, 3792785921, 2658170842, 58693292,
    1441473114, 3370435372, 3028674295, 696911745, 1279765825, 3511176247, 2905172460, 807831706,
    3445913816, 1349228974, 738901109, 2969918723, 3569940419, 1237784245, 900084590, 2829701656,
    4265436910, 1664255896, 525574723, 2187084597, 3885099509, 2057177219, 117386584, 2616249390,
    2882946228, 920233410, 1253605401, 3619119471, 2994391983, 796207833, 1393823490, 3457937012,
    2559531650, 92322804, 2044829231, 3840835417, 2166609305, 472659183, 1615663412, 4249022530,
    1102706673, 3702920839, 2698457948, 1037619754, 1477802218, 3306854812, 3111894087, 611605809,
    1927342535, 4025419953, 2475568490, 243387420, 1800169180, 4131620778, 2317525617, 388842247,
    655084445, 3120835307, 3328511792, 1533734470, 1051149446, 2745738736, 3754524715, 1120297309,
    340972971, 2304586973, 4114354438, 1748234352, 234773168, 2431761350, 3968900637, 1906278251,
    2363330345, 299003487, 1840466820, 4038896370, 2507210802, 142532932, 1948239007, 3910149609,
    3213136159, 579563625, 1592415666, 3286611140, 2787646980, 992477042, 1195825833, 3662232543,
    3933188933, 2002801203, 184645608, 2517538462, 4089658462, 1858919720, 313391347, 2409765253,
    3644239219, 1144605701, 945318366, 2773977256, 3231326824, 1570095902, 569697989, 3170568115,
    2205413346, 511446676, 1646078799, 4279421497, 2598330617, 131105167, 2075239508, 3871229218,
    2955604436, 757403810, 1363424633, 3427521551, 2844163791, 881434553, 1223211618, 3588709140,
    3854685070, 2026779384, 78583587, 2577462869, 4235025557, 1633861091, 486774840, 2148301134,
    3600338360, 1268198606, 938871061, 2868504675, 3476308643, 1379640277, 777684494, 3008718712,
    1310168890, 3541595724, 2943964055, 846639841, 1471879201, 3400857943, 3067468940, 735723002,
    2102298892, 3762382970, 2619362721, 19901655, 1692534295, 4193118049, 2240594618, 411247564,
    681945942, 3047836192, 3385552891, 1422167693, 822682701, 2886124859, 3496468704, 1298661782,
    469546336, 2264093718, 4203901389, 1738379451, 38812283, 2673859341, 3812556502, 2117148576,
    3268024339, 1606809957, 598006974, 3198893512, 3680933640, 1181316734, 973624229, 2802299603,
    4052944421, 1822222163, 285065864, 2381456382, 3896478014, 1966106696, 156323219, 2489232613,
    2759337087, 964150537, 1159127250, 3625517476, 3184831332, 551242258, 1555722185, 3249901247,
    2535537225, 170842943, 1984954084, 3946848146, 2391651666, 327308324, 1877176831, 4075589769,
    263086283, 2460058045, 4005602406, 1942963472, 369291216, 2332888742, 4151061373, 1784924683,
    1022852861, 2717425547, 3717839440, 1083595558, 626782694, 3092517008, 3291821387, 1497027645,
    1763466407, 4094934481, 2289211402, 360544636, 1890636732, 3988730570, 2447251217, 215086695,
    1514488465, 3343557607, 3140191804, 639919946, 1139395978, 3739626748, 2726758695, 1065936977,
  }, {
    0, 3120290792, 2827399569, 293431929, 2323408227, 864534155, 586863858, 2600537882,
    3481914503, 1987188591, 1729068310, 3740575486, 1173727716, 4228805132, 3983743093, 1418249117,
    1147313999, 4254680231, 3974377182, 1428157750, 3458136620, 2011505092, 1721256893, 3747844181,
    2347455432, 839944224, 594403929, 2593536433, 26687147, 3094146371, 2836498234, 283794642,
    2294627998, 826205558, 541298447, 2578994407, 45702141, 3141697557, 2856315500, 331624836,
    1196225049, 4273416689, 4023010184, 1446090848, 3442513786, 1959480466, 1706436331, 3696098563,
    3433538001, 1968994873, 1679888448, 3722103720, 1188807858, 4280295258, 3999102243, 1470541515,
    53374294, 3134568126, 2879970503, 307431215, 2303854645, 816436189, 567589284, 2553242188,
    3405478781, 1929420949, 1652411116, 3682996484, 1082596894, 4185703926, 3892424591, 1375368295,
    91404282, 3163122706, 2918450795, 336584067, 2400113305, 922028401, 663249672, 2658384096,
    2392450098, 929185754, 639587747, 2682555979, 82149713, 3172883129, 2892181696, 362343208,
    1091578037, 4176212829, 3918960932, 1349337804, 3412872662, 1922537022, 1676344391, 3658557359,
    1111377379, 4224032267, 3937989746, 1396912026, 3359776896, 1908013928, 1623494929, 3644803833,
    2377615716, 877417100, 623982837, 2630542109, 130804743, 3190831087, 2941083030, 381060734,
    106748588, 3215393092, 2933549885, 388083925, 2350956495, 903570471, 614862430, 2640172470,
    3386185259, 1882115523, 1632872378, 3634920530, 1135178568, 4199721120, 3945775833, 1389631793,
    1317531835, 4152109907, 3858841898, 1610259138, 3304822232, 2097172016, 1820140617, 3582394273,
    2165193788, 955639764, 696815021, 2423477829, 192043359, 2995356343, 2750736590, 437203750,
    182808564, 3005133852, 2724453989, 462947725, 2157513367, 962777471, 673168134, 2447663342,
    3312231283, 2090301595, 1844056802, 3557935370, 1326499344, 4142603768, 3885397889, 1584245865,
    3326266917, 2142836173, 1858371508, 3611272284, 1279175494, 4123357358, 3837270743, 1564721471,
    164299426, 2955991370, 2706223923, 414607579, 2209834945, 978107433, 724686416, 2462715320,
    2183156074, 1004243586, 715579643, 2472360723, 140260361, 2980573153, 2698675608, 421617264,
    1302961645, 4099032581, 3845074044, 1557460884, 3352688782, 2116952934, 1867729183, 3601371895,
    2222754758, 1032278062, 754596439, 2499928511, 234942117, 3086693709, 2793824052, 528319708,
    1274365761, 4061043881, 3816027856, 1518873912, 3246989858, 2020800970, 1762628531, 3505670235,
    3223196809, 2045103969, 1754834200, 3512958704, 1247965674, 4086934018, 3806642299, 1528765331,
    261609486, 3060532198, 2802936223, 518697591, 2246819181, 1007707781, 762121468, 2492913428,
    213497176, 3041029808, 2755593417, 499441441, 2261110843, 1061030867, 776167850, 2545465922,
    3274734047, 2060165687, 1807140942, 3528266662, 1229724860, 4038575956, 3788156205, 1479636677,
    1222322711, 4045468159, 3764231046, 1504067694, 3265744756, 2069664924, 1780612837, 3554288909,
    2270357136, 1051278712, 802445057, 2519698665, 221152243, 3033880603, 2779263586, 475261322,
  }, {
    0, 2926088593, 2275419491, 701019378, 3560000647, 2052709654, 1402038756, 4261017717,
    1930665807, 3715829470, 4105419308, 1524313021, 2804077512, 155861593, 545453739, 2397726522,
    3861331614, 1213181711, 1636244477, 3488582252, 840331801, 2625561480, 3048626042, 467584747,
    2503254481, 995897408, 311723186, 3170637091, 1090907478, 4016929991, 3332753461, 1758288292,
    390036349, 3109546732, 2426363422, 1056427919, 3272488954, 1835443819, 1152258713, 3938878216,
    1680663602, 3393484195, 3817652561, 1306808512, 2954733749, 510998820, 935169494, 2580880455,
    4044899811, 1601229938, 1991794816, 3637571857, 623446372, 2336332021, 2726898695, 216120726,
    2181814956, 744704829, 95158223, 2881711710, 1446680107, 4166125498, 3516576584, 2146575065,
    780072698, 2148951915, 2849952665, 129384968, 4199529085, 1411853292, 2112855838, 3548843663,
    1567451573, 4077254692, 3670887638, 1957027143, 2304517426, 657765539, 251396177, 2694091200,
    3361327204, 1714510325, 1341779207, 3784408214, 476611811, 2986349938, 2613617024, 899690513,
    3142211371, 354600634, 1021997640, 2458051545, 1870338988, 3239283261, 3906682575, 1186180958,
    960597383, 2536053782, 3202459876, 277428597, 3983589632, 1125666961, 1792074851, 3300423154,
    1246892744, 3829039961, 3455203243, 1671079482, 2657312335, 806080478, 432241452, 3081497277,
    3748049689, 1896751752, 1489409658, 4138600427, 190316446, 2772397583, 2365053693, 580864876,
    2893360214, 35503559, 735381813, 2243795108, 2017747153, 3593269568, 4293150130, 1368183843,
    1560145396, 4069882981, 3680356503, 1966430470, 2295112051, 648294626, 258769936, 2701399425,
    804156091, 2173100842, 2823706584, 103204425, 4225711676, 1438101421, 2088704863, 3524758222,
    3134903146, 347226875, 1031468553, 2467456920, 1860935661, 3229814396, 3914054286, 1193487135,
    3385412645, 1738661300, 1315531078, 3758225623, 502792354, 3012596019, 2589468097, 875607120,
    1271043721, 3853125400, 3429020650, 1644831355, 2683558414, 832261023, 408158061, 3057348348,
    953223622, 2528745559, 3211865253, 286899508, 3974120769, 1116263632, 1799381026, 3307794867,
    2917509143, 59586950, 709201268, 2217549029, 2043995280, 3619452161, 4269064691, 1344032866,
    3740677976, 1889445577, 1498812987, 4148069290, 180845535, 2762992206, 2372361916, 588238637,
    1921194766, 3706423967, 4112727661, 1531686908, 2796705673, 148555288, 554857194, 2407195515,
    26248257, 2952271312, 2251333922, 676868275, 3584149702, 2076793175, 1375858085, 4234771508,
    2493785488, 986493953, 319029491, 3178008930, 1083533591, 4009621638, 3342158964, 1767759333,
    3887577823, 1239362382, 1612160956, 3464433197, 864482904, 2649647049, 3022443323, 441336490,
    1706844275, 3419730402, 3793503504, 1282724993, 2978819316, 535149925, 908921239, 2554697734,
    380632892, 3100077741, 2433735263, 1063734222, 3265180603, 1828069930, 1161729752, 3948283721,
    2207997677, 770953084, 71007118, 2857626143, 1470763626, 4190274555, 3490330377, 2120394392,
    4035494306, 1591758899, 1999168705, 3644880208, 616140069, 2328960180, 2736367686, 225524183,
  },
};

// ---------------- Private Initializer Prototypes

// ---------------- Private Function Prototypes

// ---------------- VTables

const wuffs_base__hasher_u32__func_ptrs
wuffs_crc32__ieee_hasher__func_ptrs_for__wuffs_base__hasher_u32 = {
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_crc32__ieee_hasher__set_quirk_enabled),
  (uint32_t(*)(void*,
      wuffs_base__slice_u8))(&wuffs_crc32__ieee_hasher__update_u32),
};

// ---------------- Initializer Implementations

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_crc32__ieee_hasher__initialize(
    wuffs_crc32__ieee_hasher* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags){
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (sizeof(*self) != sizeof_star_self) {
    return wuffs_base__make_status(wuffs_base__error__bad_sizeof_receiver);
  }
  if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) ||
      (((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {
    return wuffs_base__make_status(wuffs_base__error__bad_wuffs_version);
  }

  if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {
    // The whole point of this if-check is to detect an uninitialized *self.
    // We disable the warning on GCC. Clang-5.0 does not have this warning.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if (self->private_impl.magic != 0) {
      return wuffs_base__make_status(wuffs_base__error__initialize_falsely_claimed_already_zeroed);
    }
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  } else {
    if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {
      memset(self, 0, sizeof(*self));
      initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;
    } else {
      memset(&(self->private_impl), 0, sizeof(self->private_impl));
    }
  }

  self->private_impl.magic = WUFFS_BASE__MAGIC;
  self->private_impl.vtable_for__wuffs_base__hasher_u32.vtable_name =
      wuffs_base__hasher_u32__vtable_name;
  self->private_impl.vtable_for__wuffs_base__hasher_u32.function_pointers =
      (const void*)(&wuffs_crc32__ieee_hasher__func_ptrs_for__wuffs_base__hasher_u32);
  return wuffs_base__make_status(NULL);
}

wuffs_crc32__ieee_hasher*
wuffs_crc32__ieee_hasher__alloc() {
  wuffs_crc32__ieee_hasher* x =
      (wuffs_crc32__ieee_hasher*)(calloc(sizeof(wuffs_crc32__ieee_hasher), 1));
  if (!x) {
    return NULL;
  }
  if (wuffs_crc32__ieee_hasher__initialize(
      x, sizeof(wuffs_crc32__ieee_hasher), WUFFS_VERSION, WUFFS_INITIALIZE__ALREADY_ZEROED).repr) {
    free(x);
    return NULL;
  }
  return x;
}

size_t
sizeof__wuffs_crc32__ieee_hasher() {
  return sizeof(wuffs_crc32__ieee_hasher);
}

// ---------------- Function Implementations

// -------- func crc32.ieee_hasher.set_quirk_enabled

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_crc32__ieee_hasher__set_quirk_enabled(
    wuffs_crc32__ieee_hasher* self,
    uint32_t a_quirk,
    bool a_enabled) {
  return wuffs_base__make_empty_struct();
}

// -------- func crc32.ieee_hasher.update_u32

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_crc32__ieee_hasher__update_u32(
    wuffs_crc32__ieee_hasher* self,
    wuffs_base__slice_u8 a_x) {
  if (!self) {
    return 0;
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return 0;
  }

  uint32_t v_s = 0;
  wuffs_base__slice_u8 v_p = {0};

  v_s = (4294967295 ^ self->private_impl.f_state);
  {
    wuffs_base__slice_u8 i_slice_p = a_x;
    v_p = i_slice_p;
    v_p.len = 16;
    uint8_t* i_end0_p = i_slice_p.ptr + (i_slice_p.len / 32) * 32;
    while (v_p.ptr < i_end0_p) {
      v_s ^= ((((uint32_t)(v_p.ptr[0])) << 0) |
          (((uint32_t)(v_p.ptr[1])) << 8) |
          (((uint32_t)(v_p.ptr[2])) << 16) |
          (((uint32_t)(v_p.ptr[3])) << 24));
      v_s = (WUFFS_CRC32__IEEE_TABLE[0][v_p.ptr[15]] ^
          WUFFS_CRC32__IEEE_TABLE[1][v_p.ptr[14]] ^
          WUFFS_CRC32__IEEE_TABLE[2][v_p.ptr[13]] ^
          WUFFS_CRC32__IEEE_TABLE[3][v_p.ptr[12]] ^
          WUFFS_CRC32__IEEE_TABLE[4][v_p.ptr[11]] ^
          WUFFS_CRC32__IEEE_TABLE[5][v_p.ptr[10]] ^
          WUFFS_CRC32__IEEE_TABLE[6][v_p.ptr[9]] ^
          WUFFS_CRC32__IEEE_TABLE[7][v_p.ptr[8]] ^
          WUFFS_CRC32__IEEE_TABLE[8][v_p.ptr[7]] ^
          WUFFS_CRC32__IEEE_TABLE[9][v_p.ptr[6]] ^
          WUFFS_CRC32__IEEE_TABLE[10][v_p.ptr[5]] ^
          WUFFS_CRC32__IEEE_TABLE[11][v_p.ptr[4]] ^
          WUFFS_CRC32__IEEE_TABLE[12][(255 & (v_s >> 24))] ^
          WUFFS_CRC32__IEEE_TABLE[13][(255 & (v_s >> 16))] ^
          WUFFS_CRC32__IEEE_TABLE[14][(255 & (v_s >> 8))] ^
          WUFFS_CRC32__IEEE_TABLE[15][(255 & (v_s >> 0))]);
      v_p.ptr += 16;
      v_s ^= ((((uint32_t)(v_p.ptr[0])) << 0) |
          (((uint32_t)(v_p.ptr[1])) << 8) |
          (((uint32_t)(v_p.ptr[2])) << 16) |
          (((uint32_t)(v_p.ptr[3])) << 24));
      v_s = (WUFFS_CRC32__IEEE_TABLE[0][v_p.ptr[15]] ^
          WUFFS_CRC32__IEEE_TABLE[1][v_p.ptr[14]] ^
          WUFFS_CRC32__IEEE_TABLE[2][v_p.ptr[13]] ^
          WUFFS_CRC32__IEEE_TABLE[3][v_p.ptr[12]] ^
          WUFFS_CRC32__IEEE_TABLE[4][v_p.ptr[11]] ^
          WUFFS_CRC32__IEEE_TABLE[5][v_p.ptr[10]] ^
          WUFFS_CRC32__IEEE_TABLE[6][v_p.ptr[9]] ^
          WUFFS_CRC32__IEEE_TABLE[7][v_p.ptr[8]] ^
          WUFFS_CRC32__IEEE_TABLE[8][v_p.ptr[7]] ^
          WUFFS_CRC32__IEEE_TABLE[9][v_p.ptr[6]] ^
          WUFFS_CRC32__IEEE_TABLE[10][v_p.ptr[5]] ^
          WUFFS_CRC32__IEEE_TABLE[11][v_p.ptr[4]] ^
          WUFFS_CRC32__IEEE_TABLE[12][(255 & (v_s >> 24))] ^
          WUFFS_CRC32__IEEE_TABLE[13][(255 & (v_s >> 16))] ^
          WUFFS_CRC32__IEEE_TABLE[14][(255 & (v_s >> 8))] ^
          WUFFS_CRC32__IEEE_TABLE[15][(255 & (v_s >> 0))]);
      v_p.ptr += 16;
    }
    v_p.len = 16;
    uint8_t* i_end1_p = i_slice_p.ptr + (i_slice_p.len / 16) * 16;
    while (v_p.ptr < i_end1_p) {
      v_s ^= ((((uint32_t)(v_p.ptr[0])) << 0) |
          (((uint32_t)(v_p.ptr[1])) << 8) |
          (((uint32_t)(v_p.ptr[2])) << 16) |
          (((uint32_t)(v_p.ptr[3])) << 24));
      v_s = (WUFFS_CRC32__IEEE_TABLE[0][v_p.ptr[15]] ^
          WUFFS_CRC32__IEEE_TABLE[1][v_p.ptr[14]] ^
          WUFFS_CRC32__IEEE_TABLE[2][v_p.ptr[13]] ^
          WUFFS_CRC32__IEEE_TABLE[3][v_p.ptr[12]] ^
          WUFFS_CRC32__IEEE_TABLE[4][v_p.ptr[11]] ^
          WUFFS_CRC32__IEEE_TABLE[5][v_p.ptr[10]] ^
          WUFFS_CRC32__IEEE_TABLE[6][v_p.ptr[9]] ^
          WUFFS_CRC32__IEEE_TABLE[7][v_p.ptr[8]] ^
          WUFFS_CRC32__IEEE_TABLE[8][v_p.ptr[7]] ^
          WUFFS_CRC32__IEEE_TABLE[9][v_p.ptr[6]] ^
          WUFFS_CRC32__IEEE_TABLE[10][v_p.ptr[5]] ^
          WUFFS_CRC32__IEEE_TABLE[11][v_p.ptr[4]] ^
          WUFFS_CRC32__IEEE_TABLE[12][(255 & (v_s >> 24))] ^
          WUFFS_CRC32__IEEE_TABLE[13][(255 & (v_s >> 16))] ^
          WUFFS_CRC32__IEEE_TABLE[14][(255 & (v_s >> 8))] ^
          WUFFS_CRC32__IEEE_TABLE[15][(255 & (v_s >> 0))]);
      v_p.ptr += 16;
    }
    v_p.len = 1;
    uint8_t* i_end2_p = i_slice_p.ptr + (i_slice_p.len / 1) * 1;
    while (v_p.ptr < i_end2_p) {
      v_s = (WUFFS_CRC32__IEEE_TABLE[0][(((uint8_t)((v_s & 255))) ^ v_p.ptr[0])] ^ (v_s >> 8));
      v_p.ptr += 1;
    }
  }
  self->private_impl.f_state = (4294967295 ^ v_s);
  return self->private_impl.f_state;
}

#endif  // !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__CRC32)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__DEFLATE)

// ---------------- Status Codes Implementations

const char* wuffs_deflate__error__bad_huffman_code_over_subscribed = "#deflate: bad Huffman code (over-subscribed)";
const char* wuffs_deflate__error__bad_huffman_code_under_subscribed = "#deflate: bad Huffman code (under-subscribed)";
const char* wuffs_deflate__error__bad_huffman_code_length_count = "#deflate: bad Huffman code length count";
const char* wuffs_deflate__error__bad_huffman_code_length_repetition = "#deflate: bad Huffman code length repetition";
const char* wuffs_deflate__error__bad_huffman_code = "#deflate: bad Huffman code";
const char* wuffs_deflate__error__bad_huffman_minimum_code_length = "#deflate: bad Huffman minimum code length";
const char* wuffs_deflate__error__bad_block = "#deflate: bad block";
const char* wuffs_deflate__error__bad_distance = "#deflate: bad distance";
const char* wuffs_deflate__error__bad_distance_code_count = "#deflate: bad distance code count";
const char* wuffs_deflate__error__bad_literal_length_code_count = "#deflate: bad literal/length code count";
const char* wuffs_deflate__error__inconsistent_stored_block_length = "#deflate: inconsistent stored block length";
const char* wuffs_deflate__error__missing_end_of_block_code = "#deflate: missing end-of-block code";
const char* wuffs_deflate__error__no_huffman_codes = "#deflate: no Huffman codes";
const char* wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state = "#deflate: internal error: inconsistent Huffman decoder state";
const char* wuffs_deflate__error__internal_error_inconsistent_i_o = "#deflate: internal error: inconsistent I/O";
const char* wuffs_deflate__error__internal_error_inconsistent_distance = "#deflate: internal error: inconsistent distance";
const char* wuffs_deflate__error__internal_error_inconsistent_n_bits = "#deflate: internal error: inconsistent n_bits";

// ---------------- Private Consts

static const uint8_t
WUFFS_DEFLATE__CODE_ORDER[19]WUFFS_BASE__POTENTIALLY_UNUSED = {
  16, 17, 18, 0, 8, 7, 9, 6,
  10, 5, 11, 4, 12, 3, 13, 2,
  14, 1, 15,
};

static const uint8_t
WUFFS_DEFLATE__REVERSE8[256]WUFFS_BASE__POTENTIALLY_UNUSED = {
  0, 128, 64, 192, 32, 160, 96, 224,
  16, 144, 80, 208, 48, 176, 112, 240,
  8, 136, 72, 200, 40, 168, 104, 232,
  24, 152, 88, 216, 56, 184, 120, 248,
  4, 132, 68, 196, 36, 164, 100, 228,
  20, 148, 84, 212, 52, 180, 116, 244,
  12, 140, 76, 204, 44, 172, 108, 236,
  28, 156, 92, 220, 60, 188, 124, 252,
  2, 130, 66, 194, 34, 162, 98, 226,
  18, 146, 82, 210, 50, 178, 114, 242,
  10, 138, 74, 202, 42, 170, 106, 234,
  26, 154, 90, 218, 58, 186, 122, 250,
  6, 134, 70, 198, 38, 166, 102, 230,
  22, 150, 86, 214, 54, 182, 118, 246,
  14, 142, 78, 206, 46, 174, 110, 238,
  30, 158, 94, 222, 62, 190, 126, 254,
  1, 129, 65, 193, 33, 161, 97, 225,
  17, 145, 81, 209, 49, 177, 113, 241,
  9, 137, 73, 201, 41, 169, 105, 233,
  25, 153, 89, 217, 57, 185, 121, 249,
  5, 133, 69, 197, 37, 165, 101, 229,
  21, 149, 85, 213, 53, 181, 117, 245,
  13, 141, 77, 205, 45, 173, 109, 237,
  29, 157, 93, 221, 61, 189, 125, 253,
  3, 131, 67, 195, 35, 163, 99, 227,
  19, 147, 83, 211, 51, 179, 115, 243,
  11, 139, 75, 203, 43, 171, 107, 235,
  27, 155, 91, 219, 59, 187, 123, 251,
  7, 135, 71, 199, 39, 167, 103, 231,
  23, 151, 87, 215, 55, 183, 119, 247,
  15, 143, 79, 207, 47, 175, 111, 239,
  31, 159, 95, 223, 63, 191, 127, 255,
};

static const uint32_t
WUFFS_DEFLATE__LCODE_MAGIC_NUMBERS[32]WUFFS_BASE__POTENTIALLY_UNUSED = {
  1073741824, 1073742080, 1073742336, 1073742592, 1073742848, 1073743104, 1073743360, 1073743616,
  1073743888, 1073744400, 1073744912, 1073745424, 1073745952, 1073746976, 1073748000, 1073749024,
  1073750064, 1073752112, 1073754160, 1073756208, 1073758272, 1073762368, 1073766464, 1073770560,
  1073774672, 1073782864, 1073791056, 1073799248, 1073807104, 134217728, 134217728, 134217728,
};

static const uint32_t
WUFFS_DEFLATE__DCODE_MAGIC_NUMBERS[32]WUFFS_BASE__POTENTIALLY_UNUSED = {
  1073741824, 1073742080, 1073742336, 1073742592, 1073742864, 1073743376, 1073743904, 1073744928,
  1073745968, 1073748016, 1073750080, 1073754176, 1073758288, 1073766480, 1073774688, 1073791072,
  1073807472, 1073840240, 1073873024, 1073938560, 1074004112, 1074135184, 1074266272, 1074528416,
  1074790576, 1075314864, 1075839168, 1076887744, 1077936336, 1080033488, 134217728, 134217728,
};

#define WUFFS_DEFLATE__HUFFS_TABLE_SIZE 1024

#define WUFFS_DEFLATE__HUFFS_TABLE_MASK 1023

// ---------------- Private Initializer Prototypes

// ---------------- Private Function Prototypes

static wuffs_base__status
wuffs_deflate__decoder__decode_blocks(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_deflate__decoder__decode_uncompressed(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_deflate__decoder__init_fixed_huffman(
    wuffs_deflate__decoder* self);

static wuffs_base__status
wuffs_deflate__decoder__init_dynamic_huffman(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_deflate__decoder__init_huff(
    wuffs_deflate__decoder* self,
    uint32_t a_which,
    uint32_t a_n_codes0,
    uint32_t a_n_codes1,
    uint32_t a_base_symbol);

static wuffs_base__status
wuffs_deflate__decoder__decode_huffman_fast(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_deflate__decoder__decode_huffman_slow(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src);

// ---------------- VTables

const wuffs_base__io_transformer__func_ptrs
wuffs_deflate__decoder__func_ptrs_for__wuffs_base__io_transformer = {
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_deflate__decoder__set_quirk_enabled),
  (wuffs_base__status(*)(void*,
      wuffs_base__io_buffer*,
      wuffs_base__io_buffer*,
      wuffs_base__slice_u8))(&wuffs_deflate__decoder__transform_io),
  (wuffs_base__range_ii_u64(*)(const void*))(&wuffs_deflate__decoder__workbuf_len),
};

// ---------------- Initializer Implementations

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_deflate__decoder__initialize(
    wuffs_deflate__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags){
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (sizeof(*self) != sizeof_star_self) {
    return wuffs_base__make_status(wuffs_base__error__bad_sizeof_receiver);
  }
  if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) ||
      (((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {
    return wuffs_base__make_status(wuffs_base__error__bad_wuffs_version);
  }

  if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {
    // The whole point of this if-check is to detect an uninitialized *self.
    // We disable the warning on GCC. Clang-5.0 does not have this warning.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if (self->private_impl.magic != 0) {
      return wuffs_base__make_status(wuffs_base__error__initialize_falsely_claimed_already_zeroed);
    }
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  } else {
    if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {
      memset(self, 0, sizeof(*self));
      initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;
    } else {
      memset(&(self->private_impl), 0, sizeof(self->private_impl));
    }
  }

  self->private_impl.magic = WUFFS_BASE__MAGIC;
  self->private_impl.vtable_for__wuffs_base__io_transformer.vtable_name =
      wuffs_base__io_transformer__vtable_name;
  self->private_impl.vtable_for__wuffs_base__io_transformer.function_pointers =
      (const void*)(&wuffs_deflate__decoder__func_ptrs_for__wuffs_base__io_transformer);
  return wuffs_base__make_status(NULL);
}

wuffs_deflate__decoder*
wuffs_deflate__decoder__alloc() {
  wuffs_deflate__decoder* x =
      (wuffs_deflate__decoder*)(calloc(sizeof(wuffs_deflate__decoder), 1));
  if (!x) {
    return NULL;
  }
  if (wuffs_deflate__decoder__initialize(
      x, sizeof(wuffs_deflate__decoder), WUFFS_VERSION, WUFFS_INITIALIZE__ALREADY_ZEROED).repr) {
    free(x);
    return NULL;
  }
  return x;
}

size_t
sizeof__wuffs_deflate__decoder() {
  return sizeof(wuffs_deflate__decoder);
}

// ---------------- Function Implementations

// -------- func deflate.decoder.add_history

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_deflate__decoder__add_history(
    wuffs_deflate__decoder* self,
    wuffs_base__slice_u8 a_hist) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  wuffs_base__slice_u8 v_s = {0};
  uint64_t v_n_copied = 0;
  uint32_t v_already_full = 0;

  v_s = a_hist;
  if (((uint64_t)(v_s.len)) >= 32768) {
    v_s = wuffs_base__slice_u8__suffix(v_s, 32768);
    wuffs_base__slice_u8__copy_from_slice(wuffs_base__slice_u8__subslice_j(wuffs_base__make_slice_u8(self->private_data.f_history, 33025), 32768), v_s);
    self->private_impl.f_history_index = 32768;
  } else {
    v_n_copied = wuffs_base__slice_u8__copy_from_slice(wuffs_base__slice_u8__subslice_ij(wuffs_base__make_slice_u8(self->private_data.f_history, 33025), (self->private_impl.f_history_index & 32767), 32768), v_s);
    if (v_n_copied < ((uint64_t)(v_s.len))) {
      v_s = wuffs_base__slice_u8__subslice_i(v_s, v_n_copied);
      v_n_copied = wuffs_base__slice_u8__copy_from_slice(wuffs_base__slice_u8__subslice_j(wuffs_base__make_slice_u8(self->private_data.f_history, 33025), 32768), v_s);
      self->private_impl.f_history_index = (((uint32_t)((v_n_copied & 32767))) + 32768);
    } else {
      v_already_full = 0;
      if (self->private_impl.f_history_index >= 32768) {
        v_already_full = 32768;
      }
      self->private_impl.f_history_index = ((self->private_impl.f_history_index & 32767) + ((uint32_t)((v_n_copied & 32767))) + v_already_full);
    }
  }
  wuffs_base__slice_u8__copy_from_slice(wuffs_base__slice_u8__subslice_i(wuffs_base__make_slice_u8(self->private_data.f_history, 33025), 32768), wuffs_base__make_slice_u8(self->private_data.f_history, 33025));
  return wuffs_base__make_empty_struct();
}

// -------- func deflate.decoder.set_quirk_enabled

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_deflate__decoder__set_quirk_enabled(
    wuffs_deflate__decoder* self,
    uint32_t a_quirk,
    bool a_enabled) {
  return wuffs_base__make_empty_struct();
}

// -------- func deflate.decoder.workbuf_len

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_deflate__decoder__workbuf_len(
    const wuffs_deflate__decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  return wuffs_base__utility__make_range_ii_u64(1, 1);
}

// -------- func deflate.decoder.transform_io

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_deflate__decoder__transform_io(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 1)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint64_t v_mark = 0;
  wuffs_base__status v_status = wuffs_base__make_status(NULL);

  uint8_t* iop_a_dst = NULL;
  uint8_t* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }

  uint32_t coro_susp_point = self->private_impl.p_transform_io[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    while (true) {
      v_mark = ((uint64_t)(iop_a_dst - io0_a_dst));
      {
        if (a_dst) {
          a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
        }
        wuffs_base__status t_0 = wuffs_deflate__decoder__decode_blocks(self, a_dst, a_src);
        if (a_dst) {
          iop_a_dst = a_dst->data.ptr + a_dst->meta.wi;
        }
        v_status = t_0;
      }
      if ( ! wuffs_base__status__is_suspension(&v_status)) {
        status = v_status;
        if (wuffs_base__status__is_error(&status)) {
          goto exit;
        } else if (wuffs_base__status__is_suspension(&status)) {
          status = wuffs_base__make_status(wuffs_base__error__cannot_return_a_suspension);
          goto exit;
        }
        goto ok;
      }
      wuffs_deflate__decoder__add_history(self, wuffs_base__io__since(v_mark, ((uint64_t)(iop_a_dst - io0_a_dst)), io0_a_dst));
      status = v_status;
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(1);
    }

    goto ok;
    ok:
    self->private_impl.p_transform_io[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_transform_io[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 1 : 0;

  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func deflate.decoder.decode_blocks

static wuffs_base__status
wuffs_deflate__decoder__decode_blocks(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t v_final = 0;
  uint32_t v_b0 = 0;
  uint32_t v_type = 0;
  wuffs_base__status v_status = wuffs_base__make_status(NULL);

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_blocks[0];
  if (coro_susp_point) {
    v_final = self->private_data.s_decode_blocks[0].v_final;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    label__outer__continue:;
    while (v_final == 0) {
      while (self->private_impl.f_n_bits < 3) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint32_t t_0 = *iop_a_src++;
          v_b0 = t_0;
        }
        self->private_impl.f_bits |= (v_b0 << self->private_impl.f_n_bits);
        self->private_impl.f_n_bits += 8;
      }
      v_final = (self->private_impl.f_bits & 1);
      v_type = ((self->private_impl.f_bits >> 1) & 3);
      self->private_impl.f_bits >>= 3;
      self->private_impl.f_n_bits -= 3;
      if (v_type == 0) {
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        status = wuffs_deflate__decoder__decode_uncompressed(self, a_dst, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (status.repr) {
          goto suspend;
        }
        goto label__outer__continue;
      } else if (v_type == 1) {
        v_status = wuffs_deflate__decoder__init_fixed_huffman(self);
        if ( ! wuffs_base__status__is_ok(&v_status)) {
          status = v_status;
          if (wuffs_base__status__is_error(&status)) {
            goto exit;
          } else if (wuffs_base__status__is_suspension(&status)) {
            status = wuffs_base__make_status(wuffs_base__error__cannot_return_a_suspension);
            goto exit;
          }
          goto ok;
        }
      } else if (v_type == 2) {
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
        status = wuffs_deflate__decoder__init_dynamic_huffman(self, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (status.repr) {
          goto suspend;
        }
      } else {
        status = wuffs_base__make_status(wuffs_deflate__error__bad_block);
        goto exit;
      }
      self->private_impl.f_end_of_block = false;
      while (true) {
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        v_status = wuffs_deflate__decoder__decode_huffman_fast(self, a_dst, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (wuffs_base__status__is_error(&v_status)) {
          status = v_status;
          goto exit;
        }
        if (self->private_impl.f_end_of_block) {
          goto label__outer__continue;
        }
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
        status = wuffs_deflate__decoder__decode_huffman_slow(self, a_dst, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (status.repr) {
          goto suspend;
        }
        if (self->private_impl.f_end_of_block) {
          goto label__outer__continue;
        }
      }
    }

    goto ok;
    ok:
    self->private_impl.p_decode_blocks[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_blocks[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_data.s_decode_blocks[0].v_final = v_final;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func deflate.decoder.decode_uncompressed

static wuffs_base__status
wuffs_deflate__decoder__decode_uncompressed(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t v_length = 0;
  uint32_t v_n_copied = 0;

  uint8_t* iop_a_dst = NULL;
  uint8_t* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }
  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_uncompressed[0];
  if (coro_susp_point) {
    v_length = self->private_data.s_decode_uncompressed[0].v_length;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if ((self->private_impl.f_n_bits >= 8) || ((self->private_impl.f_bits >> (self->private_impl.f_n_bits & 7)) != 0)) {
      status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_n_bits);
      goto exit;
    }
    self->private_impl.f_n_bits = 0;
    self->private_impl.f_bits = 0;
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      uint32_t t_0;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
        t_0 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
        iop_a_src += 4;
      } else {
        self->private_data.s_decode_uncompressed[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_uncompressed[0].scratch;
          uint32_t num_bits_0 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_0;
          if (num_bits_0 == 24) {
            t_0 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_0 += 8;
          *scratch |= ((uint64_t)(num_bits_0)) << 56;
        }
      }
      v_length = t_0;
    }
    if ((((v_length) & 0xFFFF) + ((v_length) >> (32 - (16)))) != 65535) {
      status = wuffs_base__make_status(wuffs_deflate__error__inconsistent_stored_block_length);
      goto exit;
    }
    v_length = ((v_length) & 0xFFFF);
    while (true) {
      v_n_copied = wuffs_base__io_writer__limited_copy_u32_from_reader(
          &iop_a_dst, io2_a_dst,v_length, &iop_a_src, io2_a_src);
      if (v_length <= v_n_copied) {
        status = wuffs_base__make_status(NULL);
        goto ok;
      }
      v_length -= v_n_copied;
      if (((uint64_t)(io2_a_dst - iop_a_dst)) == 0) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_write);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(3);
      } else {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(4);
      }
    }

    goto ok;
    ok:
    self->private_impl.p_decode_uncompressed[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_uncompressed[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_data.s_decode_uncompressed[0].v_length = v_length;

  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func deflate.decoder.init_fixed_huffman

static wuffs_base__status
wuffs_deflate__decoder__init_fixed_huffman(
    wuffs_deflate__decoder* self) {
  uint32_t v_i = 0;
  wuffs_base__status v_status = wuffs_base__make_status(NULL);

  while (v_i < 144) {
    self->private_data.f_code_lengths[v_i] = 8;
    v_i += 1;
  }
  while (v_i < 256) {
    self->private_data.f_code_lengths[v_i] = 9;
    v_i += 1;
  }
  while (v_i < 280) {
    self->private_data.f_code_lengths[v_i] = 7;
    v_i += 1;
  }
  while (v_i < 288) {
    self->private_data.f_code_lengths[v_i] = 8;
    v_i += 1;
  }
  while (v_i < 320) {
    self->private_data.f_code_lengths[v_i] = 5;
    v_i += 1;
  }
  v_status = wuffs_deflate__decoder__init_huff(self,
      0,
      0,
      288,
      257);
  if (wuffs_base__status__is_error(&v_status)) {
    return v_status;
  }
  v_status = wuffs_deflate__decoder__init_huff(self,
      1,
      288,
      320,
      0);
  if (wuffs_base__status__is_error(&v_status)) {
    return v_status;
  }
  return wuffs_base__make_status(NULL);
}

// -------- func deflate.decoder.init_dynamic_huffman

static wuffs_base__status
wuffs_deflate__decoder__init_dynamic_huffman(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t v_bits = 0;
  uint32_t v_n_bits = 0;
  uint32_t v_b0 = 0;
  uint32_t v_n_lit = 0;
  uint32_t v_n_dist = 0;
  uint32_t v_n_clen = 0;
  uint32_t v_i = 0;
  uint32_t v_b1 = 0;
  wuffs_base__status v_status = wuffs_base__make_status(NULL);
  uint32_t v_mask = 0;
  uint32_t v_table_entry = 0;
  uint32_t v_table_entry_n_bits = 0;
  uint32_t v_b2 = 0;
  uint32_t v_n_extra_bits = 0;
  uint8_t v_rep_symbol = 0;
  uint32_t v_rep_count = 0;
  uint32_t v_b3 = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_init_dynamic_huffman[0];
  if (coro_susp_point) {
    v_bits = self->private_data.s_init_dynamic_huffman[0].v_bits;
    v_n_bits = self->private_data.s_init_dynamic_huffman[0].v_n_bits;
    v_n_lit = self->private_data.s_init_dynamic_huffman[0].v_n_lit;
    v_n_dist = self->private_data.s_init_dynamic_huffman[0].v_n_dist;
    v_n_clen = self->private_data.s_init_dynamic_huffman[0].v_n_clen;
    v_i = self->private_data.s_init_dynamic_huffman[0].v_i;
    v_mask = self->private_data.s_init_dynamic_huffman[0].v_mask;
    v_table_entry = self->private_data.s_init_dynamic_huffman[0].v_table_entry;
    v_n_extra_bits = self->private_data.s_init_dynamic_huffman[0].v_n_extra_bits;
    v_rep_symbol = self->private_data.s_init_dynamic_huffman[0].v_rep_symbol;
    v_rep_count = self->private_data.s_init_dynamic_huffman[0].v_rep_count;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    v_bits = self->private_impl.f_bits;
    v_n_bits = self->private_impl.f_n_bits;
    while (v_n_bits < 14) {
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        uint32_t t_0 = *iop_a_src++;
        v_b0 = t_0;
      }
      v_bits |= (v_b0 << v_n_bits);
      v_n_bits += 8;
    }
    v_n_lit = (((v_bits) & 0x1F) + 257);
    if (v_n_lit > 286) {
      status = wuffs_base__make_status(wuffs_deflate__error__bad_literal_length_code_count);
      goto exit;
    }
    v_bits >>= 5;
    v_n_dist = (((v_bits) & 0x1F) + 1);
    if (v_n_dist > 30) {
      status = wuffs_base__make_status(wuffs_deflate__error__bad_distance_code_count);
      goto exit;
    }
    v_bits >>= 5;
    v_n_clen = (((v_bits) & 0xF) + 4);
    v_bits >>= 4;
    v_n_bits -= 14;
    v_i = 0;
    while (v_i < v_n_clen) {
      while (v_n_bits < 3) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint32_t t_1 = *iop_a_src++;
          v_b1 = t_1;
        }
        v_bits |= (v_b1 << v_n_bits);
        v_n_bits += 8;
      }
      self->private_data.f_code_lengths[WUFFS_DEFLATE__CODE_ORDER[v_i]] = ((uint8_t)((v_bits & 7)));
      v_bits >>= 3;
      v_n_bits -= 3;
      v_i += 1;
    }
    while (v_i < 19) {
      self->private_data.f_code_lengths[WUFFS_DEFLATE__CODE_ORDER[v_i]] = 0;
      v_i += 1;
    }
    v_status = wuffs_deflate__decoder__init_huff(self,
        0,
        0,
        19,
        4095);
    if (wuffs_base__status__is_error(&v_status)) {
      status = v_status;
      goto exit;
    }
    v_mask = ((((uint32_t)(1)) << self->private_impl.f_n_huffs_bits[0]) - 1);
    v_i = 0;
    label__0__continue:;
    while (v_i < (v_n_lit + v_n_dist)) {
      while (true) {
        v_table_entry = self->private_data.f_huffs[0][(v_bits & v_mask)];
        v_table_entry_n_bits = (v_table_entry & 15);
        if (v_n_bits >= v_table_entry_n_bits) {
          v_bits >>= v_table_entry_n_bits;
          v_n_bits -= v_table_entry_n_bits;
          goto label__1__break;
        }
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint32_t t_2 = *iop_a_src++;
          v_b2 = t_2;
        }
        v_bits |= (v_b2 << v_n_bits);
        v_n_bits += 8;
      }
      label__1__break:;
      if ((v_table_entry >> 24) != 128) {
        status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
        goto exit;
      }
      v_table_entry = ((v_table_entry >> 8) & 255);
      if (v_table_entry < 16) {
        self->private_data.f_code_lengths[v_i] = ((uint8_t)(v_table_entry));
        v_i += 1;
        goto label__0__continue;
      }
      v_n_extra_bits = 0;
      v_rep_symbol = 0;
      v_rep_count = 0;
      if (v_table_entry == 16) {
        v_n_extra_bits = 2;
        if (v_i <= 0) {
          status = wuffs_base__make_status(wuffs_deflate__error__bad_huffman_code_length_repetition);
          goto exit;
        }
        v_rep_symbol = (self->private_data.f_code_lengths[(v_i - 1)] & 15);
        v_rep_count = 3;
      } else if (v_table_entry == 17) {
        v_n_extra_bits = 3;
        v_rep_symbol = 0;
        v_rep_count = 3;
      } else if (v_table_entry == 18) {
        v_n_extra_bits = 7;
        v_rep_symbol = 0;
        v_rep_count = 11;
      } else {
        status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
        goto exit;
      }
      while (v_n_bits < v_n_extra_bits) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint32_t t_3 = *iop_a_src++;
          v_b3 = t_3;
        }
        v_bits |= (v_b3 << v_n_bits);
        v_n_bits += 8;
      }
      v_rep_count += ((v_bits) & WUFFS_BASE__LOW_BITS_MASK__U32(v_n_extra_bits));
      v_bits >>= v_n_extra_bits;
      v_n_bits -= v_n_extra_bits;
      while (v_rep_count > 0) {
        if (v_i >= (v_n_lit + v_n_dist)) {
          status = wuffs_base__make_status(wuffs_deflate__error__bad_huffman_code_length_count);
          goto exit;
        }
        self->private_data.f_code_lengths[v_i] = v_rep_symbol;
        v_i += 1;
        v_rep_count -= 1;
      }
    }
    if (v_i != (v_n_lit + v_n_dist)) {
      status = wuffs_base__make_status(wuffs_deflate__error__bad_huffman_code_length_count);
      goto exit;
    }
    if (self->private_data.f_code_lengths[256] == 0) {
      status = wuffs_base__make_status(wuffs_deflate__error__missing_end_of_block_code);
      goto exit;
    }
    v_status = wuffs_deflate__decoder__init_huff(self,
        0,
        0,
        v_n_lit,
        257);
    if (wuffs_base__status__is_error(&v_status)) {
      status = v_status;
      goto exit;
    }
    v_status = wuffs_deflate__decoder__init_huff(self,
        1,
        v_n_lit,
        (v_n_lit + v_n_dist),
        0);
    if (wuffs_base__status__is_error(&v_status)) {
      status = v_status;
      goto exit;
    }
    self->private_impl.f_bits = v_bits;
    self->private_impl.f_n_bits = v_n_bits;

    goto ok;
    ok:
    self->private_impl.p_init_dynamic_huffman[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_init_dynamic_huffman[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_data.s_init_dynamic_huffman[0].v_bits = v_bits;
  self->private_data.s_init_dynamic_huffman[0].v_n_bits = v_n_bits;
  self->private_data.s_init_dynamic_huffman[0].v_n_lit = v_n_lit;
  self->private_data.s_init_dynamic_huffman[0].v_n_dist = v_n_dist;
  self->private_data.s_init_dynamic_huffman[0].v_n_clen = v_n_clen;
  self->private_data.s_init_dynamic_huffman[0].v_i = v_i;
  self->private_data.s_init_dynamic_huffman[0].v_mask = v_mask;
  self->private_data.s_init_dynamic_huffman[0].v_table_entry = v_table_entry;
  self->private_data.s_init_dynamic_huffman[0].v_n_extra_bits = v_n_extra_bits;
  self->private_data.s_init_dynamic_huffman[0].v_rep_symbol = v_rep_symbol;
  self->private_data.s_init_dynamic_huffman[0].v_rep_count = v_rep_count;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func deflate.decoder.init_huff

static wuffs_base__status
wuffs_deflate__decoder__init_huff(
    wuffs_deflate__decoder* self,
    uint32_t a_which,
    uint32_t a_n_codes0,
    uint32_t a_n_codes1,
    uint32_t a_base_symbol) {
  uint16_t v_counts[16] = {0};
  uint32_t v_i = 0;
  uint32_t v_remaining = 0;
  uint16_t v_offsets[16] = {0};
  uint32_t v_n_symbols = 0;
  uint32_t v_count = 0;
  uint16_t v_symbols[320] = {0};
  uint32_t v_min_cl = 0;
  uint32_t v_max_cl = 0;
  uint32_t v_initial_high_bits = 0;
  uint32_t v_prev_cl = 0;
  uint32_t v_prev_redirect_key = 0;
  uint32_t v_top = 0;
  uint32_t v_next_top = 0;
  uint32_t v_code = 0;
  uint32_t v_key = 0;
  uint32_t v_value = 0;
  uint32_t v_cl = 0;
  uint32_t v_redirect_key = 0;
  uint32_t v_j = 0;
  uint32_t v_reversed_key = 0;
  uint32_t v_symbol = 0;
  uint32_t v_high_bits = 0;
  uint32_t v_delta = 0;

  v_i = a_n_codes0;
  while (v_i < a_n_codes1) {
    if (v_counts[(self->private_data.f_code_lengths[v_i] & 15)] >= 320) {
      return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
    }
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
    v_counts[(self->private_data.f_code_lengths[v_i] & 15)] += 1;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    v_i += 1;
  }
  if ((((uint32_t)(v_counts[0])) + a_n_codes0) == a_n_codes1) {
    return wuffs_base__make_status(wuffs_deflate__error__no_huffman_codes);
  }
  v_remaining = 1;
  v_i = 1;
  while (v_i <= 15) {
    if (v_remaining > 1073741824) {
      return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
    }
    v_remaining <<= 1;
    if (v_remaining < ((uint32_t)(v_counts[v_i]))) {
      return wuffs_base__make_status(wuffs_deflate__error__bad_huffman_code_over_subscribed);
    }
    v_remaining -= ((uint32_t)(v_counts[v_i]));
    v_i += 1;
  }
  if (v_remaining != 0) {
    if ((a_which == 1) &&
        (v_counts[1] == 1) &&
        (self->private_data.f_code_lengths[a_n_codes0] == 1) &&
        ((((uint32_t)(v_counts[0])) + a_n_codes0 + 1) == a_n_codes1)) {
      self->private_impl.f_n_huffs_bits[1] = 1;
      self->private_data.f_huffs[1][0] = (WUFFS_DEFLATE__DCODE_MAGIC_NUMBERS[0] | 1);
      self->private_data.f_huffs[1][1] = (WUFFS_DEFLATE__DCODE_MAGIC_NUMBERS[31] | 1);
      return wuffs_base__make_status(NULL);
    }
    return wuffs_base__make_status(wuffs_deflate__error__bad_huffman_code_under_subscribed);
  }
  v_i = 1;
  while (v_i <= 15) {
    v_offsets[v_i] = ((uint16_t)(v_n_symbols));
    v_count = ((uint32_t)(v_counts[v_i]));
    if (v_n_symbols > (320 - v_count)) {
      return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
    }
    v_n_symbols = (v_n_symbols + v_count);
    v_i += 1;
  }
  if (v_n_symbols > 288) {
    return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
  }
  v_i = a_n_codes0;
  while (v_i < a_n_codes1) {
    if (v_i < a_n_codes0) {
      return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
    }
    if (self->private_data.f_code_lengths[v_i] != 0) {
      if (v_offsets[(self->private_data.f_code_lengths[v_i] & 15)] >= 320) {
        return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
      }
      v_symbols[v_offsets[(self->private_data.f_code_lengths[v_i] & 15)]] = ((uint16_t)((v_i - a_n_codes0)));
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
      v_offsets[(self->private_data.f_code_lengths[v_i] & 15)] += 1;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    }
    v_i += 1;
  }
  v_min_cl = 1;
  while (true) {
    if (v_counts[v_min_cl] != 0) {
      goto label__0__break;
    }
    if (v_min_cl >= 9) {
      return wuffs_base__make_status(wuffs_deflate__error__bad_huffman_minimum_code_length);
    }
    v_min_cl += 1;
  }
  label__0__break:;
  v_max_cl = 15;
  while (true) {
    if (v_counts[v_max_cl] != 0) {
      goto label__1__break;
    }
    if (v_max_cl <= 1) {
      return wuffs_base__make_status(wuffs_deflate__error__no_huffman_codes);
    }
    v_max_cl -= 1;
  }
  label__1__break:;
  if (v_max_cl <= 9) {
    self->private_impl.f_n_huffs_bits[a_which] = v_max_cl;
  } else {
    self->private_impl.f_n_huffs_bits[a_which] = 9;
  }
  v_i = 0;
  if ((v_n_symbols != ((uint32_t)(v_offsets[v_max_cl]))) || (v_n_symbols != ((uint32_t)(v_offsets[15])))) {
    return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
  }
  if ((a_n_codes0 + ((uint32_t)(v_symbols[0]))) >= 320) {
    return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
  }
  v_initial_high_bits = 512;
  if (v_max_cl < 9) {
    v_initial_high_bits = (((uint32_t)(1)) << v_max_cl);
  }
  v_prev_cl = ((uint32_t)((self->private_data.f_code_lengths[(a_n_codes0 + ((uint32_t)(v_symbols[0])))] & 15)));
  v_prev_redirect_key = 4294967295;
  v_top = 0;
  v_next_top = 512;
  v_code = 0;
  v_key = 0;
  v_value = 0;
  while (true) {
    if ((a_n_codes0 + ((uint32_t)(v_symbols[v_i]))) >= 320) {
      return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
    }
    v_cl = ((uint32_t)((self->private_data.f_code_lengths[(a_n_codes0 + ((uint32_t)(v_symbols[v_i])))] & 15)));
    if (v_cl > v_prev_cl) {
      v_code <<= (v_cl - v_prev_cl);
      if (v_code >= 32768) {
        return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
      }
    }
    v_prev_cl = v_cl;
    v_key = v_code;
    if (v_cl > 9) {
      v_cl -= 9;
      v_redirect_key = ((v_key >> v_cl) & 511);
      v_key = ((v_key) & WUFFS_BASE__LOW_BITS_MASK__U32(v_cl));
      if (v_prev_redirect_key != v_redirect_key) {
        v_prev_redirect_key = v_redirect_key;
        v_remaining = (((uint32_t)(1)) << v_cl);
        v_j = v_prev_cl;
        while (v_j <= 15) {
          if (v_remaining <= ((uint32_t)(v_counts[v_j]))) {
            goto label__2__break;
          }
          v_remaining -= ((uint32_t)(v_counts[v_j]));
          if (v_remaining > 1073741824) {
            return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
          }
          v_remaining <<= 1;
          v_j += 1;
        }
        label__2__break:;
        if ((v_j <= 9) || (15 < v_j)) {
          return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
        }
        v_j -= 9;
        v_initial_high_bits = (((uint32_t)(1)) << v_j);
        v_top = v_next_top;
        if ((v_top + (((uint32_t)(1)) << v_j)) > 1024) {
          return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
        }
        v_next_top = (v_top + (((uint32_t)(1)) << v_j));
        v_redirect_key = (((uint32_t)(WUFFS_DEFLATE__REVERSE8[(v_redirect_key >> 1)])) | ((v_redirect_key & 1) << 8));
        self->private_data.f_huffs[a_which][v_redirect_key] = (268435465 | (v_top << 8) | (v_j << 4));
      }
    }
    if ((v_key >= 512) || (v_counts[v_prev_cl] <= 0)) {
      return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
    }
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
    v_counts[v_prev_cl] -= 1;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    v_reversed_key = (((uint32_t)(WUFFS_DEFLATE__REVERSE8[(v_key >> 1)])) | ((v_key & 1) << 8));
    v_reversed_key >>= (9 - v_cl);
    v_symbol = ((uint32_t)(v_symbols[v_i]));
    if (v_symbol == 256) {
      v_value = (536870912 | v_cl);
    } else if ((v_symbol < 256) && (a_which == 0)) {
      v_value = (2147483648 | (v_symbol << 8) | v_cl);
    } else if (v_symbol >= a_base_symbol) {
      v_symbol -= a_base_symbol;
      if (a_which == 0) {
        v_value = (WUFFS_DEFLATE__LCODE_MAGIC_NUMBERS[(v_symbol & 31)] | v_cl);
      } else {
        v_value = (WUFFS_DEFLATE__DCODE_MAGIC_NUMBERS[(v_symbol & 31)] | v_cl);
      }
    } else {
      return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
    }
    v_high_bits = v_initial_high_bits;
    v_delta = (((uint32_t)(1)) << v_cl);
    while (v_high_bits >= v_delta) {
      v_high_bits -= v_delta;
      if ((v_top + ((v_high_bits | v_reversed_key) & 511)) >= 1024) {
        return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
      }
      self->private_data.f_huffs[a_which][(v_top + ((v_high_bits | v_reversed_key) & 511))] = v_value;
    }
    v_i += 1;
    if (v_i >= v_n_symbols) {
      goto label__3__break;
    }
    v_code += 1;
    if (v_code >= 32768) {
      return wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
    }
  }
  label__3__break:;
  return wuffs_base__make_status(NULL);
}

// -------- func deflate.decoder.decode_huffman_fast

static wuffs_base__status
wuffs_deflate__decoder__decode_huffman_fast(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t v_bits = 0;
  uint32_t v_n_bits = 0;
  uint32_t v_table_entry = 0;
  uint32_t v_table_entry_n_bits = 0;
  uint32_t v_lmask = 0;
  uint32_t v_dmask = 0;
  uint32_t v_redir_top = 0;
  uint32_t v_redir_mask = 0;
  uint32_t v_length = 0;
  uint32_t v_dist_minus_1 = 0;
  uint32_t v_hlen = 0;
  uint32_t v_hdist = 0;

  uint8_t* iop_a_dst = NULL;
  uint8_t* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }
  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  if ((self->private_impl.f_n_bits >= 8) || ((self->private_impl.f_bits >> (self->private_impl.f_n_bits & 7)) != 0)) {
    status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_n_bits);
    goto exit;
  }
  v_bits = self->private_impl.f_bits;
  v_n_bits = self->private_impl.f_n_bits;
  v_lmask = ((((uint32_t)(1)) << self->private_impl.f_n_huffs_bits[0]) - 1);
  v_dmask = ((((uint32_t)(1)) << self->private_impl.f_n_huffs_bits[1]) - 1);
  label__loop__continue:;
  while ((((uint64_t)(io2_a_dst - iop_a_dst)) >= 258) && (((uint64_t)(io2_a_src - iop_a_src)) >= 12)) {
    if (v_n_bits < 15) {
      v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
      (iop_a_src += 1, wuffs_base__make_empty_struct());
      v_n_bits += 8;
      v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
      (iop_a_src += 1, wuffs_base__make_empty_struct());
      v_n_bits += 8;
    } else {
    }
    v_table_entry = self->private_data.f_huffs[0][(v_bits & v_lmask)];
    v_table_entry_n_bits = (v_table_entry & 15);
    v_bits >>= v_table_entry_n_bits;
    v_n_bits -= v_table_entry_n_bits;
    if ((v_table_entry >> 31) != 0) {
      (wuffs_base__store_u8be__no_bounds_check(iop_a_dst,((uint8_t)(((v_table_entry >> 8) & 255)))), iop_a_dst += 1, wuffs_base__make_empty_struct());
      goto label__loop__continue;
    } else if ((v_table_entry >> 30) != 0) {
    } else if ((v_table_entry >> 29) != 0) {
      self->private_impl.f_end_of_block = true;
      goto label__loop__break;
    } else if ((v_table_entry >> 28) != 0) {
      if (v_n_bits < 15) {
        v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
        (iop_a_src += 1, wuffs_base__make_empty_struct());
        v_n_bits += 8;
        v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
        (iop_a_src += 1, wuffs_base__make_empty_struct());
        v_n_bits += 8;
      } else {
      }
      v_redir_top = ((v_table_entry >> 8) & 65535);
      v_redir_mask = ((((uint32_t)(1)) << ((v_table_entry >> 4) & 15)) - 1);
      v_table_entry = self->private_data.f_huffs[0][((v_redir_top + (v_bits & v_redir_mask)) & 1023)];
      v_table_entry_n_bits = (v_table_entry & 15);
      v_bits >>= v_table_entry_n_bits;
      v_n_bits -= v_table_entry_n_bits;
      if ((v_table_entry >> 31) != 0) {
        (wuffs_base__store_u8be__no_bounds_check(iop_a_dst,((uint8_t)(((v_table_entry >> 8) & 255)))), iop_a_dst += 1, wuffs_base__make_empty_struct());
        goto label__loop__continue;
      } else if ((v_table_entry >> 30) != 0) {
      } else if ((v_table_entry >> 29) != 0) {
        self->private_impl.f_end_of_block = true;
        goto label__loop__break;
      } else if ((v_table_entry >> 28) != 0) {
        status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
        goto exit;
      } else if ((v_table_entry >> 27) != 0) {
        status = wuffs_base__make_status(wuffs_deflate__error__bad_huffman_code);
        goto exit;
      } else {
        status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
        goto exit;
      }
    } else if ((v_table_entry >> 27) != 0) {
      status = wuffs_base__make_status(wuffs_deflate__error__bad_huffman_code);
      goto exit;
    } else {
      status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
      goto exit;
    }
    v_length = (((v_table_entry >> 8) & 255) + 3);
    v_table_entry_n_bits = ((v_table_entry >> 4) & 15);
    if (v_table_entry_n_bits > 0) {
      if (v_n_bits < 15) {
        v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
        (iop_a_src += 1, wuffs_base__make_empty_struct());
        v_n_bits += 8;
        v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
        (iop_a_src += 1, wuffs_base__make_empty_struct());
        v_n_bits += 8;
      } else {
      }
      v_length = (((v_length + 253 + ((v_bits) & WUFFS_BASE__LOW_BITS_MASK__U32(v_table_entry_n_bits))) & 255) + 3);
      v_bits >>= v_table_entry_n_bits;
      v_n_bits -= v_table_entry_n_bits;
    } else {
    }
    if (v_n_bits < 15) {
      v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
      (iop_a_src += 1, wuffs_base__make_empty_struct());
      v_n_bits += 8;
      v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
      (iop_a_src += 1, wuffs_base__make_empty_struct());
      v_n_bits += 8;
    } else {
    }
    v_table_entry = self->private_data.f_huffs[1][(v_bits & v_dmask)];
    v_table_entry_n_bits = (v_table_entry & 15);
    v_bits >>= v_table_entry_n_bits;
    v_n_bits -= v_table_entry_n_bits;
    if ((v_table_entry >> 28) == 1) {
      if (v_n_bits < 15) {
        v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
        (iop_a_src += 1, wuffs_base__make_empty_struct());
        v_n_bits += 8;
        v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
        (iop_a_src += 1, wuffs_base__make_empty_struct());
        v_n_bits += 8;
      } else {
      }
      v_redir_top = ((v_table_entry >> 8) & 65535);
      v_redir_mask = ((((uint32_t)(1)) << ((v_table_entry >> 4) & 15)) - 1);
      v_table_entry = self->private_data.f_huffs[1][((v_redir_top + (v_bits & v_redir_mask)) & 1023)];
      v_table_entry_n_bits = (v_table_entry & 15);
      v_bits >>= v_table_entry_n_bits;
      v_n_bits -= v_table_entry_n_bits;
    } else {
    }
    if ((v_table_entry >> 24) != 64) {
      if ((v_table_entry >> 24) == 8) {
        status = wuffs_base__make_status(wuffs_deflate__error__bad_huffman_code);
        goto exit;
      }
      status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
      goto exit;
    }
    v_dist_minus_1 = ((v_table_entry >> 8) & 32767);
    v_table_entry_n_bits = ((v_table_entry >> 4) & 15);
    if (v_n_bits < v_table_entry_n_bits) {
      v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
      (iop_a_src += 1, wuffs_base__make_empty_struct());
      v_n_bits += 8;
      v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
      (iop_a_src += 1, wuffs_base__make_empty_struct());
      v_n_bits += 8;
    }
    v_dist_minus_1 = ((v_dist_minus_1 + ((v_bits) & WUFFS_BASE__LOW_BITS_MASK__U32(v_table_entry_n_bits))) & 32767);
    v_bits >>= v_table_entry_n_bits;
    v_n_bits -= v_table_entry_n_bits;
    while (true) {
      if (((uint64_t)((v_dist_minus_1 + 1))) > ((uint64_t)(iop_a_dst - io0_a_dst))) {
        v_hlen = 0;
        v_hdist = ((uint32_t)((((uint64_t)((v_dist_minus_1 + 1))) - ((uint64_t)(iop_a_dst - io0_a_dst)))));
        if (v_length > v_hdist) {
          v_length -= v_hdist;
          v_hlen = v_hdist;
        } else {
          v_hlen = v_length;
          v_length = 0;
        }
        if (self->private_impl.f_history_index < v_hdist) {
          status = wuffs_base__make_status(wuffs_deflate__error__bad_distance);
          goto exit;
        }
        v_hdist = (self->private_impl.f_history_index - v_hdist);
        wuffs_base__io_writer__limited_copy_u32_from_slice(
            &iop_a_dst, io2_a_dst,v_hlen, wuffs_base__slice_u8__subslice_i(wuffs_base__make_slice_u8(self->private_data.f_history, 33025), (v_hdist & 32767)));
        if (v_length == 0) {
          goto label__loop__continue;
        }
        if (((uint64_t)((v_dist_minus_1 + 1))) > ((uint64_t)(iop_a_dst - io0_a_dst))) {
          status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_distance);
          goto exit;
        }
      }
      wuffs_base__io_writer__limited_copy_u32_from_history_fast(
          &iop_a_dst, io0_a_dst, io2_a_dst, v_length, (v_dist_minus_1 + 1));
      goto label__0__break;
    }
    label__0__break:;
  }
  label__loop__break:;
  while (v_n_bits >= 8) {
    v_n_bits -= 8;
    if (iop_a_src > io1_a_src) {
      (iop_a_src--, wuffs_base__make_empty_struct());
    } else {
      status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_i_o);
      goto exit;
    }
  }
  self->private_impl.f_bits = (v_bits & ((((uint32_t)(1)) << v_n_bits) - 1));
  self->private_impl.f_n_bits = v_n_bits;
  if ((self->private_impl.f_n_bits >= 8) || ((self->private_impl.f_bits >> self->private_impl.f_n_bits) != 0)) {
    status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_n_bits);
    goto exit;
  }
  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func deflate.decoder.decode_huffman_slow

static wuffs_base__status
wuffs_deflate__decoder__decode_huffman_slow(
    wuffs_deflate__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t v_bits = 0;
  uint32_t v_n_bits = 0;
  uint32_t v_table_entry = 0;
  uint32_t v_table_entry_n_bits = 0;
  uint32_t v_lmask = 0;
  uint32_t v_dmask = 0;
  uint32_t v_b0 = 0;
  uint32_t v_redir_top = 0;
  uint32_t v_redir_mask = 0;
  uint32_t v_b1 = 0;
  uint32_t v_length = 0;
  uint32_t v_b2 = 0;
  uint32_t v_b3 = 0;
  uint32_t v_b4 = 0;
  uint32_t v_dist_minus_1 = 0;
  uint32_t v_b5 = 0;
  uint32_t v_n_copied = 0;
  uint32_t v_hlen = 0;
  uint32_t v_hdist = 0;

  uint8_t* iop_a_dst = NULL;
  uint8_t* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }
  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_huffman_slow[0];
  if (coro_susp_point) {
    v_bits = self->private_data.s_decode_huffman_slow[0].v_bits;
    v_n_bits = self->private_data.s_decode_huffman_slow[0].v_n_bits;
    v_table_entry = self->private_data.s_decode_huffman_slow[0].v_table_entry;
    v_table_entry_n_bits = self->private_data.s_decode_huffman_slow[0].v_table_entry_n_bits;
    v_lmask = self->private_data.s_decode_huffman_slow[0].v_lmask;
    v_dmask = self->private_data.s_decode_huffman_slow[0].v_dmask;
    v_redir_top = self->private_data.s_decode_huffman_slow[0].v_redir_top;
    v_redir_mask = self->private_data.s_decode_huffman_slow[0].v_redir_mask;
    v_length = self->private_data.s_decode_huffman_slow[0].v_length;
    v_dist_minus_1 = self->private_data.s_decode_huffman_slow[0].v_dist_minus_1;
    v_hlen = self->private_data.s_decode_huffman_slow[0].v_hlen;
    v_hdist = self->private_data.s_decode_huffman_slow[0].v_hdist;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if ((self->private_impl.f_n_bits >= 8) || ((self->private_impl.f_bits >> (self->private_impl.f_n_bits & 7)) != 0)) {
      status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_n_bits);
      goto exit;
    }
    v_bits = self->private_impl.f_bits;
    v_n_bits = self->private_impl.f_n_bits;
    v_lmask = ((((uint32_t)(1)) << self->private_impl.f_n_huffs_bits[0]) - 1);
    v_dmask = ((((uint32_t)(1)) << self->private_impl.f_n_huffs_bits[1]) - 1);
    label__loop__continue:;
    while ( ! (self->private_impl.p_decode_huffman_slow[0] != 0)) {
      while (true) {
        v_table_entry = self->private_data.f_huffs[0][(v_bits & v_lmask)];
        v_table_entry_n_bits = (v_table_entry & 15);
        if (v_n_bits >= v_table_entry_n_bits) {
          v_bits >>= v_table_entry_n_bits;
          v_n_bits -= v_table_entry_n_bits;
          goto label__0__break;
        }
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint32_t t_0 = *iop_a_src++;
          v_b0 = t_0;
        }
        v_bits |= (v_b0 << v_n_bits);
        v_n_bits += 8;
      }
      label__0__break:;
      if ((v_table_entry >> 31) != 0) {
        self->private_data.s_decode_huffman_slow[0].scratch = ((uint8_t)(((v_table_entry >> 8) & 255)));
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        if (iop_a_dst == io2_a_dst) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_write);
          goto suspend;
        }
        *iop_a_dst++ = ((uint8_t)(self->private_data.s_decode_huffman_slow[0].scratch));
        goto label__loop__continue;
      } else if ((v_table_entry >> 30) != 0) {
      } else if ((v_table_entry >> 29) != 0) {
        self->private_impl.f_end_of_block = true;
        goto label__loop__break;
      } else if ((v_table_entry >> 28) != 0) {
        v_redir_top = ((v_table_entry >> 8) & 65535);
        v_redir_mask = ((((uint32_t)(1)) << ((v_table_entry >> 4) & 15)) - 1);
        while (true) {
          v_table_entry = self->private_data.f_huffs[0][((v_redir_top + (v_bits & v_redir_mask)) & 1023)];
          v_table_entry_n_bits = (v_table_entry & 15);
          if (v_n_bits >= v_table_entry_n_bits) {
            v_bits >>= v_table_entry_n_bits;
            v_n_bits -= v_table_entry_n_bits;
            goto label__1__break;
          }
          {
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
            if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              goto suspend;
            }
            uint32_t t_1 = *iop_a_src++;
            v_b1 = t_1;
          }
          v_bits |= (v_b1 << v_n_bits);
          v_n_bits += 8;
        }
        label__1__break:;
        if ((v_table_entry >> 31) != 0) {
          self->private_data.s_decode_huffman_slow[0].scratch = ((uint8_t)(((v_table_entry >> 8) & 255)));
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
          if (iop_a_dst == io2_a_dst) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_write);
            goto suspend;
          }
          *iop_a_dst++ = ((uint8_t)(self->private_data.s_decode_huffman_slow[0].scratch));
          goto label__loop__continue;
        } else if ((v_table_entry >> 30) != 0) {
        } else if ((v_table_entry >> 29) != 0) {
          self->private_impl.f_end_of_block = true;
          goto label__loop__break;
        } else if ((v_table_entry >> 28) != 0) {
          status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
          goto exit;
        } else if ((v_table_entry >> 27) != 0) {
          status = wuffs_base__make_status(wuffs_deflate__error__bad_huffman_code);
          goto exit;
        } else {
          status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
          goto exit;
        }
      } else if ((v_table_entry >> 27) != 0) {
        status = wuffs_base__make_status(wuffs_deflate__error__bad_huffman_code);
        goto exit;
      } else {
        status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
        goto exit;
      }
      v_length = (((v_table_entry >> 8) & 255) + 3);
      v_table_entry_n_bits = ((v_table_entry >> 4) & 15);
      if (v_table_entry_n_bits > 0) {
        while (v_n_bits < v_table_entry_n_bits) {
          {
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(5);
            if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              goto suspend;
            }
            uint32_t t_2 = *iop_a_src++;
            v_b2 = t_2;
          }
          v_bits |= (v_b2 << v_n_bits);
          v_n_bits += 8;
        }
        v_length = (((v_length + 253 + ((v_bits) & WUFFS_BASE__LOW_BITS_MASK__U32(v_table_entry_n_bits))) & 255) + 3);
        v_bits >>= v_table_entry_n_bits;
        v_n_bits -= v_table_entry_n_bits;
      }
      while (true) {
        v_table_entry = self->private_data.f_huffs[1][(v_bits & v_dmask)];
        v_table_entry_n_bits = (v_table_entry & 15);
        if (v_n_bits >= v_table_entry_n_bits) {
          v_bits >>= v_table_entry_n_bits;
          v_n_bits -= v_table_entry_n_bits;
          goto label__2__break;
        }
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint32_t t_3 = *iop_a_src++;
          v_b3 = t_3;
        }
        v_bits |= (v_b3 << v_n_bits);
        v_n_bits += 8;
      }
      label__2__break:;
      if ((v_table_entry >> 28) == 1) {
        v_redir_top = ((v_table_entry >> 8) & 65535);
        v_redir_mask = ((((uint32_t)(1)) << ((v_table_entry >> 4) & 15)) - 1);
        while (true) {
          v_table_entry = self->private_data.f_huffs[1][((v_redir_top + (v_bits & v_redir_mask)) & 1023)];
          v_table_entry_n_bits = (v_table_entry & 15);
          if (v_n_bits >= v_table_entry_n_bits) {
            v_bits >>= v_table_entry_n_bits;
            v_n_bits -= v_table_entry_n_bits;
            goto label__3__break;
          }
          {
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(7);
            if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              goto suspend;
            }
            uint32_t t_4 = *iop_a_src++;
            v_b4 = t_4;
          }
          v_bits |= (v_b4 << v_n_bits);
          v_n_bits += 8;
        }
        label__3__break:;
      }
      if ((v_table_entry >> 24) != 64) {
        if ((v_table_entry >> 24) == 8) {
          status = wuffs_base__make_status(wuffs_deflate__error__bad_huffman_code);
          goto exit;
        }
        status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state);
        goto exit;
      }
      v_dist_minus_1 = ((v_table_entry >> 8) & 32767);
      v_table_entry_n_bits = ((v_table_entry >> 4) & 15);
      if (v_table_entry_n_bits > 0) {
        while (v_n_bits < v_table_entry_n_bits) {
          {
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(8);
            if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              goto suspend;
            }
            uint32_t t_5 = *iop_a_src++;
            v_b5 = t_5;
          }
          v_bits |= (v_b5 << v_n_bits);
          v_n_bits += 8;
        }
        v_dist_minus_1 = ((v_dist_minus_1 + ((v_bits) & WUFFS_BASE__LOW_BITS_MASK__U32(v_table_entry_n_bits))) & 32767);
        v_bits >>= v_table_entry_n_bits;
        v_n_bits -= v_table_entry_n_bits;
      }
      while (true) {
        if (((uint64_t)((v_dist_minus_1 + 1))) > ((uint64_t)(iop_a_dst - io0_a_dst))) {
          v_hdist = ((uint32_t)((((uint64_t)((v_dist_minus_1 + 1))) - ((uint64_t)(iop_a_dst - io0_a_dst)))));
          if (v_length > v_hdist) {
            v_length -= v_hdist;
            v_hlen = v_hdist;
          } else {
            v_hlen = v_length;
            v_length = 0;
          }
          if (self->private_impl.f_history_index < v_hdist) {
            status = wuffs_base__make_status(wuffs_deflate__error__bad_distance);
            goto exit;
          }
          v_hdist = (self->private_impl.f_history_index - v_hdist);
          while (true) {
            v_n_copied = wuffs_base__io_writer__limited_copy_u32_from_slice(
                &iop_a_dst, io2_a_dst,v_hlen, wuffs_base__slice_u8__subslice_ij(wuffs_base__make_slice_u8(self->private_data.f_history, 33025), (v_hdist & 32767), 32768));
            if (v_hlen <= v_n_copied) {
              v_hlen = 0;
              goto label__4__break;
            }
            if (v_n_copied > 0) {
              v_hlen -= v_n_copied;
              v_hdist = ((v_hdist + v_n_copied) & 32767);
              if (v_hdist == 0) {
                goto label__4__break;
              }
            }
            status = wuffs_base__make_status(wuffs_base__suspension__short_write);
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(9);
          }
          label__4__break:;
          if (v_hlen > 0) {
            while (true) {
              v_n_copied = wuffs_base__io_writer__limited_copy_u32_from_slice(
                  &iop_a_dst, io2_a_dst,v_hlen, wuffs_base__slice_u8__subslice_ij(wuffs_base__make_slice_u8(self->private_data.f_history, 33025), (v_hdist & 32767), 32768));
              if (v_hlen <= v_n_copied) {
                v_hlen = 0;
                goto label__5__break;
              }
              v_hlen -= v_n_copied;
              v_hdist += v_n_copied;
              status = wuffs_base__make_status(wuffs_base__suspension__short_write);
              WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(10);
            }
            label__5__break:;
          }
          if (v_length == 0) {
            goto label__loop__continue;
          }
        }
        v_n_copied = wuffs_base__io_writer__limited_copy_u32_from_history(
            &iop_a_dst, io0_a_dst, io2_a_dst, v_length, (v_dist_minus_1 + 1));
        if (v_length <= v_n_copied) {
          v_length = 0;
          goto label__6__break;
        }
        v_length -= v_n_copied;
        status = wuffs_base__make_status(wuffs_base__suspension__short_write);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(11);
      }
      label__6__break:;
    }
    label__loop__break:;
    self->private_impl.f_bits = v_bits;
    self->private_impl.f_n_bits = v_n_bits;
    if ((self->private_impl.f_n_bits >= 8) || ((self->private_impl.f_bits >> (self->private_impl.f_n_bits & 7)) != 0)) {
      status = wuffs_base__make_status(wuffs_deflate__error__internal_error_inconsistent_n_bits);
      goto exit;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_huffman_slow[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_huffman_slow[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_data.s_decode_huffman_slow[0].v_bits = v_bits;
  self->private_data.s_decode_huffman_slow[0].v_n_bits = v_n_bits;
  self->private_data.s_decode_huffman_slow[0].v_table_entry = v_table_entry;
  self->private_data.s_decode_huffman_slow[0].v_table_entry_n_bits = v_table_entry_n_bits;
  self->private_data.s_decode_huffman_slow[0].v_lmask = v_lmask;
  self->private_data.s_decode_huffman_slow[0].v_dmask = v_dmask;
  self->private_data.s_decode_huffman_slow[0].v_redir_top = v_redir_top;
  self->private_data.s_decode_huffman_slow[0].v_redir_mask = v_redir_mask;
  self->private_data.s_decode_huffman_slow[0].v_length = v_length;
  self->private_data.s_decode_huffman_slow[0].v_dist_minus_1 = v_dist_minus_1;
  self->private_data.s_decode_huffman_slow[0].v_hlen = v_hlen;
  self->private_data.s_decode_huffman_slow[0].v_hdist = v_hdist;

  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

#endif  // !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__DEFLATE)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__LZW)

// ---------------- Status Codes Implementations

const char* wuffs_lzw__error__bad_code = "#lzw: bad code";
const char* wuffs_lzw__error__internal_error_inconsistent_i_o = "#lzw: internal error: inconsistent I/O";

// ---------------- Private Consts

// ---------------- Private Initializer Prototypes

// ---------------- Private Function Prototypes

static wuffs_base__empty_struct
wuffs_lzw__decoder__read_from(
    wuffs_lzw__decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_lzw__decoder__write_to(
    wuffs_lzw__decoder* self,
    wuffs_base__io_buffer* a_dst);

// ---------------- VTables

const wuffs_base__io_transformer__func_ptrs
wuffs_lzw__decoder__func_ptrs_for__wuffs_base__io_transformer = {
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_lzw__decoder__set_quirk_enabled),
  (wuffs_base__status(*)(void*,
      wuffs_base__io_buffer*,
      wuffs_base__io_buffer*,
      wuffs_base__slice_u8))(&wuffs_lzw__decoder__transform_io),
  (wuffs_base__range_ii_u64(*)(const void*))(&wuffs_lzw__decoder__workbuf_len),
};

// ---------------- Initializer Implementations

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_lzw__decoder__initialize(
    wuffs_lzw__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags){
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (sizeof(*self) != sizeof_star_self) {
    return wuffs_base__make_status(wuffs_base__error__bad_sizeof_receiver);
  }
  if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) ||
      (((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {
    return wuffs_base__make_status(wuffs_base__error__bad_wuffs_version);
  }

  if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {
    // The whole point of this if-check is to detect an uninitialized *self.
    // We disable the warning on GCC. Clang-5.0 does not have this warning.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if (self->private_impl.magic != 0) {
      return wuffs_base__make_status(wuffs_base__error__initialize_falsely_claimed_already_zeroed);
    }
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  } else {
    if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {
      memset(self, 0, sizeof(*self));
      initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;
    } else {
      memset(&(self->private_impl), 0, sizeof(self->private_impl));
    }
  }

  self->private_impl.magic = WUFFS_BASE__MAGIC;
  self->private_impl.vtable_for__wuffs_base__io_transformer.vtable_name =
      wuffs_base__io_transformer__vtable_name;
  self->private_impl.vtable_for__wuffs_base__io_transformer.function_pointers =
      (const void*)(&wuffs_lzw__decoder__func_ptrs_for__wuffs_base__io_transformer);
  return wuffs_base__make_status(NULL);
}

wuffs_lzw__decoder*
wuffs_lzw__decoder__alloc() {
  wuffs_lzw__decoder* x =
      (wuffs_lzw__decoder*)(calloc(sizeof(wuffs_lzw__decoder), 1));
  if (!x) {
    return NULL;
  }
  if (wuffs_lzw__decoder__initialize(
      x, sizeof(wuffs_lzw__decoder), WUFFS_VERSION, WUFFS_INITIALIZE__ALREADY_ZEROED).repr) {
    free(x);
    return NULL;
  }
  return x;
}

size_t
sizeof__wuffs_lzw__decoder() {
  return sizeof(wuffs_lzw__decoder);
}

// ---------------- Function Implementations

// -------- func lzw.decoder.set_quirk_enabled

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_lzw__decoder__set_quirk_enabled(
    wuffs_lzw__decoder* self,
    uint32_t a_quirk,
    bool a_enabled) {
  return wuffs_base__make_empty_struct();
}

// -------- func lzw.decoder.set_literal_width

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_lzw__decoder__set_literal_width(
    wuffs_lzw__decoder* self,
    uint32_t a_lw) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }
  if (a_lw > 8) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_empty_struct();
  }

  self->private_impl.f_set_literal_width_arg = (a_lw + 1);
  return wuffs_base__make_empty_struct();
}

// -------- func lzw.decoder.workbuf_len

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_lzw__decoder__workbuf_len(
    const wuffs_lzw__decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  return wuffs_base__utility__make_range_ii_u64(0, 0);
}

// -------- func lzw.decoder.transform_io

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_lzw__decoder__transform_io(
    wuffs_lzw__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 1)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t v_i = 0;

  uint32_t coro_susp_point = self->private_impl.p_transform_io[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    self->private_impl.f_literal_width = 8;
    if (self->private_impl.f_set_literal_width_arg > 0) {
      self->private_impl.f_literal_width = (self->private_impl.f_set_literal_width_arg - 1);
    }
    self->private_impl.f_clear_code = (((uint32_t)(1)) << self->private_impl.f_literal_width);
    self->private_impl.f_end_code = (self->private_impl.f_clear_code + 1);
    self->private_impl.f_save_code = self->private_impl.f_end_code;
    self->private_impl.f_prev_code = self->private_impl.f_end_code;
    self->private_impl.f_width = (self->private_impl.f_literal_width + 1);
    self->private_impl.f_bits = 0;
    self->private_impl.f_n_bits = 0;
    self->private_impl.f_output_ri = 0;
    self->private_impl.f_output_wi = 0;
    v_i = 0;
    while (v_i < self->private_impl.f_clear_code) {
      self->private_data.f_lm1s[v_i] = 0;
      self->private_data.f_suffixes[v_i][0] = ((uint8_t)(v_i));
      v_i += 1;
    }
    label__0__continue:;
    while (true) {
      wuffs_lzw__decoder__read_from(self, a_src);
      if (self->private_impl.f_output_wi > 0) {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        status = wuffs_lzw__decoder__write_to(self, a_dst);
        if (status.repr) {
          goto suspend;
        }
      }
      if (self->private_impl.f_read_from_return_value == 0) {
        goto label__0__break;
      } else if (self->private_impl.f_read_from_return_value == 1) {
        goto label__0__continue;
      } else if (self->private_impl.f_read_from_return_value == 2) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(2);
      } else if (self->private_impl.f_read_from_return_value == 3) {
        status = wuffs_base__make_status(wuffs_lzw__error__bad_code);
        goto exit;
      } else {
        status = wuffs_base__make_status(wuffs_lzw__error__internal_error_inconsistent_i_o);
        goto exit;
      }
    }
    label__0__break:;

    goto ok;
    ok:
    self->private_impl.p_transform_io[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_transform_io[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 1 : 0;

  goto exit;
  exit:
  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func lzw.decoder.read_from

static wuffs_base__empty_struct
wuffs_lzw__decoder__read_from(
    wuffs_lzw__decoder* self,
    wuffs_base__io_buffer* a_src) {
  uint32_t v_clear_code = 0;
  uint32_t v_end_code = 0;
  uint32_t v_save_code = 0;
  uint32_t v_prev_code = 0;
  uint32_t v_width = 0;
  uint32_t v_bits = 0;
  uint32_t v_n_bits = 0;
  uint32_t v_output_wi = 0;
  uint32_t v_code = 0;
  uint32_t v_c = 0;
  uint32_t v_o = 0;
  uint32_t v_steps = 0;
  uint8_t v_first_byte = 0;
  uint16_t v_lm1_b = 0;
  uint16_t v_lm1_a = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  v_clear_code = self->private_impl.f_clear_code;
  v_end_code = self->private_impl.f_end_code;
  v_save_code = self->private_impl.f_save_code;
  v_prev_code = self->private_impl.f_prev_code;
  v_width = self->private_impl.f_width;
  v_bits = self->private_impl.f_bits;
  v_n_bits = self->private_impl.f_n_bits;
  v_output_wi = self->private_impl.f_output_wi;
  while (true) {
    if (v_n_bits < v_width) {
      if (((uint64_t)(io2_a_src - iop_a_src)) >= 4) {
        v_bits |= (wuffs_base__load_u32le__no_bounds_check(iop_a_src) << v_n_bits);
        (iop_a_src += ((31 - v_n_bits) >> 3), wuffs_base__make_empty_struct());
        v_n_bits |= 24;
      } else if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
        self->private_impl.f_read_from_return_value = 2;
        goto label__0__break;
      } else {
        v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
        (iop_a_src += 1, wuffs_base__make_empty_struct());
        v_n_bits += 8;
        if (v_n_bits >= v_width) {
        } else if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
          self->private_impl.f_read_from_return_value = 2;
          goto label__0__break;
        } else {
          v_bits |= (((uint32_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src))) << v_n_bits);
          (iop_a_src += 1, wuffs_base__make_empty_struct());
          v_n_bits += 8;
          if (v_n_bits < v_width) {
            self->private_impl.f_read_from_return_value = 4;
            goto label__0__break;
          }
        }
      }
    }
    v_code = ((v_bits) & WUFFS_BASE__LOW_BITS_MASK__U32(v_width));
    v_bits >>= v_width;
    v_n_bits -= v_width;
    if (v_code < v_clear_code) {
      self->private_data.f_output[v_output_wi] = ((uint8_t)(v_code));
      v_output_wi = ((v_output_wi + 1) & 8191);
      if (v_save_code <= 4095) {
        v_lm1_a = ((self->private_data.f_lm1s[v_prev_code] + 1) & 4095);
        self->private_data.f_lm1s[v_save_code] = v_lm1_a;
        if ((v_lm1_a % 8) != 0) {
          self->private_impl.f_prefixes[v_save_code] = self->private_impl.f_prefixes[v_prev_code];
          memcpy(self->private_data.f_suffixes[v_save_code],self->private_data.f_suffixes[v_prev_code], sizeof(self->private_data.f_suffixes[v_save_code]));
          self->private_data.f_suffixes[v_save_code][(v_lm1_a % 8)] = ((uint8_t)(v_code));
        } else {
          self->private_impl.f_prefixes[v_save_code] = ((uint16_t)(v_prev_code));
          self->private_data.f_suffixes[v_save_code][0] = ((uint8_t)(v_code));
        }
        v_save_code += 1;
        if (v_width < 12) {
          v_width += (1 & (v_save_code >> v_width));
        }
        v_prev_code = v_code;
      }
    } else if (v_code <= v_end_code) {
      if (v_code == v_end_code) {
        self->private_impl.f_read_from_return_value = 0;
        goto label__0__break;
      }
      v_save_code = v_end_code;
      v_prev_code = v_end_code;
      v_width = (self->private_impl.f_literal_width + 1);
    } else if (v_code <= v_save_code) {
      v_c = v_code;
      if (v_code == v_save_code) {
        v_c = v_prev_code;
      }
      v_o = ((v_output_wi + (((uint32_t)(self->private_data.f_lm1s[v_c])) & 4294967288)) & 8191);
      v_output_wi = ((v_output_wi + 1 + ((uint32_t)(self->private_data.f_lm1s[v_c]))) & 8191);
      v_steps = (((uint32_t)(self->private_data.f_lm1s[v_c])) >> 3);
      while (true) {
        memcpy((self->private_data.f_output)+(v_o), (self->private_data.f_suffixes[v_c]), 8);
        if (v_steps <= 0) {
          goto label__1__break;
        }
        v_steps -= 1;
        v_o = ((v_o - 8) & 8191);
        v_c = ((uint32_t)(self->private_impl.f_prefixes[v_c]));
      }
      label__1__break:;
      v_first_byte = self->private_data.f_suffixes[v_c][0];
      if (v_code == v_save_code) {
        self->private_data.f_output[v_output_wi] = v_first_byte;
        v_output_wi = ((v_output_wi + 1) & 8191);
      }
      if (v_save_code <= 4095) {
        v_lm1_b = ((self->private_data.f_lm1s[v_prev_code] + 1) & 4095);
        self->private_data.f_lm1s[v_save_code] = v_lm1_b;
        if ((v_lm1_b % 8) != 0) {
          self->private_impl.f_prefixes[v_save_code] = self->private_impl.f_prefixes[v_prev_code];
          memcpy(self->private_data.f_suffixes[v_save_code],self->private_data.f_suffixes[v_prev_code], sizeof(self->private_data.f_suffixes[v_save_code]));
          self->private_data.f_suffixes[v_save_code][(v_lm1_b % 8)] = v_first_byte;
        } else {
          self->private_impl.f_prefixes[v_save_code] = ((uint16_t)(v_prev_code));
          self->private_data.f_suffixes[v_save_code][0] = ((uint8_t)(v_first_byte));
        }
        v_save_code += 1;
        if (v_width < 12) {
          v_width += (1 & (v_save_code >> v_width));
        }
        v_prev_code = v_code;
      }
    } else {
      self->private_impl.f_read_from_return_value = 3;
      goto label__0__break;
    }
    if (v_output_wi > 4095) {
      self->private_impl.f_read_from_return_value = 1;
      goto label__0__break;
    }
  }
  label__0__break:;
  if (self->private_impl.f_read_from_return_value != 2) {
    while (v_n_bits >= 8) {
      v_n_bits -= 8;
      if (iop_a_src > io1_a_src) {
        (iop_a_src--, wuffs_base__make_empty_struct());
      } else {
        self->private_impl.f_read_from_return_value = 4;
        goto label__2__break;
      }
    }
    label__2__break:;
  }
  self->private_impl.f_save_code = v_save_code;
  self->private_impl.f_prev_code = v_prev_code;
  self->private_impl.f_width = v_width;
  self->private_impl.f_bits = v_bits;
  self->private_impl.f_n_bits = v_n_bits;
  self->private_impl.f_output_wi = v_output_wi;
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return wuffs_base__make_empty_struct();
}

// -------- func lzw.decoder.write_to

static wuffs_base__status
wuffs_lzw__decoder__write_to(
    wuffs_lzw__decoder* self,
    wuffs_base__io_buffer* a_dst) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  wuffs_base__slice_u8 v_s = {0};
  uint64_t v_n = 0;

  uint8_t* iop_a_dst = NULL;
  uint8_t* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }

  uint32_t coro_susp_point = self->private_impl.p_write_to[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    while (self->private_impl.f_output_wi > 0) {
      if (self->private_impl.f_output_ri > self->private_impl.f_output_wi) {
        status = wuffs_base__make_status(wuffs_lzw__error__internal_error_inconsistent_i_o);
        goto exit;
      }
      v_s = wuffs_base__slice_u8__subslice_ij(wuffs_base__make_slice_u8(self->private_data.f_output,
          8199),
          self->private_impl.f_output_ri,
          self->private_impl.f_output_wi);
      v_n = wuffs_base__io_writer__copy_from_slice(&iop_a_dst, io2_a_dst,v_s);
      if (v_n == ((uint64_t)(v_s.len))) {
        self->private_impl.f_output_ri = 0;
        self->private_impl.f_output_wi = 0;
        status = wuffs_base__make_status(NULL);
        goto ok;
      }
      self->private_impl.f_output_ri = ((self->private_impl.f_output_ri + ((uint32_t)((v_n & 4294967295)))) & 8191);
      status = wuffs_base__make_status(wuffs_base__suspension__short_write);
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(1);
    }

    goto ok;
    ok:
    self->private_impl.p_write_to[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_write_to[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }

  return status;
}

// -------- func lzw.decoder.flush

WUFFS_BASE__MAYBE_STATIC wuffs_base__slice_u8
wuffs_lzw__decoder__flush(
    wuffs_lzw__decoder* self) {
  if (!self) {
    return wuffs_base__make_slice_u8(NULL, 0);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_slice_u8(NULL, 0);
  }

  wuffs_base__slice_u8 v_s = {0};

  if (self->private_impl.f_output_ri <= self->private_impl.f_output_wi) {
    v_s = wuffs_base__slice_u8__subslice_ij(wuffs_base__make_slice_u8(self->private_data.f_output,
        8199),
        self->private_impl.f_output_ri,
        self->private_impl.f_output_wi);
  }
  self->private_impl.f_output_ri = 0;
  self->private_impl.f_output_wi = 0;
  return v_s;
}

#endif  // !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__LZW)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__GIF)

// ---------------- Status Codes Implementations

const char* wuffs_gif__error__bad_extension_label = "#gif: bad extension label";
const char* wuffs_gif__error__bad_frame_size = "#gif: bad frame size";
const char* wuffs_gif__error__bad_graphic_control = "#gif: bad graphic control";
const char* wuffs_gif__error__bad_header = "#gif: bad header";
const char* wuffs_gif__error__bad_literal_width = "#gif: bad literal width";
const char* wuffs_gif__error__bad_palette = "#gif: bad palette";
const char* wuffs_gif__error__internal_error_inconsistent_ri_wi = "#gif: internal error: inconsistent ri/wi";

// ---------------- Private Consts

static const uint32_t
WUFFS_GIF__INTERLACE_START[5]WUFFS_BASE__POTENTIALLY_UNUSED = {
  4294967295, 1, 2, 4, 0,
};

static const uint8_t
WUFFS_GIF__INTERLACE_DELTA[5]WUFFS_BASE__POTENTIALLY_UNUSED = {
  1, 2, 4, 8, 8,
};

static const uint8_t
WUFFS_GIF__INTERLACE_COUNT[5]WUFFS_BASE__POTENTIALLY_UNUSED = {
  0, 1, 2, 4, 8,
};

static const uint8_t
WUFFS_GIF__ANIMEXTS1DOT0[11]WUFFS_BASE__POTENTIALLY_UNUSED = {
  65, 78, 73, 77, 69, 88, 84, 83,
  49, 46, 48,
};

static const uint8_t
WUFFS_GIF__NETSCAPE2DOT0[11]WUFFS_BASE__POTENTIALLY_UNUSED = {
  78, 69, 84, 83, 67, 65, 80, 69,
  50, 46, 48,
};

static const uint8_t
WUFFS_GIF__ICCRGBG1012[11]WUFFS_BASE__POTENTIALLY_UNUSED = {
  73, 67, 67, 82, 71, 66, 71, 49,
  48, 49, 50,
};

static const uint8_t
WUFFS_GIF__XMPDATAXMP[11]WUFFS_BASE__POTENTIALLY_UNUSED = {
  88, 77, 80, 32, 68, 97, 116, 97,
  88, 77, 80,
};

#define WUFFS_GIF__QUIRKS_BASE 1041635328

#define WUFFS_GIF__QUIRKS_COUNT 7

// ---------------- Private Initializer Prototypes

// ---------------- Private Function Prototypes

static wuffs_base__status
wuffs_gif__config_decoder__skip_frame(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__empty_struct
wuffs_gif__config_decoder__reset_gc(
    wuffs_gif__config_decoder* self);

static wuffs_base__status
wuffs_gif__config_decoder__decode_up_to_id_part1(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__config_decoder__decode_header(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__config_decoder__decode_lsd(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__config_decoder__decode_extension(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__config_decoder__skip_blocks(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__config_decoder__decode_ae(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__config_decoder__decode_gc(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__config_decoder__decode_id_part0(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__decoder__skip_frame(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__empty_struct
wuffs_gif__decoder__reset_gc(
    wuffs_gif__decoder* self);

static wuffs_base__status
wuffs_gif__decoder__decode_up_to_id_part1(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__decoder__decode_header(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__decoder__decode_lsd(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__decoder__decode_extension(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__decoder__skip_blocks(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__decoder__decode_ae(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__decoder__decode_gc(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__decoder__decode_id_part0(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_gif__decoder__decode_id_part1(
    wuffs_gif__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend);

static wuffs_base__status
wuffs_gif__decoder__decode_id_part2(
    wuffs_gif__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf);

static wuffs_base__status
wuffs_gif__decoder__copy_to_image_buffer(
    wuffs_gif__decoder* self,
    wuffs_base__pixel_buffer* a_pb,
    wuffs_base__slice_u8 a_src);

// ---------------- VTables

const wuffs_base__image_decoder__func_ptrs
wuffs_gif__config_decoder__func_ptrs_for__wuffs_base__image_decoder = {
  (wuffs_base__status(*)(void*,
      wuffs_base__pixel_buffer*,
      wuffs_base__io_buffer*,
      wuffs_base__pixel_blend,
      wuffs_base__slice_u8,
      wuffs_base__decode_frame_options*))(&wuffs_gif__config_decoder__decode_frame),
  (wuffs_base__status(*)(void*,
      wuffs_base__frame_config*,
      wuffs_base__io_buffer*))(&wuffs_gif__config_decoder__decode_frame_config),
  (wuffs_base__status(*)(void*,
      wuffs_base__image_config*,
      wuffs_base__io_buffer*))(&wuffs_gif__config_decoder__decode_image_config),
  (wuffs_base__rect_ie_u32(*)(const void*))(&wuffs_gif__config_decoder__frame_dirty_rect),
  (uint32_t(*)(const void*))(&wuffs_gif__config_decoder__num_animation_loops),
  (uint64_t(*)(const void*))(&wuffs_gif__config_decoder__num_decoded_frame_configs),
  (uint64_t(*)(const void*))(&wuffs_gif__config_decoder__num_decoded_frames),
  (wuffs_base__status(*)(void*,
      uint64_t,
      uint64_t))(&wuffs_gif__config_decoder__restart_frame),
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_gif__config_decoder__set_quirk_enabled),
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_gif__config_decoder__set_report_metadata),
  (wuffs_base__status(*)(void*,
      wuffs_base__io_buffer*,
      wuffs_base__more_information*,
      wuffs_base__io_buffer*))(&wuffs_gif__config_decoder__tell_me_more),
  (wuffs_base__range_ii_u64(*)(const void*))(&wuffs_gif__config_decoder__workbuf_len),
};

const wuffs_base__image_decoder__func_ptrs
wuffs_gif__decoder__func_ptrs_for__wuffs_base__image_decoder = {
  (wuffs_base__status(*)(void*,
      wuffs_base__pixel_buffer*,
      wuffs_base__io_buffer*,
      wuffs_base__pixel_blend,
      wuffs_base__slice_u8,
      wuffs_base__decode_frame_options*))(&wuffs_gif__decoder__decode_frame),
  (wuffs_base__status(*)(void*,
      wuffs_base__frame_config*,
      wuffs_base__io_buffer*))(&wuffs_gif__decoder__decode_frame_config),
  (wuffs_base__status(*)(void*,
      wuffs_base__image_config*,
      wuffs_base__io_buffer*))(&wuffs_gif__decoder__decode_image_config),
  (wuffs_base__rect_ie_u32(*)(const void*))(&wuffs_gif__decoder__frame_dirty_rect),
  (uint32_t(*)(const void*))(&wuffs_gif__decoder__num_animation_loops),
  (uint64_t(*)(const void*))(&wuffs_gif__decoder__num_decoded_frame_configs),
  (uint64_t(*)(const void*))(&wuffs_gif__decoder__num_decoded_frames),
  (wuffs_base__status(*)(void*,
      uint64_t,
      uint64_t))(&wuffs_gif__decoder__restart_frame),
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_gif__decoder__set_quirk_enabled),
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_gif__decoder__set_report_metadata),
  (wuffs_base__status(*)(void*,
      wuffs_base__io_buffer*,
      wuffs_base__more_information*,
      wuffs_base__io_buffer*))(&wuffs_gif__decoder__tell_me_more),
  (wuffs_base__range_ii_u64(*)(const void*))(&wuffs_gif__decoder__workbuf_len),
};

// ---------------- Initializer Implementations

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_gif__config_decoder__initialize(
    wuffs_gif__config_decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags){
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (sizeof(*self) != sizeof_star_self) {
    return wuffs_base__make_status(wuffs_base__error__bad_sizeof_receiver);
  }
  if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) ||
      (((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {
    return wuffs_base__make_status(wuffs_base__error__bad_wuffs_version);
  }

  if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {
    // The whole point of this if-check is to detect an uninitialized *self.
    // We disable the warning on GCC. Clang-5.0 does not have this warning.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if (self->private_impl.magic != 0) {
      return wuffs_base__make_status(wuffs_base__error__initialize_falsely_claimed_already_zeroed);
    }
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  } else {
    if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {
      memset(self, 0, sizeof(*self));
      initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;
    } else {
      memset(&(self->private_impl), 0, sizeof(self->private_impl));
    }
  }

  self->private_impl.magic = WUFFS_BASE__MAGIC;
  self->private_impl.vtable_for__wuffs_base__image_decoder.vtable_name =
      wuffs_base__image_decoder__vtable_name;
  self->private_impl.vtable_for__wuffs_base__image_decoder.function_pointers =
      (const void*)(&wuffs_gif__config_decoder__func_ptrs_for__wuffs_base__image_decoder);
  return wuffs_base__make_status(NULL);
}

wuffs_gif__config_decoder*
wuffs_gif__config_decoder__alloc() {
  wuffs_gif__config_decoder* x =
      (wuffs_gif__config_decoder*)(calloc(sizeof(wuffs_gif__config_decoder), 1));
  if (!x) {
    return NULL;
  }
  if (wuffs_gif__config_decoder__initialize(
      x, sizeof(wuffs_gif__config_decoder), WUFFS_VERSION, WUFFS_INITIALIZE__ALREADY_ZEROED).repr) {
    free(x);
    return NULL;
  }
  return x;
}

size_t
sizeof__wuffs_gif__config_decoder() {
  return sizeof(wuffs_gif__config_decoder);
}

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_gif__decoder__initialize(
    wuffs_gif__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags){
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (sizeof(*self) != sizeof_star_self) {
    return wuffs_base__make_status(wuffs_base__error__bad_sizeof_receiver);
  }
  if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) ||
      (((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {
    return wuffs_base__make_status(wuffs_base__error__bad_wuffs_version);
  }

  if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {
    // The whole point of this if-check is to detect an uninitialized *self.
    // We disable the warning on GCC. Clang-5.0 does not have this warning.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if (self->private_impl.magic != 0) {
      return wuffs_base__make_status(wuffs_base__error__initialize_falsely_claimed_already_zeroed);
    }
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  } else {
    if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {
      memset(self, 0, sizeof(*self));
      initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;
    } else {
      memset(&(self->private_impl), 0, sizeof(self->private_impl));
    }
  }

  {
    wuffs_base__status z = wuffs_lzw__decoder__initialize(
        &self->private_data.f_lzw, sizeof(self->private_data.f_lzw), WUFFS_VERSION, initialize_flags);
    if (z.repr) {
      return z;
    }
  }
  self->private_impl.magic = WUFFS_BASE__MAGIC;
  self->private_impl.vtable_for__wuffs_base__image_decoder.vtable_name =
      wuffs_base__image_decoder__vtable_name;
  self->private_impl.vtable_for__wuffs_base__image_decoder.function_pointers =
      (const void*)(&wuffs_gif__decoder__func_ptrs_for__wuffs_base__image_decoder);
  return wuffs_base__make_status(NULL);
}

wuffs_gif__decoder*
wuffs_gif__decoder__alloc() {
  wuffs_gif__decoder* x =
      (wuffs_gif__decoder*)(calloc(sizeof(wuffs_gif__decoder), 1));
  if (!x) {
    return NULL;
  }
  if (wuffs_gif__decoder__initialize(
      x, sizeof(wuffs_gif__decoder), WUFFS_VERSION, WUFFS_INITIALIZE__ALREADY_ZEROED).repr) {
    free(x);
    return NULL;
  }
  return x;
}

size_t
sizeof__wuffs_gif__decoder() {
  return sizeof(wuffs_gif__decoder);
}

// ---------------- Function Implementations

// -------- func gif.config_decoder.set_quirk_enabled

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gif__config_decoder__set_quirk_enabled(
    wuffs_gif__config_decoder* self,
    uint32_t a_quirk,
    bool a_enabled) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  if ((self->private_impl.f_call_sequence == 0) && (a_quirk >= 1041635328)) {
    a_quirk -= 1041635328;
    if (a_quirk < 7) {
      self->private_impl.f_quirks[a_quirk] = a_enabled;
    }
  }
  return wuffs_base__make_empty_struct();
}

// -------- func gif.config_decoder.decode_image_config

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__config_decoder__decode_image_config(
    wuffs_gif__config_decoder* self,
    wuffs_base__image_config* a_dst,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 1)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  bool v_ffio = false;

  uint32_t coro_susp_point = self->private_impl.p_decode_image_config[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if (self->private_impl.f_call_sequence == 0) {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      status = wuffs_gif__config_decoder__decode_header(self, a_src);
      if (status.repr) {
        goto suspend;
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      status = wuffs_gif__config_decoder__decode_lsd(self, a_src);
      if (status.repr) {
        goto suspend;
      }
    } else if (self->private_impl.f_call_sequence != 2) {
      status = wuffs_base__make_status(wuffs_base__error__bad_call_sequence);
      goto exit;
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
    status = wuffs_gif__config_decoder__decode_up_to_id_part1(self, a_src);
    if (status.repr) {
      goto suspend;
    }
    v_ffio =  ! self->private_impl.f_gc_has_transparent_index;
    if ( ! self->private_impl.f_quirks[2]) {
      v_ffio = (v_ffio &&
          (self->private_impl.f_frame_rect_x0 == 0) &&
          (self->private_impl.f_frame_rect_y0 == 0) &&
          (self->private_impl.f_frame_rect_x1 == self->private_impl.f_width) &&
          (self->private_impl.f_frame_rect_y1 == self->private_impl.f_height));
    } else if (v_ffio) {
      self->private_impl.f_black_color_u32_argb_premul = 4278190080;
    }
    if (self->private_impl.f_background_color_u32_argb_premul == 77) {
      self->private_impl.f_background_color_u32_argb_premul = self->private_impl.f_black_color_u32_argb_premul;
    }
    if (a_dst != NULL) {
      wuffs_base__image_config__set(
          a_dst,
          2198077448,
          0,
          self->private_impl.f_width,
          self->private_impl.f_height,
          self->private_impl.f_frame_config_io_position,
          v_ffio);
    }
    self->private_impl.f_call_sequence = 3;

    goto ok;
    ok:
    self->private_impl.p_decode_image_config[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_image_config[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 1 : 0;

  goto exit;
  exit:
  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func gif.config_decoder.set_report_metadata

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gif__config_decoder__set_report_metadata(
    wuffs_gif__config_decoder* self,
    uint32_t a_fourcc,
    bool a_report) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  if (a_fourcc == 1229144912) {
    self->private_impl.f_report_metadata_iccp = a_report;
  } else if (a_fourcc == 1481461792) {
    self->private_impl.f_report_metadata_xmp = a_report;
  }
  return wuffs_base__make_empty_struct();
}

// -------- func gif.config_decoder.tell_me_more

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__config_decoder__tell_me_more(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__more_information* a_minfo,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 2)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint64_t v_chunk_length = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_tell_me_more[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if (self->private_impl.f_call_sequence != 1) {
      status = wuffs_base__make_status(wuffs_base__error__bad_call_sequence);
      goto exit;
    }
    if (self->private_impl.f_metadata_fourcc == 0) {
      status = wuffs_base__make_status(wuffs_base__error__no_more_information);
      goto exit;
    }
    while (true) {
      label__0__continue:;
      while (true) {
        if (wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src))) != self->private_impl.f_metadata_io_position) {
          if (a_minfo != NULL) {
            wuffs_base__more_information__set(a_minfo,
                2,
                0,
                self->private_impl.f_metadata_io_position,
                0,
                0);
          }
          status = wuffs_base__make_status(wuffs_base__suspension__mispositioned_read);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(1);
          goto label__0__continue;
        }
        if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
          if (a_minfo != NULL) {
            wuffs_base__more_information__set(a_minfo,
                0,
                0,
                0,
                0,
                0);
          }
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(2);
          goto label__0__continue;
        }
        goto label__0__break;
      }
      label__0__break:;
      v_chunk_length = ((uint64_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src)));
      if (v_chunk_length <= 0) {
        (iop_a_src += 1, wuffs_base__make_empty_struct());
        goto label__1__break;
      }
      if (self->private_impl.f_metadata_fourcc == 1481461792) {
        v_chunk_length += 1;
      } else {
        (iop_a_src += 1, wuffs_base__make_empty_struct());
      }
      self->private_impl.f_metadata_io_position = wuffs_base__u64__sat_add(wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src))), v_chunk_length);
      if (a_minfo != NULL) {
        wuffs_base__more_information__set(a_minfo,
            3,
            self->private_impl.f_metadata_fourcc,
            0,
            wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src))),
            self->private_impl.f_metadata_io_position);
      }
      status = wuffs_base__make_status(wuffs_base__suspension__even_more_information);
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(3);
    }
    label__1__break:;
    if (a_minfo != NULL) {
      wuffs_base__more_information__set(a_minfo,
          3,
          self->private_impl.f_metadata_fourcc,
          0,
          self->private_impl.f_metadata_io_position,
          self->private_impl.f_metadata_io_position);
    }
    self->private_impl.f_call_sequence = 2;
    self->private_impl.f_metadata_fourcc = 0;
    self->private_impl.f_metadata_io_position = 0;
    status = wuffs_base__make_status(NULL);
    goto ok;

    goto ok;
    ok:
    self->private_impl.p_tell_me_more[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_tell_me_more[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 2 : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func gif.config_decoder.num_animation_loops

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_gif__config_decoder__num_animation_loops(
    const wuffs_gif__config_decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  if (self->private_impl.f_seen_num_loops) {
    return self->private_impl.f_num_loops;
  }
  return 1;
}

// -------- func gif.config_decoder.num_decoded_frame_configs

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_gif__config_decoder__num_decoded_frame_configs(
    const wuffs_gif__config_decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  return self->private_impl.f_num_decoded_frame_configs_value;
}

// -------- func gif.config_decoder.num_decoded_frames

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_gif__config_decoder__num_decoded_frames(
    const wuffs_gif__config_decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  return self->private_impl.f_num_decoded_frames_value;
}

// -------- func gif.config_decoder.frame_dirty_rect

WUFFS_BASE__MAYBE_STATIC wuffs_base__rect_ie_u32
wuffs_gif__config_decoder__frame_dirty_rect(
    const wuffs_gif__config_decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_rect_ie_u32();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_rect_ie_u32();
  }

  return wuffs_base__utility__empty_rect_ie_u32();
}

// -------- func gif.config_decoder.workbuf_len

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_gif__config_decoder__workbuf_len(
    const wuffs_gif__config_decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  return wuffs_base__utility__empty_range_ii_u64();
}

// -------- func gif.config_decoder.restart_frame

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__config_decoder__restart_frame(
    wuffs_gif__config_decoder* self,
    uint64_t a_index,
    uint64_t a_io_position) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }

  if (self->private_impl.f_call_sequence == 0) {
    return wuffs_base__make_status(wuffs_base__error__bad_call_sequence);
  }
  self->private_impl.f_delayed_num_decoded_frames = false;
  self->private_impl.f_end_of_data = false;
  self->private_impl.f_restarted = true;
  self->private_impl.f_frame_config_io_position = a_io_position;
  self->private_impl.f_num_decoded_frame_configs_value = a_index;
  self->private_impl.f_num_decoded_frames_value = a_index;
  wuffs_gif__config_decoder__reset_gc(self);
  return wuffs_base__make_status(NULL);
}

// -------- func gif.config_decoder.decode_frame_config

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__config_decoder__decode_frame_config(
    wuffs_gif__config_decoder* self,
    wuffs_base__frame_config* a_dst,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 3)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t v_background_color = 0;
  uint8_t v_flags = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_frame_config[0];
  if (coro_susp_point) {
    v_background_color = self->private_data.s_decode_frame_config[0].v_background_color;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    self->private_impl.f_ignore_metadata = true;
    if ( ! self->private_impl.f_end_of_data) {
      if (self->private_impl.f_call_sequence == 0) {
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        status = wuffs_gif__config_decoder__decode_image_config(self, NULL, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (status.repr) {
          goto suspend;
        }
      } else if (self->private_impl.f_call_sequence != 3) {
        if (self->private_impl.f_call_sequence == 4) {
          if (a_src) {
            a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
          }
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
          status = wuffs_gif__config_decoder__skip_frame(self, a_src);
          if (a_src) {
            iop_a_src = a_src->data.ptr + a_src->meta.ri;
          }
          if (status.repr) {
            goto suspend;
          }
        }
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
        status = wuffs_gif__config_decoder__decode_up_to_id_part1(self, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (status.repr) {
          goto suspend;
        }
      }
    }
    if (self->private_impl.f_end_of_data) {
      status = wuffs_base__make_status(wuffs_base__note__end_of_data);
      goto ok;
    }
    v_background_color = self->private_impl.f_black_color_u32_argb_premul;
    if ( ! self->private_impl.f_gc_has_transparent_index) {
      v_background_color = self->private_impl.f_background_color_u32_argb_premul;
      if (self->private_impl.f_quirks[1] && (self->private_impl.f_num_decoded_frame_configs_value == 0)) {
        while (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(4);
        }
        v_flags = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
        if ((v_flags & 128) != 0) {
          v_background_color = self->private_impl.f_black_color_u32_argb_premul;
        }
      }
    }
    if (a_dst != NULL) {
      wuffs_base__frame_config__set(
          a_dst,
          wuffs_base__utility__make_rect_ie_u32(
          wuffs_base__u32__min(self->private_impl.f_frame_rect_x0, self->private_impl.f_width),
          wuffs_base__u32__min(self->private_impl.f_frame_rect_y0, self->private_impl.f_height),
          wuffs_base__u32__min(self->private_impl.f_frame_rect_x1, self->private_impl.f_width),
          wuffs_base__u32__min(self->private_impl.f_frame_rect_y1, self->private_impl.f_height)),
          ((wuffs_base__flicks)(self->private_impl.f_gc_duration)),
          self->private_impl.f_num_decoded_frame_configs_value,
          self->private_impl.f_frame_config_io_position,
          self->private_impl.f_gc_disposal,
          ! self->private_impl.f_gc_has_transparent_index,
          false,
          v_background_color);
    }
    wuffs_base__u64__sat_add_indirect(&self->private_impl.f_num_decoded_frame_configs_value, 1);
    self->private_impl.f_call_sequence = 4;

    goto ok;
    ok:
    self->private_impl.p_decode_frame_config[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_frame_config[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 3 : 0;
  self->private_data.s_decode_frame_config[0].v_background_color = v_background_color;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func gif.config_decoder.skip_frame

static wuffs_base__status
wuffs_gif__config_decoder__skip_frame(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_flags = 0;
  uint8_t v_lw = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_skip_frame[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_0 = *iop_a_src++;
      v_flags = t_0;
    }
    if ((v_flags & 128) != 0) {
      self->private_data.s_skip_frame[0].scratch = (((uint32_t)(3)) << (1 + (v_flags & 7)));
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      if (self->private_data.s_skip_frame[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
        self->private_data.s_skip_frame[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
        iop_a_src = io2_a_src;
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      iop_a_src += self->private_data.s_skip_frame[0].scratch;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_1 = *iop_a_src++;
      v_lw = t_1;
    }
    if (v_lw > 8) {
      status = wuffs_base__make_status(wuffs_gif__error__bad_literal_width);
      goto exit;
    }
    if (a_src) {
      a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
    status = wuffs_gif__config_decoder__skip_blocks(self, a_src);
    if (a_src) {
      iop_a_src = a_src->data.ptr + a_src->meta.ri;
    }
    if (status.repr) {
      goto suspend;
    }
    if (self->private_impl.f_quirks[0]) {
      self->private_impl.f_delayed_num_decoded_frames = true;
    } else {
      wuffs_base__u64__sat_add_indirect(&self->private_impl.f_num_decoded_frames_value, 1);
    }
    wuffs_gif__config_decoder__reset_gc(self);

    goto ok;
    ok:
    self->private_impl.p_skip_frame[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_skip_frame[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.config_decoder.decode_frame

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__config_decoder__decode_frame(
    wuffs_gif__config_decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend,
    wuffs_base__slice_u8 a_workbuf,
    wuffs_base__decode_frame_options* a_opts) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 4)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  status = wuffs_base__make_status(wuffs_base__error__unsupported_method);
  goto exit;

  goto ok;
  ok:
  goto exit;
  exit:
  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func gif.config_decoder.reset_gc

static wuffs_base__empty_struct
wuffs_gif__config_decoder__reset_gc(
    wuffs_gif__config_decoder* self) {
  self->private_impl.f_call_sequence = 5;
  self->private_impl.f_gc_has_transparent_index = false;
  self->private_impl.f_gc_transparent_index = 0;
  self->private_impl.f_gc_disposal = 0;
  self->private_impl.f_gc_duration = 0;
  return wuffs_base__make_empty_struct();
}

// -------- func gif.config_decoder.decode_up_to_id_part1

static wuffs_base__status
wuffs_gif__config_decoder__decode_up_to_id_part1(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_block_type = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_up_to_id_part1[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if ( ! self->private_impl.f_restarted) {
      if (self->private_impl.f_call_sequence != 2) {
        self->private_impl.f_frame_config_io_position = wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)));
      }
    } else if (self->private_impl.f_frame_config_io_position != wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)))) {
      status = wuffs_base__make_status(wuffs_base__error__bad_restart);
      goto exit;
    } else {
      self->private_impl.f_restarted = false;
    }
    while (true) {
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        uint8_t t_0 = *iop_a_src++;
        v_block_type = t_0;
      }
      if (v_block_type == 33) {
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        status = wuffs_gif__config_decoder__decode_extension(self, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (status.repr) {
          goto suspend;
        }
      } else if (v_block_type == 44) {
        if (self->private_impl.f_delayed_num_decoded_frames) {
          self->private_impl.f_delayed_num_decoded_frames = false;
          wuffs_base__u64__sat_add_indirect(&self->private_impl.f_num_decoded_frames_value, 1);
        }
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
        status = wuffs_gif__config_decoder__decode_id_part0(self, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (status.repr) {
          goto suspend;
        }
        goto label__0__break;
      } else {
        if (self->private_impl.f_delayed_num_decoded_frames) {
          self->private_impl.f_delayed_num_decoded_frames = false;
          wuffs_base__u64__sat_add_indirect(&self->private_impl.f_num_decoded_frames_value, 1);
        }
        self->private_impl.f_end_of_data = true;
        goto label__0__break;
      }
    }
    label__0__break:;

    goto ok;
    ok:
    self->private_impl.p_decode_up_to_id_part1[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_up_to_id_part1[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.config_decoder.decode_header

static wuffs_base__status
wuffs_gif__config_decoder__decode_header(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_c[6] = {0};
  uint32_t v_i = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_header[0];
  if (coro_susp_point) {
    memcpy(v_c, self->private_data.s_decode_header[0].v_c, sizeof(v_c));
    v_i = self->private_data.s_decode_header[0].v_i;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    while (v_i < 6) {
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        uint8_t t_0 = *iop_a_src++;
        v_c[v_i] = t_0;
      }
      v_i += 1;
    }
    if ((v_c[0] != 71) ||
        (v_c[1] != 73) ||
        (v_c[2] != 70) ||
        (v_c[3] != 56) ||
        ((v_c[4] != 55) && (v_c[4] != 57)) ||
        (v_c[5] != 97)) {
      status = wuffs_base__make_status(wuffs_gif__error__bad_header);
      goto exit;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_header[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_header[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  memcpy(self->private_data.s_decode_header[0].v_c, v_c, sizeof(v_c));
  self->private_data.s_decode_header[0].v_i = v_i;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.config_decoder.decode_lsd

static wuffs_base__status
wuffs_gif__config_decoder__decode_lsd(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_flags = 0;
  uint8_t v_background_color_index = 0;
  uint32_t v_num_palette_entries = 0;
  uint32_t v_i = 0;
  uint32_t v_j = 0;
  uint32_t v_argb = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_lsd[0];
  if (coro_susp_point) {
    v_flags = self->private_data.s_decode_lsd[0].v_flags;
    v_background_color_index = self->private_data.s_decode_lsd[0].v_background_color_index;
    v_num_palette_entries = self->private_data.s_decode_lsd[0].v_num_palette_entries;
    v_i = self->private_data.s_decode_lsd[0].v_i;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      uint32_t t_0;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_0 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_lsd[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_lsd[0].scratch;
          uint32_t num_bits_0 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_0;
          if (num_bits_0 == 8) {
            t_0 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_0 += 8;
          *scratch |= ((uint64_t)(num_bits_0)) << 56;
        }
      }
      self->private_impl.f_width = t_0;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
      uint32_t t_1;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_1 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_lsd[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_lsd[0].scratch;
          uint32_t num_bits_1 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_1;
          if (num_bits_1 == 8) {
            t_1 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_1 += 8;
          *scratch |= ((uint64_t)(num_bits_1)) << 56;
        }
      }
      self->private_impl.f_height = t_1;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(5);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_2 = *iop_a_src++;
      v_flags = t_2;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_3 = *iop_a_src++;
      v_background_color_index = t_3;
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(7);
    if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
      status = wuffs_base__make_status(wuffs_base__suspension__short_read);
      goto suspend;
    }
    iop_a_src++;
    v_i = 0;
    self->private_impl.f_has_global_palette = ((v_flags & 128) != 0);
    if (self->private_impl.f_has_global_palette) {
      v_num_palette_entries = (((uint32_t)(1)) << (1 + (v_flags & 7)));
      while (v_i < v_num_palette_entries) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(8);
          uint32_t t_4;
          if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 3)) {
            t_4 = ((uint32_t)(wuffs_base__load_u24be__no_bounds_check(iop_a_src)));
            iop_a_src += 3;
          } else {
            self->private_data.s_decode_lsd[0].scratch = 0;
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(9);
            while (true) {
              if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
                status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                goto suspend;
              }
              uint64_t* scratch = &self->private_data.s_decode_lsd[0].scratch;
              uint32_t num_bits_4 = ((uint32_t)(*scratch & 0xFF));
              *scratch >>= 8;
              *scratch <<= 8;
              *scratch |= ((uint64_t)(*iop_a_src++)) << (56 - num_bits_4);
              if (num_bits_4 == 16) {
                t_4 = ((uint32_t)(*scratch >> 40));
                break;
              }
              num_bits_4 += 8;
              *scratch |= ((uint64_t)(num_bits_4));
            }
          }
          v_argb = t_4;
        }
        v_argb |= 4278190080;
        self->private_data.f_palettes[0][((4 * v_i) + 0)] = ((uint8_t)(((v_argb >> 0) & 255)));
        self->private_data.f_palettes[0][((4 * v_i) + 1)] = ((uint8_t)(((v_argb >> 8) & 255)));
        self->private_data.f_palettes[0][((4 * v_i) + 2)] = ((uint8_t)(((v_argb >> 16) & 255)));
        self->private_data.f_palettes[0][((4 * v_i) + 3)] = ((uint8_t)(((v_argb >> 24) & 255)));
        v_i += 1;
      }
      if (self->private_impl.f_quirks[2]) {
        if ((v_background_color_index != 0) && (((uint32_t)(v_background_color_index)) < v_num_palette_entries)) {
          v_j = (4 * ((uint32_t)(v_background_color_index)));
          self->private_impl.f_background_color_u32_argb_premul = ((((uint32_t)(self->private_data.f_palettes[0][(v_j + 0)])) << 0) |
              (((uint32_t)(self->private_data.f_palettes[0][(v_j + 1)])) << 8) |
              (((uint32_t)(self->private_data.f_palettes[0][(v_j + 2)])) << 16) |
              (((uint32_t)(self->private_data.f_palettes[0][(v_j + 3)])) << 24));
        } else {
          self->private_impl.f_background_color_u32_argb_premul = 77;
        }
      }
    }
    while (v_i < 256) {
      self->private_data.f_palettes[0][((4 * v_i) + 0)] = 0;
      self->private_data.f_palettes[0][((4 * v_i) + 1)] = 0;
      self->private_data.f_palettes[0][((4 * v_i) + 2)] = 0;
      self->private_data.f_palettes[0][((4 * v_i) + 3)] = 255;
      v_i += 1;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_lsd[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_lsd[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_data.s_decode_lsd[0].v_flags = v_flags;
  self->private_data.s_decode_lsd[0].v_background_color_index = v_background_color_index;
  self->private_data.s_decode_lsd[0].v_num_palette_entries = v_num_palette_entries;
  self->private_data.s_decode_lsd[0].v_i = v_i;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.config_decoder.decode_extension

static wuffs_base__status
wuffs_gif__config_decoder__decode_extension(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_label = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_extension[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_0 = *iop_a_src++;
      v_label = t_0;
    }
    if (v_label == 249) {
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      status = wuffs_gif__config_decoder__decode_gc(self, a_src);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
      status = wuffs_base__make_status(NULL);
      goto ok;
    } else if (v_label == 255) {
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
      status = wuffs_gif__config_decoder__decode_ae(self, a_src);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
      status = wuffs_base__make_status(NULL);
      goto ok;
    }
    if (a_src) {
      a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
    status = wuffs_gif__config_decoder__skip_blocks(self, a_src);
    if (a_src) {
      iop_a_src = a_src->data.ptr + a_src->meta.ri;
    }
    if (status.repr) {
      goto suspend;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_extension[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_extension[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.config_decoder.skip_blocks

static wuffs_base__status
wuffs_gif__config_decoder__skip_blocks(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_block_size = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_skip_blocks[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    while (true) {
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        uint8_t t_0 = *iop_a_src++;
        v_block_size = t_0;
      }
      if (v_block_size == 0) {
        status = wuffs_base__make_status(NULL);
        goto ok;
      }
      self->private_data.s_skip_blocks[0].scratch = ((uint32_t)(v_block_size));
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      if (self->private_data.s_skip_blocks[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
        self->private_data.s_skip_blocks[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
        iop_a_src = io2_a_src;
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      iop_a_src += self->private_data.s_skip_blocks[0].scratch;
    }

    goto ok;
    ok:
    self->private_impl.p_skip_blocks[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_skip_blocks[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.config_decoder.decode_ae

static wuffs_base__status
wuffs_gif__config_decoder__decode_ae(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_c = 0;
  uint8_t v_block_size = 0;
  bool v_is_animexts = false;
  bool v_is_netscape = false;
  bool v_is_iccp = false;
  bool v_is_xmp = false;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_ae[0];
  if (coro_susp_point) {
    v_block_size = self->private_data.s_decode_ae[0].v_block_size;
    v_is_animexts = self->private_data.s_decode_ae[0].v_is_animexts;
    v_is_netscape = self->private_data.s_decode_ae[0].v_is_netscape;
    v_is_iccp = self->private_data.s_decode_ae[0].v_is_iccp;
    v_is_xmp = self->private_data.s_decode_ae[0].v_is_xmp;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    while (true) {
      if (self->private_impl.f_metadata_fourcc != 0) {
        status = wuffs_base__make_status(wuffs_base__note__metadata_reported);
        goto ok;
      }
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        uint8_t t_0 = *iop_a_src++;
        v_block_size = t_0;
      }
      if (v_block_size == 0) {
        status = wuffs_base__make_status(NULL);
        goto ok;
      }
      if (v_block_size != 11) {
        self->private_data.s_decode_ae[0].scratch = ((uint32_t)(v_block_size));
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        if (self->private_data.s_decode_ae[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
          self->private_data.s_decode_ae[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
          iop_a_src = io2_a_src;
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        iop_a_src += self->private_data.s_decode_ae[0].scratch;
        goto label__goto_done__break;
      }
      v_is_animexts = true;
      v_is_netscape = true;
      v_is_iccp = true;
      v_is_xmp = true;
      v_block_size = 0;
      while (v_block_size < 11) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint8_t t_1 = *iop_a_src++;
          v_c = t_1;
        }
        v_is_animexts = (v_is_animexts && (v_c == WUFFS_GIF__ANIMEXTS1DOT0[v_block_size]));
        v_is_netscape = (v_is_netscape && (v_c == WUFFS_GIF__NETSCAPE2DOT0[v_block_size]));
        v_is_iccp = (v_is_iccp && (v_c == WUFFS_GIF__ICCRGBG1012[v_block_size]));
        v_is_xmp = (v_is_xmp && (v_c == WUFFS_GIF__XMPDATAXMP[v_block_size]));
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
        v_block_size += 1;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
      }
      if (v_is_animexts || v_is_netscape) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint8_t t_2 = *iop_a_src++;
          v_block_size = t_2;
        }
        if (v_block_size != 3) {
          self->private_data.s_decode_ae[0].scratch = ((uint32_t)(v_block_size));
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(5);
          if (self->private_data.s_decode_ae[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
            self->private_data.s_decode_ae[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
            iop_a_src = io2_a_src;
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          iop_a_src += self->private_data.s_decode_ae[0].scratch;
          goto label__goto_done__break;
        }
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint8_t t_3 = *iop_a_src++;
          v_c = t_3;
        }
        if (v_c != 1) {
          self->private_data.s_decode_ae[0].scratch = 2;
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(7);
          if (self->private_data.s_decode_ae[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
            self->private_data.s_decode_ae[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
            iop_a_src = io2_a_src;
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          iop_a_src += self->private_data.s_decode_ae[0].scratch;
          goto label__goto_done__break;
        }
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(8);
          uint32_t t_4;
          if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
            t_4 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
            iop_a_src += 2;
          } else {
            self->private_data.s_decode_ae[0].scratch = 0;
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(9);
            while (true) {
              if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
                status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                goto suspend;
              }
              uint64_t* scratch = &self->private_data.s_decode_ae[0].scratch;
              uint32_t num_bits_4 = ((uint32_t)(*scratch >> 56));
              *scratch <<= 8;
              *scratch >>= 8;
              *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_4;
              if (num_bits_4 == 8) {
                t_4 = ((uint32_t)(*scratch));
                break;
              }
              num_bits_4 += 8;
              *scratch |= ((uint64_t)(num_bits_4)) << 56;
            }
          }
          self->private_impl.f_num_loops = t_4;
        }
        self->private_impl.f_seen_num_loops = true;
        if ((0 < self->private_impl.f_num_loops) && (self->private_impl.f_num_loops <= 65535)) {
          self->private_impl.f_num_loops += 1;
        }
      } else if (self->private_impl.f_ignore_metadata) {
      } else if (v_is_iccp && self->private_impl.f_report_metadata_iccp) {
        self->private_impl.f_metadata_fourcc = 1229144912;
        self->private_impl.f_metadata_io_position = wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)));
        self->private_impl.f_call_sequence = 1;
        status = wuffs_base__make_status(wuffs_base__note__metadata_reported);
        goto ok;
      } else if (v_is_xmp && self->private_impl.f_report_metadata_xmp) {
        self->private_impl.f_metadata_fourcc = 1481461792;
        self->private_impl.f_metadata_io_position = wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)));
        self->private_impl.f_call_sequence = 1;
        status = wuffs_base__make_status(wuffs_base__note__metadata_reported);
        goto ok;
      }
      goto label__goto_done__break;
    }
    label__goto_done__break:;
    if (a_src) {
      a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(10);
    status = wuffs_gif__config_decoder__skip_blocks(self, a_src);
    if (a_src) {
      iop_a_src = a_src->data.ptr + a_src->meta.ri;
    }
    if (status.repr) {
      goto suspend;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_ae[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_ae[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_data.s_decode_ae[0].v_block_size = v_block_size;
  self->private_data.s_decode_ae[0].v_is_animexts = v_is_animexts;
  self->private_data.s_decode_ae[0].v_is_netscape = v_is_netscape;
  self->private_data.s_decode_ae[0].v_is_iccp = v_is_iccp;
  self->private_data.s_decode_ae[0].v_is_xmp = v_is_xmp;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.config_decoder.decode_gc

static wuffs_base__status
wuffs_gif__config_decoder__decode_gc(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_c = 0;
  uint8_t v_flags = 0;
  uint16_t v_gc_duration_centiseconds = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_gc[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_0 = *iop_a_src++;
      v_c = t_0;
    }
    if (v_c != 4) {
      status = wuffs_base__make_status(wuffs_gif__error__bad_graphic_control);
      goto exit;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_1 = *iop_a_src++;
      v_flags = t_1;
    }
    self->private_impl.f_gc_has_transparent_index = ((v_flags & 1) != 0);
    v_flags = ((v_flags >> 2) & 7);
    if (v_flags == 2) {
      self->private_impl.f_gc_disposal = 1;
    } else if ((v_flags == 3) || (v_flags == 4)) {
      self->private_impl.f_gc_disposal = 2;
    } else {
      self->private_impl.f_gc_disposal = 0;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
      uint16_t t_2;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_2 = wuffs_base__load_u16le__no_bounds_check(iop_a_src);
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_gc[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_gc[0].scratch;
          uint32_t num_bits_2 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_2;
          if (num_bits_2 == 8) {
            t_2 = ((uint16_t)(*scratch));
            break;
          }
          num_bits_2 += 8;
          *scratch |= ((uint64_t)(num_bits_2)) << 56;
        }
      }
      v_gc_duration_centiseconds = t_2;
    }
    self->private_impl.f_gc_duration = (((uint64_t)(v_gc_duration_centiseconds)) * 7056000);
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(5);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_3 = *iop_a_src++;
      self->private_impl.f_gc_transparent_index = t_3;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_4 = *iop_a_src++;
      v_c = t_4;
    }
    if (v_c != 0) {
      status = wuffs_base__make_status(wuffs_gif__error__bad_graphic_control);
      goto exit;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_gc[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_gc[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.config_decoder.decode_id_part0

static wuffs_base__status
wuffs_gif__config_decoder__decode_id_part0(
    wuffs_gif__config_decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_id_part0[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      uint32_t t_0;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_0 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_id_part0[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_id_part0[0].scratch;
          uint32_t num_bits_0 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_0;
          if (num_bits_0 == 8) {
            t_0 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_0 += 8;
          *scratch |= ((uint64_t)(num_bits_0)) << 56;
        }
      }
      self->private_impl.f_frame_rect_x0 = t_0;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
      uint32_t t_1;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_1 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_id_part0[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_id_part0[0].scratch;
          uint32_t num_bits_1 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_1;
          if (num_bits_1 == 8) {
            t_1 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_1 += 8;
          *scratch |= ((uint64_t)(num_bits_1)) << 56;
        }
      }
      self->private_impl.f_frame_rect_y0 = t_1;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(5);
      uint32_t t_2;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_2 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_id_part0[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_id_part0[0].scratch;
          uint32_t num_bits_2 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_2;
          if (num_bits_2 == 8) {
            t_2 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_2 += 8;
          *scratch |= ((uint64_t)(num_bits_2)) << 56;
        }
      }
      self->private_impl.f_frame_rect_x1 = t_2;
    }
    self->private_impl.f_frame_rect_x1 += self->private_impl.f_frame_rect_x0;
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(7);
      uint32_t t_3;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_3 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_id_part0[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(8);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_id_part0[0].scratch;
          uint32_t num_bits_3 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_3;
          if (num_bits_3 == 8) {
            t_3 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_3 += 8;
          *scratch |= ((uint64_t)(num_bits_3)) << 56;
        }
      }
      self->private_impl.f_frame_rect_y1 = t_3;
    }
    self->private_impl.f_frame_rect_y1 += self->private_impl.f_frame_rect_y0;
    if ((self->private_impl.f_call_sequence == 0) &&  ! self->private_impl.f_quirks[4]) {
      self->private_impl.f_width = wuffs_base__u32__max(self->private_impl.f_width, self->private_impl.f_frame_rect_x1);
      self->private_impl.f_height = wuffs_base__u32__max(self->private_impl.f_height, self->private_impl.f_frame_rect_y1);
    }

    goto ok;
    ok:
    self->private_impl.p_decode_id_part0[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_id_part0[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.set_quirk_enabled

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gif__decoder__set_quirk_enabled(
    wuffs_gif__decoder* self,
    uint32_t a_quirk,
    bool a_enabled) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  if ((self->private_impl.f_call_sequence == 0) && (a_quirk >= 1041635328)) {
    a_quirk -= 1041635328;
    if (a_quirk < 7) {
      self->private_impl.f_quirks[a_quirk] = a_enabled;
    }
  }
  return wuffs_base__make_empty_struct();
}

// -------- func gif.decoder.decode_image_config

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__decoder__decode_image_config(
    wuffs_gif__decoder* self,
    wuffs_base__image_config* a_dst,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 1)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  bool v_ffio = false;

  uint32_t coro_susp_point = self->private_impl.p_decode_image_config[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if (self->private_impl.f_call_sequence == 0) {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      status = wuffs_gif__decoder__decode_header(self, a_src);
      if (status.repr) {
        goto suspend;
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      status = wuffs_gif__decoder__decode_lsd(self, a_src);
      if (status.repr) {
        goto suspend;
      }
    } else if (self->private_impl.f_call_sequence != 2) {
      status = wuffs_base__make_status(wuffs_base__error__bad_call_sequence);
      goto exit;
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
    status = wuffs_gif__decoder__decode_up_to_id_part1(self, a_src);
    if (status.repr) {
      goto suspend;
    }
    v_ffio =  ! self->private_impl.f_gc_has_transparent_index;
    if ( ! self->private_impl.f_quirks[2]) {
      v_ffio = (v_ffio &&
          (self->private_impl.f_frame_rect_x0 == 0) &&
          (self->private_impl.f_frame_rect_y0 == 0) &&
          (self->private_impl.f_frame_rect_x1 == self->private_impl.f_width) &&
          (self->private_impl.f_frame_rect_y1 == self->private_impl.f_height));
    } else if (v_ffio) {
      self->private_impl.f_black_color_u32_argb_premul = 4278190080;
    }
    if (self->private_impl.f_background_color_u32_argb_premul == 77) {
      self->private_impl.f_background_color_u32_argb_premul = self->private_impl.f_black_color_u32_argb_premul;
    }
    if (a_dst != NULL) {
      wuffs_base__image_config__set(
          a_dst,
          2198077448,
          0,
          self->private_impl.f_width,
          self->private_impl.f_height,
          self->private_impl.f_frame_config_io_position,
          v_ffio);
    }
    self->private_impl.f_call_sequence = 3;

    goto ok;
    ok:
    self->private_impl.p_decode_image_config[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_image_config[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 1 : 0;

  goto exit;
  exit:
  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func gif.decoder.set_report_metadata

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gif__decoder__set_report_metadata(
    wuffs_gif__decoder* self,
    uint32_t a_fourcc,
    bool a_report) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  if (a_fourcc == 1229144912) {
    self->private_impl.f_report_metadata_iccp = a_report;
  } else if (a_fourcc == 1481461792) {
    self->private_impl.f_report_metadata_xmp = a_report;
  }
  return wuffs_base__make_empty_struct();
}

// -------- func gif.decoder.tell_me_more

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__decoder__tell_me_more(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__more_information* a_minfo,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 2)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint64_t v_chunk_length = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_tell_me_more[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if (self->private_impl.f_call_sequence != 1) {
      status = wuffs_base__make_status(wuffs_base__error__bad_call_sequence);
      goto exit;
    }
    if (self->private_impl.f_metadata_fourcc == 0) {
      status = wuffs_base__make_status(wuffs_base__error__no_more_information);
      goto exit;
    }
    while (true) {
      label__0__continue:;
      while (true) {
        if (wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src))) != self->private_impl.f_metadata_io_position) {
          if (a_minfo != NULL) {
            wuffs_base__more_information__set(a_minfo,
                2,
                0,
                self->private_impl.f_metadata_io_position,
                0,
                0);
          }
          status = wuffs_base__make_status(wuffs_base__suspension__mispositioned_read);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(1);
          goto label__0__continue;
        }
        if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
          if (a_minfo != NULL) {
            wuffs_base__more_information__set(a_minfo,
                0,
                0,
                0,
                0,
                0);
          }
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(2);
          goto label__0__continue;
        }
        goto label__0__break;
      }
      label__0__break:;
      v_chunk_length = ((uint64_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src)));
      if (v_chunk_length <= 0) {
        (iop_a_src += 1, wuffs_base__make_empty_struct());
        goto label__1__break;
      }
      if (self->private_impl.f_metadata_fourcc == 1481461792) {
        v_chunk_length += 1;
      } else {
        (iop_a_src += 1, wuffs_base__make_empty_struct());
      }
      self->private_impl.f_metadata_io_position = wuffs_base__u64__sat_add(wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src))), v_chunk_length);
      if (a_minfo != NULL) {
        wuffs_base__more_information__set(a_minfo,
            3,
            self->private_impl.f_metadata_fourcc,
            0,
            wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src))),
            self->private_impl.f_metadata_io_position);
      }
      status = wuffs_base__make_status(wuffs_base__suspension__even_more_information);
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(3);
    }
    label__1__break:;
    if (a_minfo != NULL) {
      wuffs_base__more_information__set(a_minfo,
          3,
          self->private_impl.f_metadata_fourcc,
          0,
          self->private_impl.f_metadata_io_position,
          self->private_impl.f_metadata_io_position);
    }
    self->private_impl.f_call_sequence = 2;
    self->private_impl.f_metadata_fourcc = 0;
    self->private_impl.f_metadata_io_position = 0;
    status = wuffs_base__make_status(NULL);
    goto ok;

    goto ok;
    ok:
    self->private_impl.p_tell_me_more[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_tell_me_more[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 2 : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func gif.decoder.num_animation_loops

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_gif__decoder__num_animation_loops(
    const wuffs_gif__decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  if (self->private_impl.f_seen_num_loops) {
    return self->private_impl.f_num_loops;
  }
  return 1;
}

// -------- func gif.decoder.num_decoded_frame_configs

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_gif__decoder__num_decoded_frame_configs(
    const wuffs_gif__decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  return self->private_impl.f_num_decoded_frame_configs_value;
}

// -------- func gif.decoder.num_decoded_frames

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_gif__decoder__num_decoded_frames(
    const wuffs_gif__decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  return self->private_impl.f_num_decoded_frames_value;
}

// -------- func gif.decoder.frame_dirty_rect

WUFFS_BASE__MAYBE_STATIC wuffs_base__rect_ie_u32
wuffs_gif__decoder__frame_dirty_rect(
    const wuffs_gif__decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_rect_ie_u32();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_rect_ie_u32();
  }

  return wuffs_base__utility__make_rect_ie_u32(
      wuffs_base__u32__min(self->private_impl.f_frame_rect_x0, self->private_impl.f_width),
      wuffs_base__u32__min(self->private_impl.f_frame_rect_y0, self->private_impl.f_height),
      wuffs_base__u32__min(self->private_impl.f_frame_rect_x1, self->private_impl.f_width),
      wuffs_base__u32__min(self->private_impl.f_dirty_max_excl_y, self->private_impl.f_height));
}

// -------- func gif.decoder.workbuf_len

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_gif__decoder__workbuf_len(
    const wuffs_gif__decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  return wuffs_base__utility__make_range_ii_u64(0, 0);
}

// -------- func gif.decoder.restart_frame

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__decoder__restart_frame(
    wuffs_gif__decoder* self,
    uint64_t a_index,
    uint64_t a_io_position) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }

  if (self->private_impl.f_call_sequence == 0) {
    return wuffs_base__make_status(wuffs_base__error__bad_call_sequence);
  }
  self->private_impl.f_delayed_num_decoded_frames = false;
  self->private_impl.f_end_of_data = false;
  self->private_impl.f_restarted = true;
  self->private_impl.f_frame_config_io_position = a_io_position;
  self->private_impl.f_num_decoded_frame_configs_value = a_index;
  self->private_impl.f_num_decoded_frames_value = a_index;
  wuffs_gif__decoder__reset_gc(self);
  return wuffs_base__make_status(NULL);
}

// -------- func gif.decoder.decode_frame_config

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__decoder__decode_frame_config(
    wuffs_gif__decoder* self,
    wuffs_base__frame_config* a_dst,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 3)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t v_background_color = 0;
  uint8_t v_flags = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_frame_config[0];
  if (coro_susp_point) {
    v_background_color = self->private_data.s_decode_frame_config[0].v_background_color;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    self->private_impl.f_ignore_metadata = true;
    self->private_impl.f_dirty_max_excl_y = 0;
    if ( ! self->private_impl.f_end_of_data) {
      if (self->private_impl.f_call_sequence == 0) {
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        status = wuffs_gif__decoder__decode_image_config(self, NULL, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (status.repr) {
          goto suspend;
        }
      } else if (self->private_impl.f_call_sequence != 3) {
        if (self->private_impl.f_call_sequence == 4) {
          if (a_src) {
            a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
          }
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
          status = wuffs_gif__decoder__skip_frame(self, a_src);
          if (a_src) {
            iop_a_src = a_src->data.ptr + a_src->meta.ri;
          }
          if (status.repr) {
            goto suspend;
          }
        }
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
        status = wuffs_gif__decoder__decode_up_to_id_part1(self, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (status.repr) {
          goto suspend;
        }
      }
    }
    if (self->private_impl.f_end_of_data) {
      status = wuffs_base__make_status(wuffs_base__note__end_of_data);
      goto ok;
    }
    v_background_color = self->private_impl.f_black_color_u32_argb_premul;
    if ( ! self->private_impl.f_gc_has_transparent_index) {
      v_background_color = self->private_impl.f_background_color_u32_argb_premul;
      if (self->private_impl.f_quirks[1] && (self->private_impl.f_num_decoded_frame_configs_value == 0)) {
        while (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(4);
        }
        v_flags = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
        if ((v_flags & 128) != 0) {
          v_background_color = self->private_impl.f_black_color_u32_argb_premul;
        }
      }
    }
    if (a_dst != NULL) {
      wuffs_base__frame_config__set(
          a_dst,
          wuffs_base__utility__make_rect_ie_u32(
          wuffs_base__u32__min(self->private_impl.f_frame_rect_x0, self->private_impl.f_width),
          wuffs_base__u32__min(self->private_impl.f_frame_rect_y0, self->private_impl.f_height),
          wuffs_base__u32__min(self->private_impl.f_frame_rect_x1, self->private_impl.f_width),
          wuffs_base__u32__min(self->private_impl.f_frame_rect_y1, self->private_impl.f_height)),
          ((wuffs_base__flicks)(self->private_impl.f_gc_duration)),
          self->private_impl.f_num_decoded_frame_configs_value,
          self->private_impl.f_frame_config_io_position,
          self->private_impl.f_gc_disposal,
          ! self->private_impl.f_gc_has_transparent_index,
          false,
          v_background_color);
    }
    wuffs_base__u64__sat_add_indirect(&self->private_impl.f_num_decoded_frame_configs_value, 1);
    self->private_impl.f_call_sequence = 4;

    goto ok;
    ok:
    self->private_impl.p_decode_frame_config[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_frame_config[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 3 : 0;
  self->private_data.s_decode_frame_config[0].v_background_color = v_background_color;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func gif.decoder.skip_frame

static wuffs_base__status
wuffs_gif__decoder__skip_frame(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_flags = 0;
  uint8_t v_lw = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_skip_frame[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_0 = *iop_a_src++;
      v_flags = t_0;
    }
    if ((v_flags & 128) != 0) {
      self->private_data.s_skip_frame[0].scratch = (((uint32_t)(3)) << (1 + (v_flags & 7)));
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      if (self->private_data.s_skip_frame[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
        self->private_data.s_skip_frame[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
        iop_a_src = io2_a_src;
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      iop_a_src += self->private_data.s_skip_frame[0].scratch;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_1 = *iop_a_src++;
      v_lw = t_1;
    }
    if (v_lw > 8) {
      status = wuffs_base__make_status(wuffs_gif__error__bad_literal_width);
      goto exit;
    }
    if (a_src) {
      a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
    status = wuffs_gif__decoder__skip_blocks(self, a_src);
    if (a_src) {
      iop_a_src = a_src->data.ptr + a_src->meta.ri;
    }
    if (status.repr) {
      goto suspend;
    }
    if (self->private_impl.f_quirks[0]) {
      self->private_impl.f_delayed_num_decoded_frames = true;
    } else {
      wuffs_base__u64__sat_add_indirect(&self->private_impl.f_num_decoded_frames_value, 1);
    }
    wuffs_gif__decoder__reset_gc(self);

    goto ok;
    ok:
    self->private_impl.p_skip_frame[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_skip_frame[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.decode_frame

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gif__decoder__decode_frame(
    wuffs_gif__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend,
    wuffs_base__slice_u8 a_workbuf,
    wuffs_base__decode_frame_options* a_opts) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 4)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t coro_susp_point = self->private_impl.p_decode_frame[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    self->private_impl.f_ignore_metadata = true;
    if (self->private_impl.f_call_sequence != 4) {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      status = wuffs_gif__decoder__decode_frame_config(self, NULL, a_src);
      if (status.repr) {
        goto suspend;
      }
    }
    if (self->private_impl.f_quirks[5] && ((self->private_impl.f_frame_rect_x0 == self->private_impl.f_frame_rect_x1) || (self->private_impl.f_frame_rect_y0 == self->private_impl.f_frame_rect_y1))) {
      status = wuffs_base__make_status(wuffs_gif__error__bad_frame_size);
      goto exit;
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
    status = wuffs_gif__decoder__decode_id_part1(self, a_dst, a_src, a_blend);
    if (status.repr) {
      goto suspend;
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
    status = wuffs_gif__decoder__decode_id_part2(self, a_dst, a_src, a_workbuf);
    if (status.repr) {
      goto suspend;
    }
    wuffs_base__u64__sat_add_indirect(&self->private_impl.f_num_decoded_frames_value, 1);
    wuffs_gif__decoder__reset_gc(self);

    goto ok;
    ok:
    self->private_impl.p_decode_frame[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_frame[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 4 : 0;

  goto exit;
  exit:
  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func gif.decoder.reset_gc

static wuffs_base__empty_struct
wuffs_gif__decoder__reset_gc(
    wuffs_gif__decoder* self) {
  self->private_impl.f_call_sequence = 5;
  self->private_impl.f_gc_has_transparent_index = false;
  self->private_impl.f_gc_transparent_index = 0;
  self->private_impl.f_gc_disposal = 0;
  self->private_impl.f_gc_duration = 0;
  return wuffs_base__make_empty_struct();
}

// -------- func gif.decoder.decode_up_to_id_part1

static wuffs_base__status
wuffs_gif__decoder__decode_up_to_id_part1(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_block_type = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_up_to_id_part1[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if ( ! self->private_impl.f_restarted) {
      if (self->private_impl.f_call_sequence != 2) {
        self->private_impl.f_frame_config_io_position = wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)));
      }
    } else if (self->private_impl.f_frame_config_io_position != wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)))) {
      status = wuffs_base__make_status(wuffs_base__error__bad_restart);
      goto exit;
    } else {
      self->private_impl.f_restarted = false;
    }
    while (true) {
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        uint8_t t_0 = *iop_a_src++;
        v_block_type = t_0;
      }
      if (v_block_type == 33) {
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        status = wuffs_gif__decoder__decode_extension(self, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (status.repr) {
          goto suspend;
        }
      } else if (v_block_type == 44) {
        if (self->private_impl.f_delayed_num_decoded_frames) {
          self->private_impl.f_delayed_num_decoded_frames = false;
          wuffs_base__u64__sat_add_indirect(&self->private_impl.f_num_decoded_frames_value, 1);
        }
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
        status = wuffs_gif__decoder__decode_id_part0(self, a_src);
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        if (status.repr) {
          goto suspend;
        }
        goto label__0__break;
      } else {
        if (self->private_impl.f_delayed_num_decoded_frames) {
          self->private_impl.f_delayed_num_decoded_frames = false;
          wuffs_base__u64__sat_add_indirect(&self->private_impl.f_num_decoded_frames_value, 1);
        }
        self->private_impl.f_end_of_data = true;
        goto label__0__break;
      }
    }
    label__0__break:;

    goto ok;
    ok:
    self->private_impl.p_decode_up_to_id_part1[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_up_to_id_part1[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.decode_header

static wuffs_base__status
wuffs_gif__decoder__decode_header(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_c[6] = {0};
  uint32_t v_i = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_header[0];
  if (coro_susp_point) {
    memcpy(v_c, self->private_data.s_decode_header[0].v_c, sizeof(v_c));
    v_i = self->private_data.s_decode_header[0].v_i;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    while (v_i < 6) {
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        uint8_t t_0 = *iop_a_src++;
        v_c[v_i] = t_0;
      }
      v_i += 1;
    }
    if ((v_c[0] != 71) ||
        (v_c[1] != 73) ||
        (v_c[2] != 70) ||
        (v_c[3] != 56) ||
        ((v_c[4] != 55) && (v_c[4] != 57)) ||
        (v_c[5] != 97)) {
      status = wuffs_base__make_status(wuffs_gif__error__bad_header);
      goto exit;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_header[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_header[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  memcpy(self->private_data.s_decode_header[0].v_c, v_c, sizeof(v_c));
  self->private_data.s_decode_header[0].v_i = v_i;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.decode_lsd

static wuffs_base__status
wuffs_gif__decoder__decode_lsd(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_flags = 0;
  uint8_t v_background_color_index = 0;
  uint32_t v_num_palette_entries = 0;
  uint32_t v_i = 0;
  uint32_t v_j = 0;
  uint32_t v_argb = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_lsd[0];
  if (coro_susp_point) {
    v_flags = self->private_data.s_decode_lsd[0].v_flags;
    v_background_color_index = self->private_data.s_decode_lsd[0].v_background_color_index;
    v_num_palette_entries = self->private_data.s_decode_lsd[0].v_num_palette_entries;
    v_i = self->private_data.s_decode_lsd[0].v_i;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      uint32_t t_0;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_0 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_lsd[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_lsd[0].scratch;
          uint32_t num_bits_0 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_0;
          if (num_bits_0 == 8) {
            t_0 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_0 += 8;
          *scratch |= ((uint64_t)(num_bits_0)) << 56;
        }
      }
      self->private_impl.f_width = t_0;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
      uint32_t t_1;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_1 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_lsd[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_lsd[0].scratch;
          uint32_t num_bits_1 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_1;
          if (num_bits_1 == 8) {
            t_1 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_1 += 8;
          *scratch |= ((uint64_t)(num_bits_1)) << 56;
        }
      }
      self->private_impl.f_height = t_1;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(5);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_2 = *iop_a_src++;
      v_flags = t_2;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_3 = *iop_a_src++;
      v_background_color_index = t_3;
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(7);
    if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
      status = wuffs_base__make_status(wuffs_base__suspension__short_read);
      goto suspend;
    }
    iop_a_src++;
    v_i = 0;
    self->private_impl.f_has_global_palette = ((v_flags & 128) != 0);
    if (self->private_impl.f_has_global_palette) {
      v_num_palette_entries = (((uint32_t)(1)) << (1 + (v_flags & 7)));
      while (v_i < v_num_palette_entries) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(8);
          uint32_t t_4;
          if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 3)) {
            t_4 = ((uint32_t)(wuffs_base__load_u24be__no_bounds_check(iop_a_src)));
            iop_a_src += 3;
          } else {
            self->private_data.s_decode_lsd[0].scratch = 0;
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(9);
            while (true) {
              if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
                status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                goto suspend;
              }
              uint64_t* scratch = &self->private_data.s_decode_lsd[0].scratch;
              uint32_t num_bits_4 = ((uint32_t)(*scratch & 0xFF));
              *scratch >>= 8;
              *scratch <<= 8;
              *scratch |= ((uint64_t)(*iop_a_src++)) << (56 - num_bits_4);
              if (num_bits_4 == 16) {
                t_4 = ((uint32_t)(*scratch >> 40));
                break;
              }
              num_bits_4 += 8;
              *scratch |= ((uint64_t)(num_bits_4));
            }
          }
          v_argb = t_4;
        }
        v_argb |= 4278190080;
        self->private_data.f_palettes[0][((4 * v_i) + 0)] = ((uint8_t)(((v_argb >> 0) & 255)));
        self->private_data.f_palettes[0][((4 * v_i) + 1)] = ((uint8_t)(((v_argb >> 8) & 255)));
        self->private_data.f_palettes[0][((4 * v_i) + 2)] = ((uint8_t)(((v_argb >> 16) & 255)));
        self->private_data.f_palettes[0][((4 * v_i) + 3)] = ((uint8_t)(((v_argb >> 24) & 255)));
        v_i += 1;
      }
      if (self->private_impl.f_quirks[2]) {
        if ((v_background_color_index != 0) && (((uint32_t)(v_background_color_index)) < v_num_palette_entries)) {
          v_j = (4 * ((uint32_t)(v_background_color_index)));
          self->private_impl.f_background_color_u32_argb_premul = ((((uint32_t)(self->private_data.f_palettes[0][(v_j + 0)])) << 0) |
              (((uint32_t)(self->private_data.f_palettes[0][(v_j + 1)])) << 8) |
              (((uint32_t)(self->private_data.f_palettes[0][(v_j + 2)])) << 16) |
              (((uint32_t)(self->private_data.f_palettes[0][(v_j + 3)])) << 24));
        } else {
          self->private_impl.f_background_color_u32_argb_premul = 77;
        }
      }
    }
    while (v_i < 256) {
      self->private_data.f_palettes[0][((4 * v_i) + 0)] = 0;
      self->private_data.f_palettes[0][((4 * v_i) + 1)] = 0;
      self->private_data.f_palettes[0][((4 * v_i) + 2)] = 0;
      self->private_data.f_palettes[0][((4 * v_i) + 3)] = 255;
      v_i += 1;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_lsd[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_lsd[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_data.s_decode_lsd[0].v_flags = v_flags;
  self->private_data.s_decode_lsd[0].v_background_color_index = v_background_color_index;
  self->private_data.s_decode_lsd[0].v_num_palette_entries = v_num_palette_entries;
  self->private_data.s_decode_lsd[0].v_i = v_i;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.decode_extension

static wuffs_base__status
wuffs_gif__decoder__decode_extension(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_label = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_extension[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_0 = *iop_a_src++;
      v_label = t_0;
    }
    if (v_label == 249) {
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      status = wuffs_gif__decoder__decode_gc(self, a_src);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
      status = wuffs_base__make_status(NULL);
      goto ok;
    } else if (v_label == 255) {
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
      status = wuffs_gif__decoder__decode_ae(self, a_src);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
      status = wuffs_base__make_status(NULL);
      goto ok;
    }
    if (a_src) {
      a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
    status = wuffs_gif__decoder__skip_blocks(self, a_src);
    if (a_src) {
      iop_a_src = a_src->data.ptr + a_src->meta.ri;
    }
    if (status.repr) {
      goto suspend;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_extension[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_extension[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.skip_blocks

static wuffs_base__status
wuffs_gif__decoder__skip_blocks(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_block_size = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_skip_blocks[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    while (true) {
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        uint8_t t_0 = *iop_a_src++;
        v_block_size = t_0;
      }
      if (v_block_size == 0) {
        status = wuffs_base__make_status(NULL);
        goto ok;
      }
      self->private_data.s_skip_blocks[0].scratch = ((uint32_t)(v_block_size));
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      if (self->private_data.s_skip_blocks[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
        self->private_data.s_skip_blocks[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
        iop_a_src = io2_a_src;
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      iop_a_src += self->private_data.s_skip_blocks[0].scratch;
    }

    goto ok;
    ok:
    self->private_impl.p_skip_blocks[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_skip_blocks[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.decode_ae

static wuffs_base__status
wuffs_gif__decoder__decode_ae(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_c = 0;
  uint8_t v_block_size = 0;
  bool v_is_animexts = false;
  bool v_is_netscape = false;
  bool v_is_iccp = false;
  bool v_is_xmp = false;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_ae[0];
  if (coro_susp_point) {
    v_block_size = self->private_data.s_decode_ae[0].v_block_size;
    v_is_animexts = self->private_data.s_decode_ae[0].v_is_animexts;
    v_is_netscape = self->private_data.s_decode_ae[0].v_is_netscape;
    v_is_iccp = self->private_data.s_decode_ae[0].v_is_iccp;
    v_is_xmp = self->private_data.s_decode_ae[0].v_is_xmp;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    while (true) {
      if (self->private_impl.f_metadata_fourcc != 0) {
        status = wuffs_base__make_status(wuffs_base__note__metadata_reported);
        goto ok;
      }
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        uint8_t t_0 = *iop_a_src++;
        v_block_size = t_0;
      }
      if (v_block_size == 0) {
        status = wuffs_base__make_status(NULL);
        goto ok;
      }
      if (v_block_size != 11) {
        self->private_data.s_decode_ae[0].scratch = ((uint32_t)(v_block_size));
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        if (self->private_data.s_decode_ae[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
          self->private_data.s_decode_ae[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
          iop_a_src = io2_a_src;
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        iop_a_src += self->private_data.s_decode_ae[0].scratch;
        goto label__goto_done__break;
      }
      v_is_animexts = true;
      v_is_netscape = true;
      v_is_iccp = true;
      v_is_xmp = true;
      v_block_size = 0;
      while (v_block_size < 11) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint8_t t_1 = *iop_a_src++;
          v_c = t_1;
        }
        v_is_animexts = (v_is_animexts && (v_c == WUFFS_GIF__ANIMEXTS1DOT0[v_block_size]));
        v_is_netscape = (v_is_netscape && (v_c == WUFFS_GIF__NETSCAPE2DOT0[v_block_size]));
        v_is_iccp = (v_is_iccp && (v_c == WUFFS_GIF__ICCRGBG1012[v_block_size]));
        v_is_xmp = (v_is_xmp && (v_c == WUFFS_GIF__XMPDATAXMP[v_block_size]));
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
        v_block_size += 1;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
      }
      if (v_is_animexts || v_is_netscape) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint8_t t_2 = *iop_a_src++;
          v_block_size = t_2;
        }
        if (v_block_size != 3) {
          self->private_data.s_decode_ae[0].scratch = ((uint32_t)(v_block_size));
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(5);
          if (self->private_data.s_decode_ae[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
            self->private_data.s_decode_ae[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
            iop_a_src = io2_a_src;
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          iop_a_src += self->private_data.s_decode_ae[0].scratch;
          goto label__goto_done__break;
        }
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint8_t t_3 = *iop_a_src++;
          v_c = t_3;
        }
        if (v_c != 1) {
          self->private_data.s_decode_ae[0].scratch = 2;
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(7);
          if (self->private_data.s_decode_ae[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
            self->private_data.s_decode_ae[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
            iop_a_src = io2_a_src;
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          iop_a_src += self->private_data.s_decode_ae[0].scratch;
          goto label__goto_done__break;
        }
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(8);
          uint32_t t_4;
          if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
            t_4 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
            iop_a_src += 2;
          } else {
            self->private_data.s_decode_ae[0].scratch = 0;
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(9);
            while (true) {
              if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
                status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                goto suspend;
              }
              uint64_t* scratch = &self->private_data.s_decode_ae[0].scratch;
              uint32_t num_bits_4 = ((uint32_t)(*scratch >> 56));
              *scratch <<= 8;
              *scratch >>= 8;
              *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_4;
              if (num_bits_4 == 8) {
                t_4 = ((uint32_t)(*scratch));
                break;
              }
              num_bits_4 += 8;
              *scratch |= ((uint64_t)(num_bits_4)) << 56;
            }
          }
          self->private_impl.f_num_loops = t_4;
        }
        self->private_impl.f_seen_num_loops = true;
        if ((0 < self->private_impl.f_num_loops) && (self->private_impl.f_num_loops <= 65535)) {
          self->private_impl.f_num_loops += 1;
        }
      } else if (self->private_impl.f_ignore_metadata) {
      } else if (v_is_iccp && self->private_impl.f_report_metadata_iccp) {
        self->private_impl.f_metadata_fourcc = 1229144912;
        self->private_impl.f_metadata_io_position = wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)));
        self->private_impl.f_call_sequence = 1;
        status = wuffs_base__make_status(wuffs_base__note__metadata_reported);
        goto ok;
      } else if (v_is_xmp && self->private_impl.f_report_metadata_xmp) {
        self->private_impl.f_metadata_fourcc = 1481461792;
        self->private_impl.f_metadata_io_position = wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)));
        self->private_impl.f_call_sequence = 1;
        status = wuffs_base__make_status(wuffs_base__note__metadata_reported);
        goto ok;
      }
      goto label__goto_done__break;
    }
    label__goto_done__break:;
    if (a_src) {
      a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
    }
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(10);
    status = wuffs_gif__decoder__skip_blocks(self, a_src);
    if (a_src) {
      iop_a_src = a_src->data.ptr + a_src->meta.ri;
    }
    if (status.repr) {
      goto suspend;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_ae[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_ae[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_data.s_decode_ae[0].v_block_size = v_block_size;
  self->private_data.s_decode_ae[0].v_is_animexts = v_is_animexts;
  self->private_data.s_decode_ae[0].v_is_netscape = v_is_netscape;
  self->private_data.s_decode_ae[0].v_is_iccp = v_is_iccp;
  self->private_data.s_decode_ae[0].v_is_xmp = v_is_xmp;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.decode_gc

static wuffs_base__status
wuffs_gif__decoder__decode_gc(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_c = 0;
  uint8_t v_flags = 0;
  uint16_t v_gc_duration_centiseconds = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_gc[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_0 = *iop_a_src++;
      v_c = t_0;
    }
    if (v_c != 4) {
      status = wuffs_base__make_status(wuffs_gif__error__bad_graphic_control);
      goto exit;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_1 = *iop_a_src++;
      v_flags = t_1;
    }
    self->private_impl.f_gc_has_transparent_index = ((v_flags & 1) != 0);
    v_flags = ((v_flags >> 2) & 7);
    if (v_flags == 2) {
      self->private_impl.f_gc_disposal = 1;
    } else if ((v_flags == 3) || (v_flags == 4)) {
      self->private_impl.f_gc_disposal = 2;
    } else {
      self->private_impl.f_gc_disposal = 0;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
      uint16_t t_2;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_2 = wuffs_base__load_u16le__no_bounds_check(iop_a_src);
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_gc[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_gc[0].scratch;
          uint32_t num_bits_2 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_2;
          if (num_bits_2 == 8) {
            t_2 = ((uint16_t)(*scratch));
            break;
          }
          num_bits_2 += 8;
          *scratch |= ((uint64_t)(num_bits_2)) << 56;
        }
      }
      v_gc_duration_centiseconds = t_2;
    }
    self->private_impl.f_gc_duration = (((uint64_t)(v_gc_duration_centiseconds)) * 7056000);
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(5);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_3 = *iop_a_src++;
      self->private_impl.f_gc_transparent_index = t_3;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_4 = *iop_a_src++;
      v_c = t_4;
    }
    if (v_c != 0) {
      status = wuffs_base__make_status(wuffs_gif__error__bad_graphic_control);
      goto exit;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_gc[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_gc[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.decode_id_part0

static wuffs_base__status
wuffs_gif__decoder__decode_id_part0(
    wuffs_gif__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_id_part0[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      uint32_t t_0;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_0 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_id_part0[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_id_part0[0].scratch;
          uint32_t num_bits_0 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_0;
          if (num_bits_0 == 8) {
            t_0 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_0 += 8;
          *scratch |= ((uint64_t)(num_bits_0)) << 56;
        }
      }
      self->private_impl.f_frame_rect_x0 = t_0;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
      uint32_t t_1;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_1 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_id_part0[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_id_part0[0].scratch;
          uint32_t num_bits_1 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_1;
          if (num_bits_1 == 8) {
            t_1 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_1 += 8;
          *scratch |= ((uint64_t)(num_bits_1)) << 56;
        }
      }
      self->private_impl.f_frame_rect_y0 = t_1;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(5);
      uint32_t t_2;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_2 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_id_part0[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_id_part0[0].scratch;
          uint32_t num_bits_2 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_2;
          if (num_bits_2 == 8) {
            t_2 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_2 += 8;
          *scratch |= ((uint64_t)(num_bits_2)) << 56;
        }
      }
      self->private_impl.f_frame_rect_x1 = t_2;
    }
    self->private_impl.f_frame_rect_x1 += self->private_impl.f_frame_rect_x0;
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(7);
      uint32_t t_3;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
        t_3 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
        iop_a_src += 2;
      } else {
        self->private_data.s_decode_id_part0[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(8);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_decode_id_part0[0].scratch;
          uint32_t num_bits_3 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_3;
          if (num_bits_3 == 8) {
            t_3 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_3 += 8;
          *scratch |= ((uint64_t)(num_bits_3)) << 56;
        }
      }
      self->private_impl.f_frame_rect_y1 = t_3;
    }
    self->private_impl.f_frame_rect_y1 += self->private_impl.f_frame_rect_y0;
    self->private_impl.f_dst_x = self->private_impl.f_frame_rect_x0;
    self->private_impl.f_dst_y = self->private_impl.f_frame_rect_y0;
    if ((self->private_impl.f_call_sequence == 0) &&  ! self->private_impl.f_quirks[4]) {
      self->private_impl.f_width = wuffs_base__u32__max(self->private_impl.f_width, self->private_impl.f_frame_rect_x1);
      self->private_impl.f_height = wuffs_base__u32__max(self->private_impl.f_height, self->private_impl.f_frame_rect_y1);
    }

    goto ok;
    ok:
    self->private_impl.p_decode_id_part0[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_id_part0[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.decode_id_part1

static wuffs_base__status
wuffs_gif__decoder__decode_id_part1(
    wuffs_gif__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_flags = 0;
  uint8_t v_which_palette = 0;
  uint32_t v_num_palette_entries = 0;
  uint32_t v_i = 0;
  uint32_t v_argb = 0;
  wuffs_base__slice_u8 v_dst_palette = {0};
  wuffs_base__status v_status = wuffs_base__make_status(NULL);
  uint8_t v_lw = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_id_part1[0];
  if (coro_susp_point) {
    v_which_palette = self->private_data.s_decode_id_part1[0].v_which_palette;
    v_num_palette_entries = self->private_data.s_decode_id_part1[0].v_num_palette_entries;
    v_i = self->private_data.s_decode_id_part1[0].v_i;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_0 = *iop_a_src++;
      v_flags = t_0;
    }
    if ((v_flags & 64) != 0) {
      self->private_impl.f_interlace = 4;
    } else {
      self->private_impl.f_interlace = 0;
    }
    v_which_palette = 1;
    if ((v_flags & 128) != 0) {
      v_num_palette_entries = (((uint32_t)(1)) << (1 + (v_flags & 7)));
      v_i = 0;
      while (v_i < v_num_palette_entries) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
          uint32_t t_1;
          if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 3)) {
            t_1 = ((uint32_t)(wuffs_base__load_u24be__no_bounds_check(iop_a_src)));
            iop_a_src += 3;
          } else {
            self->private_data.s_decode_id_part1[0].scratch = 0;
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
            while (true) {
              if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
                status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                goto suspend;
              }
              uint64_t* scratch = &self->private_data.s_decode_id_part1[0].scratch;
              uint32_t num_bits_1 = ((uint32_t)(*scratch & 0xFF));
              *scratch >>= 8;
              *scratch <<= 8;
              *scratch |= ((uint64_t)(*iop_a_src++)) << (56 - num_bits_1);
              if (num_bits_1 == 16) {
                t_1 = ((uint32_t)(*scratch >> 40));
                break;
              }
              num_bits_1 += 8;
              *scratch |= ((uint64_t)(num_bits_1));
            }
          }
          v_argb = t_1;
        }
        v_argb |= 4278190080;
        self->private_data.f_palettes[1][((4 * v_i) + 0)] = ((uint8_t)(((v_argb >> 0) & 255)));
        self->private_data.f_palettes[1][((4 * v_i) + 1)] = ((uint8_t)(((v_argb >> 8) & 255)));
        self->private_data.f_palettes[1][((4 * v_i) + 2)] = ((uint8_t)(((v_argb >> 16) & 255)));
        self->private_data.f_palettes[1][((4 * v_i) + 3)] = ((uint8_t)(((v_argb >> 24) & 255)));
        v_i += 1;
      }
      while (v_i < 256) {
        self->private_data.f_palettes[1][((4 * v_i) + 0)] = 0;
        self->private_data.f_palettes[1][((4 * v_i) + 1)] = 0;
        self->private_data.f_palettes[1][((4 * v_i) + 2)] = 0;
        self->private_data.f_palettes[1][((4 * v_i) + 3)] = 255;
        v_i += 1;
      }
    } else if (self->private_impl.f_quirks[6] &&  ! self->private_impl.f_has_global_palette) {
      status = wuffs_base__make_status(wuffs_gif__error__bad_palette);
      goto exit;
    } else if (self->private_impl.f_gc_has_transparent_index) {
      wuffs_base__slice_u8__copy_from_slice(wuffs_base__make_slice_u8(self->private_data.f_palettes[1], 1024), wuffs_base__make_slice_u8(self->private_data.f_palettes[0], 1024));
    } else {
      v_which_palette = 0;
    }
    if (self->private_impl.f_gc_has_transparent_index) {
      self->private_data.f_palettes[1][((4 * ((uint32_t)(self->private_impl.f_gc_transparent_index))) + 0)] = 0;
      self->private_data.f_palettes[1][((4 * ((uint32_t)(self->private_impl.f_gc_transparent_index))) + 1)] = 0;
      self->private_data.f_palettes[1][((4 * ((uint32_t)(self->private_impl.f_gc_transparent_index))) + 2)] = 0;
      self->private_data.f_palettes[1][((4 * ((uint32_t)(self->private_impl.f_gc_transparent_index))) + 3)] = 0;
    }
    v_dst_palette = wuffs_base__pixel_buffer__palette(a_dst);
    if (((uint64_t)(v_dst_palette.len)) == 0) {
      v_dst_palette = wuffs_base__make_slice_u8(self->private_data.f_dst_palette, 1024);
    }
    v_status = wuffs_base__pixel_swizzler__prepare(&self->private_impl.f_swizzler,
        wuffs_base__pixel_buffer__pixel_format(a_dst),
        v_dst_palette,
        wuffs_base__utility__make_pixel_format(2198077448),
        wuffs_base__make_slice_u8(self->private_data.f_palettes[v_which_palette], 1024),
        a_blend);
    if ( ! wuffs_base__status__is_ok(&v_status)) {
      status = v_status;
      if (wuffs_base__status__is_error(&status)) {
        goto exit;
      } else if (wuffs_base__status__is_suspension(&status)) {
        status = wuffs_base__make_status(wuffs_base__error__cannot_return_a_suspension);
        goto exit;
      }
      goto ok;
    }
    if (self->private_impl.f_previous_lzw_decode_ended_abruptly) {
      wuffs_base__ignore_status(wuffs_lzw__decoder__initialize(&self->private_data.f_lzw, sizeof (wuffs_lzw__decoder), WUFFS_VERSION, 0));
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_2 = *iop_a_src++;
      v_lw = t_2;
    }
    if (v_lw > 8) {
      status = wuffs_base__make_status(wuffs_gif__error__bad_literal_width);
      goto exit;
    }
    wuffs_lzw__decoder__set_literal_width(&self->private_data.f_lzw, ((uint32_t)(v_lw)));
    self->private_impl.f_previous_lzw_decode_ended_abruptly = true;

    goto ok;
    ok:
    self->private_impl.p_decode_id_part1[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_id_part1[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_data.s_decode_id_part1[0].v_which_palette = v_which_palette;
  self->private_data.s_decode_id_part1[0].v_num_palette_entries = v_num_palette_entries;
  self->private_data.s_decode_id_part1[0].v_i = v_i;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.decode_id_part2

static wuffs_base__status
wuffs_gif__decoder__decode_id_part2(
    wuffs_gif__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf) {
  wuffs_base__io_buffer empty_io_buffer = wuffs_base__empty_io_buffer();

  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint64_t v_block_size = 0;
  bool v_need_block_size = false;
  uint32_t v_n_copied = 0;
  uint64_t v_n_compressed = 0;
  wuffs_base__io_buffer u_r = wuffs_base__empty_io_buffer();
  wuffs_base__io_buffer* v_r = &u_r;
  const uint8_t* iop_v_r WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io0_v_r WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_v_r WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_v_r WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint64_t v_mark = 0;
  wuffs_base__status v_lzw_status = wuffs_base__make_status(NULL);
  wuffs_base__status v_copy_status = wuffs_base__make_status(NULL);
  wuffs_base__slice_u8 v_uncompressed = {0};

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_id_part2[0];
  if (coro_susp_point) {
    v_block_size = self->private_data.s_decode_id_part2[0].v_block_size;
    v_need_block_size = self->private_data.s_decode_id_part2[0].v_need_block_size;
    v_lzw_status = self->private_data.s_decode_id_part2[0].v_lzw_status;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    v_need_block_size = true;
    label__outer__continue:;
    while (true) {
      if (v_need_block_size) {
        v_need_block_size = false;
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t t_0 = *iop_a_src++;
          v_block_size = t_0;
        }
      }
      if (v_block_size == 0) {
        goto label__outer__break;
      }
      while (((uint64_t)(io2_a_src - iop_a_src)) == 0) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(2);
      }
      if (self->private_impl.f_compressed_ri == self->private_impl.f_compressed_wi) {
        self->private_impl.f_compressed_ri = 0;
        self->private_impl.f_compressed_wi = 0;
      }
      while (self->private_impl.f_compressed_wi <= 3841) {
        v_n_compressed = wuffs_base__u64__min(v_block_size, ((uint64_t)(io2_a_src - iop_a_src)));
        if (v_n_compressed <= 0) {
          goto label__0__break;
        }
        v_n_copied = wuffs_base__io_reader__limited_copy_u32_to_slice(
            &iop_a_src, io2_a_src,((uint32_t)((v_n_compressed & 4294967295))), wuffs_base__slice_u8__subslice_i(wuffs_base__make_slice_u8(self->private_data.f_compressed, 4096), self->private_impl.f_compressed_wi));
        wuffs_base__u64__sat_add_indirect(&self->private_impl.f_compressed_wi, ((uint64_t)(v_n_copied)));
        wuffs_base__u64__sat_sub_indirect(&v_block_size, ((uint64_t)(v_n_copied)));
        if (v_block_size > 0) {
          goto label__0__break;
        }
        if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
          v_need_block_size = true;
          goto label__0__break;
        }
        v_block_size = ((uint64_t)(wuffs_base__load_u8be__no_bounds_check(iop_a_src)));
        (iop_a_src += 1, wuffs_base__make_empty_struct());
      }
      label__0__break:;
      label__inner__continue:;
      while (true) {
        if ((self->private_impl.f_compressed_ri > self->private_impl.f_compressed_wi) || (self->private_impl.f_compressed_wi > 4096)) {
          status = wuffs_base__make_status(wuffs_gif__error__internal_error_inconsistent_ri_wi);
          goto exit;
        }
        {
          wuffs_base__io_buffer* o_0_v_r = v_r;
          const uint8_t *o_0_iop_v_r = iop_v_r;
          const uint8_t *o_0_io0_v_r = io0_v_r;
          const uint8_t *o_0_io1_v_r = io1_v_r;
          const uint8_t *o_0_io2_v_r = io2_v_r;
          v_r = wuffs_base__io_reader__set(
              &u_r,
              &iop_v_r,
              &io0_v_r,
              &io1_v_r,
              &io2_v_r,
              wuffs_base__slice_u8__subslice_ij(wuffs_base__make_slice_u8(self->private_data.f_compressed,
              4096),
              self->private_impl.f_compressed_ri,
              self->private_impl.f_compressed_wi));
          v_mark = ((uint64_t)(iop_v_r - io0_v_r));
          {
            u_r.meta.ri = ((size_t)(iop_v_r - u_r.data.ptr));
            wuffs_base__status t_1 = wuffs_lzw__decoder__transform_io(&self->private_data.f_lzw, &empty_io_buffer, v_r, wuffs_base__utility__empty_slice_u8());
            iop_v_r = u_r.data.ptr + u_r.meta.ri;
            v_lzw_status = t_1;
          }
          wuffs_base__u64__sat_add_indirect(&self->private_impl.f_compressed_ri, wuffs_base__io__count_since(v_mark, ((uint64_t)(iop_v_r - io0_v_r))));
          v_r = o_0_v_r;
          iop_v_r = o_0_iop_v_r;
          io0_v_r = o_0_io0_v_r;
          io1_v_r = o_0_io1_v_r;
          io2_v_r = o_0_io2_v_r;
        }
        v_uncompressed = wuffs_lzw__decoder__flush(&self->private_data.f_lzw);
        if (((uint64_t)(v_uncompressed.len)) > 0) {
          v_copy_status = wuffs_gif__decoder__copy_to_image_buffer(self, a_dst, v_uncompressed);
          if (wuffs_base__status__is_error(&v_copy_status)) {
            status = v_copy_status;
            goto exit;
          }
        }
        if (wuffs_base__status__is_ok(&v_lzw_status)) {
          self->private_impl.f_previous_lzw_decode_ended_abruptly = false;
          if (v_need_block_size || (v_block_size > 0)) {
            self->private_data.s_decode_id_part2[0].scratch = ((uint32_t)(v_block_size));
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
            if (self->private_data.s_decode_id_part2[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
              self->private_data.s_decode_id_part2[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
              iop_a_src = io2_a_src;
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              goto suspend;
            }
            iop_a_src += self->private_data.s_decode_id_part2[0].scratch;
            if (a_src) {
              a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
            }
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
            status = wuffs_gif__decoder__skip_blocks(self, a_src);
            if (a_src) {
              iop_a_src = a_src->data.ptr + a_src->meta.ri;
            }
            if (status.repr) {
              goto suspend;
            }
          }
          goto label__outer__break;
        } else if (v_lzw_status.repr == wuffs_base__suspension__short_read) {
          goto label__outer__continue;
        } else if (v_lzw_status.repr == wuffs_base__suspension__short_write) {
          goto label__inner__continue;
        }
        status = v_lzw_status;
        if (wuffs_base__status__is_error(&status)) {
          goto exit;
        } else if (wuffs_base__status__is_suspension(&status)) {
          status = wuffs_base__make_status(wuffs_base__error__cannot_return_a_suspension);
          goto exit;
        }
        goto ok;
      }
    }
    label__outer__break:;
    self->private_impl.f_compressed_ri = 0;
    self->private_impl.f_compressed_wi = 0;
    if ((self->private_impl.f_dst_y < self->private_impl.f_frame_rect_y1) && (self->private_impl.f_frame_rect_x0 != self->private_impl.f_frame_rect_x1) && (self->private_impl.f_frame_rect_y0 != self->private_impl.f_frame_rect_y1)) {
      status = wuffs_base__make_status(wuffs_base__error__not_enough_data);
      goto exit;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_id_part2[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_id_part2[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_data.s_decode_id_part2[0].v_block_size = v_block_size;
  self->private_data.s_decode_id_part2[0].v_need_block_size = v_need_block_size;
  self->private_data.s_decode_id_part2[0].v_lzw_status = v_lzw_status;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func gif.decoder.copy_to_image_buffer

static wuffs_base__status
wuffs_gif__decoder__copy_to_image_buffer(
    wuffs_gif__decoder* self,
    wuffs_base__pixel_buffer* a_pb,
    wuffs_base__slice_u8 a_src) {
  wuffs_base__slice_u8 v_dst = {0};
  wuffs_base__slice_u8 v_src = {0};
  uint64_t v_width_in_bytes = 0;
  uint64_t v_n = 0;
  uint64_t v_src_ri = 0;
  wuffs_base__pixel_format v_pixfmt = {0};
  uint32_t v_bytes_per_pixel = 0;
  uint32_t v_bits_per_pixel = 0;
  wuffs_base__table_u8 v_tab = {0};
  uint64_t v_i = 0;
  uint64_t v_j = 0;
  uint32_t v_replicate_y0 = 0;
  uint32_t v_replicate_y1 = 0;
  wuffs_base__slice_u8 v_replicate_dst = {0};
  wuffs_base__slice_u8 v_replicate_src = {0};

  v_pixfmt = wuffs_base__pixel_buffer__pixel_format(a_pb);
  v_bits_per_pixel = wuffs_base__pixel_format__bits_per_pixel(&v_pixfmt);
  if ((v_bits_per_pixel & 7) != 0) {
    return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }
  v_bytes_per_pixel = (v_bits_per_pixel >> 3);
  v_width_in_bytes = (((uint64_t)(self->private_impl.f_width)) * ((uint64_t)(v_bytes_per_pixel)));
  v_tab = wuffs_base__pixel_buffer__plane(a_pb, 0);
  label__0__continue:;
  while (v_src_ri < ((uint64_t)(a_src.len))) {
    v_src = wuffs_base__slice_u8__subslice_i(a_src, v_src_ri);
    if (self->private_impl.f_dst_y >= self->private_impl.f_frame_rect_y1) {
      if (self->private_impl.f_quirks[3]) {
        return wuffs_base__make_status(NULL);
      }
      return wuffs_base__make_status(wuffs_base__error__too_much_data);
    }
    v_dst = wuffs_base__table_u8__row(v_tab, self->private_impl.f_dst_y);
    if (self->private_impl.f_dst_y >= self->private_impl.f_height) {
      v_dst = wuffs_base__slice_u8__subslice_j(v_dst, 0);
    } else if (v_width_in_bytes < ((uint64_t)(v_dst.len))) {
      v_dst = wuffs_base__slice_u8__subslice_j(v_dst, v_width_in_bytes);
    }
    v_i = (((uint64_t)(self->private_impl.f_dst_x)) * ((uint64_t)(v_bytes_per_pixel)));
    if (v_i < ((uint64_t)(v_dst.len))) {
      v_j = (((uint64_t)(self->private_impl.f_frame_rect_x1)) * ((uint64_t)(v_bytes_per_pixel)));
      if ((v_i <= v_j) && (v_j <= ((uint64_t)(v_dst.len)))) {
        v_dst = wuffs_base__slice_u8__subslice_ij(v_dst, v_i, v_j);
      } else {
        v_dst = wuffs_base__slice_u8__subslice_i(v_dst, v_i);
      }
      v_n = wuffs_base__pixel_swizzler__swizzle_interleaved_from_slice(&self->private_impl.f_swizzler, v_dst, wuffs_base__make_slice_u8(self->private_data.f_dst_palette, 1024), v_src);
      wuffs_base__u64__sat_add_indirect(&v_src_ri, v_n);
      wuffs_base__u32__sat_add_indirect(&self->private_impl.f_dst_x, ((uint32_t)((v_n & 4294967295))));
      self->private_impl.f_dirty_max_excl_y = wuffs_base__u32__max(self->private_impl.f_dirty_max_excl_y, wuffs_base__u32__sat_add(self->private_impl.f_dst_y, 1));
    }
    if (self->private_impl.f_frame_rect_x1 <= self->private_impl.f_dst_x) {
      self->private_impl.f_dst_x = self->private_impl.f_frame_rect_x0;
      if (self->private_impl.f_interlace == 0) {
        wuffs_base__u32__sat_add_indirect(&self->private_impl.f_dst_y, 1);
        goto label__0__continue;
      }
      if ((self->private_impl.f_num_decoded_frames_value == 0) &&  ! self->private_impl.f_gc_has_transparent_index && (self->private_impl.f_interlace > 1)) {
        v_replicate_src = wuffs_base__table_u8__row(v_tab, self->private_impl.f_dst_y);
        v_replicate_y0 = wuffs_base__u32__sat_add(self->private_impl.f_dst_y, 1);
        v_replicate_y1 = wuffs_base__u32__sat_add(self->private_impl.f_dst_y, ((uint32_t)(WUFFS_GIF__INTERLACE_COUNT[self->private_impl.f_interlace])));
        v_replicate_y1 = wuffs_base__u32__min(v_replicate_y1, self->private_impl.f_frame_rect_y1);
        while (v_replicate_y0 < v_replicate_y1) {
          v_replicate_dst = wuffs_base__table_u8__row(v_tab, v_replicate_y0);
          wuffs_base__slice_u8__copy_from_slice(v_replicate_dst, v_replicate_src);
          v_replicate_y0 += 1;
        }
        self->private_impl.f_dirty_max_excl_y = wuffs_base__u32__max(self->private_impl.f_dirty_max_excl_y, v_replicate_y1);
      }
      wuffs_base__u32__sat_add_indirect(&self->private_impl.f_dst_y, ((uint32_t)(WUFFS_GIF__INTERLACE_DELTA[self->private_impl.f_interlace])));
      while ((self->private_impl.f_interlace > 0) && (self->private_impl.f_dst_y >= self->private_impl.f_frame_rect_y1)) {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
        self->private_impl.f_interlace -= 1;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
        self->private_impl.f_dst_y = wuffs_base__u32__sat_add(self->private_impl.f_frame_rect_y0, WUFFS_GIF__INTERLACE_START[self->private_impl.f_interlace]);
      }
      goto label__0__continue;
    }
    if (((uint64_t)(a_src.len)) == v_src_ri) {
      goto label__0__break;
    } else if (((uint64_t)(a_src.len)) < v_src_ri) {
      return wuffs_base__make_status(wuffs_gif__error__internal_error_inconsistent_ri_wi);
    }
    v_n = ((uint64_t)((self->private_impl.f_frame_rect_x1 - self->private_impl.f_dst_x)));
    v_n = wuffs_base__u64__min(v_n, (((uint64_t)(a_src.len)) - v_src_ri));
    wuffs_base__u64__sat_add_indirect(&v_src_ri, v_n);
    wuffs_base__u32__sat_add_indirect(&self->private_impl.f_dst_x, ((uint32_t)((v_n & 4294967295))));
    if (self->private_impl.f_frame_rect_x1 <= self->private_impl.f_dst_x) {
      self->private_impl.f_dst_x = self->private_impl.f_frame_rect_x0;
      wuffs_base__u32__sat_add_indirect(&self->private_impl.f_dst_y, ((uint32_t)(WUFFS_GIF__INTERLACE_DELTA[self->private_impl.f_interlace])));
      while ((self->private_impl.f_interlace > 0) && (self->private_impl.f_dst_y >= self->private_impl.f_frame_rect_y1)) {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
        self->private_impl.f_interlace -= 1;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
        self->private_impl.f_dst_y = wuffs_base__u32__sat_add(self->private_impl.f_frame_rect_y0, WUFFS_GIF__INTERLACE_START[self->private_impl.f_interlace]);
      }
      goto label__0__continue;
    }
    if (v_src_ri != ((uint64_t)(a_src.len))) {
      return wuffs_base__make_status(wuffs_gif__error__internal_error_inconsistent_ri_wi);
    }
    goto label__0__break;
  }
  label__0__break:;
  return wuffs_base__make_status(NULL);
}

#endif  // !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__GIF)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__GZIP)

// ---------------- Status Codes Implementations

const char* wuffs_gzip__error__bad_checksum = "#gzip: bad checksum";
const char* wuffs_gzip__error__bad_compression_method = "#gzip: bad compression method";
const char* wuffs_gzip__error__bad_encoding_flags = "#gzip: bad encoding flags";
const char* wuffs_gzip__error__bad_header = "#gzip: bad header";

// ---------------- Private Consts

// ---------------- Private Initializer Prototypes

// ---------------- Private Function Prototypes

// ---------------- VTables

const wuffs_base__io_transformer__func_ptrs
wuffs_gzip__decoder__func_ptrs_for__wuffs_base__io_transformer = {
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_gzip__decoder__set_quirk_enabled),
  (wuffs_base__status(*)(void*,
      wuffs_base__io_buffer*,
      wuffs_base__io_buffer*,
      wuffs_base__slice_u8))(&wuffs_gzip__decoder__transform_io),
  (wuffs_base__range_ii_u64(*)(const void*))(&wuffs_gzip__decoder__workbuf_len),
};

// ---------------- Initializer Implementations

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_gzip__decoder__initialize(
    wuffs_gzip__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags){
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (sizeof(*self) != sizeof_star_self) {
    return wuffs_base__make_status(wuffs_base__error__bad_sizeof_receiver);
  }
  if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) ||
      (((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {
    return wuffs_base__make_status(wuffs_base__error__bad_wuffs_version);
  }

  if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {
    // The whole point of this if-check is to detect an uninitialized *self.
    // We disable the warning on GCC. Clang-5.0 does not have this warning.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if (self->private_impl.magic != 0) {
      return wuffs_base__make_status(wuffs_base__error__initialize_falsely_claimed_already_zeroed);
    }
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  } else {
    if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {
      memset(self, 0, sizeof(*self));
      initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;
    } else {
      memset(&(self->private_impl), 0, sizeof(self->private_impl));
    }
  }

  {
    wuffs_base__status z = wuffs_crc32__ieee_hasher__initialize(
        &self->private_data.f_checksum, sizeof(self->private_data.f_checksum), WUFFS_VERSION, initialize_flags);
    if (z.repr) {
      return z;
    }
  }
  {
    wuffs_base__status z = wuffs_deflate__decoder__initialize(
        &self->private_data.f_flate, sizeof(self->private_data.f_flate), WUFFS_VERSION, initialize_flags);
    if (z.repr) {
      return z;
    }
  }
  self->private_impl.magic = WUFFS_BASE__MAGIC;
  self->private_impl.vtable_for__wuffs_base__io_transformer.vtable_name =
      wuffs_base__io_transformer__vtable_name;
  self->private_impl.vtable_for__wuffs_base__io_transformer.function_pointers =
      (const void*)(&wuffs_gzip__decoder__func_ptrs_for__wuffs_base__io_transformer);
  return wuffs_base__make_status(NULL);
}

wuffs_gzip__decoder*
wuffs_gzip__decoder__alloc() {
  wuffs_gzip__decoder* x =
      (wuffs_gzip__decoder*)(calloc(sizeof(wuffs_gzip__decoder), 1));
  if (!x) {
    return NULL;
  }
  if (wuffs_gzip__decoder__initialize(
      x, sizeof(wuffs_gzip__decoder), WUFFS_VERSION, WUFFS_INITIALIZE__ALREADY_ZEROED).repr) {
    free(x);
    return NULL;
  }
  return x;
}

size_t
sizeof__wuffs_gzip__decoder() {
  return sizeof(wuffs_gzip__decoder);
}

// ---------------- Function Implementations

// -------- func gzip.decoder.set_ignore_checksum

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gzip__decoder__set_ignore_checksum(
    wuffs_gzip__decoder* self,
    bool a_ic) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  self->private_impl.f_ignore_checksum = a_ic;
  return wuffs_base__make_empty_struct();
}

// -------- func gzip.decoder.set_quirk_enabled

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_gzip__decoder__set_quirk_enabled(
    wuffs_gzip__decoder* self,
    uint32_t a_quirk,
    bool a_enabled) {
  return wuffs_base__make_empty_struct();
}

// -------- func gzip.decoder.workbuf_len

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_gzip__decoder__workbuf_len(
    const wuffs_gzip__decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  return wuffs_base__utility__make_range_ii_u64(1, 1);
}

// -------- func gzip.decoder.transform_io

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_gzip__decoder__transform_io(
    wuffs_gzip__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 1)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_c = 0;
  uint8_t v_flags = 0;
  uint16_t v_xlen = 0;
  uint64_t v_mark = 0;
  uint32_t v_checksum_got = 0;
  uint32_t v_decoded_length_got = 0;
  wuffs_base__status v_status = wuffs_base__make_status(NULL);
  uint32_t v_checksum_want = 0;
  uint32_t v_decoded_length_want = 0;

  uint8_t* iop_a_dst = NULL;
  uint8_t* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }
  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_transform_io[0];
  if (coro_susp_point) {
    v_flags = self->private_data.s_transform_io[0].v_flags;
    v_checksum_got = self->private_data.s_transform_io[0].v_checksum_got;
    v_decoded_length_got = self->private_data.s_transform_io[0].v_decoded_length_got;
    v_checksum_want = self->private_data.s_transform_io[0].v_checksum_want;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_0 = *iop_a_src++;
      v_c = t_0;
    }
    if (v_c != 31) {
      status = wuffs_base__make_status(wuffs_gzip__error__bad_header);
      goto exit;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_1 = *iop_a_src++;
      v_c = t_1;
    }
    if (v_c != 139) {
      status = wuffs_base__make_status(wuffs_gzip__error__bad_header);
      goto exit;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_2 = *iop_a_src++;
      v_c = t_2;
    }
    if (v_c != 8) {
      status = wuffs_base__make_status(wuffs_gzip__error__bad_compression_method);
      goto exit;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
      if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      uint8_t t_3 = *iop_a_src++;
      v_flags = t_3;
    }
    self->private_data.s_transform_io[0].scratch = 6;
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(5);
    if (self->private_data.s_transform_io[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
      self->private_data.s_transform_io[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
      iop_a_src = io2_a_src;
      status = wuffs_base__make_status(wuffs_base__suspension__short_read);
      goto suspend;
    }
    iop_a_src += self->private_data.s_transform_io[0].scratch;
    if ((v_flags & 4) != 0) {
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
        uint16_t t_4;
        if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
          t_4 = wuffs_base__load_u16le__no_bounds_check(iop_a_src);
          iop_a_src += 2;
        } else {
          self->private_data.s_transform_io[0].scratch = 0;
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(7);
          while (true) {
            if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              goto suspend;
            }
            uint64_t* scratch = &self->private_data.s_transform_io[0].scratch;
            uint32_t num_bits_4 = ((uint32_t)(*scratch >> 56));
            *scratch <<= 8;
            *scratch >>= 8;
            *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_4;
            if (num_bits_4 == 8) {
              t_4 = ((uint16_t)(*scratch));
              break;
            }
            num_bits_4 += 8;
            *scratch |= ((uint64_t)(num_bits_4)) << 56;
          }
        }
        v_xlen = t_4;
      }
      self->private_data.s_transform_io[0].scratch = ((uint32_t)(v_xlen));
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(8);
      if (self->private_data.s_transform_io[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
        self->private_data.s_transform_io[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
        iop_a_src = io2_a_src;
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      iop_a_src += self->private_data.s_transform_io[0].scratch;
    }
    if ((v_flags & 8) != 0) {
      while (true) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(9);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint8_t t_5 = *iop_a_src++;
          v_c = t_5;
        }
        if (v_c == 0) {
          goto label__0__break;
        }
      }
      label__0__break:;
    }
    if ((v_flags & 16) != 0) {
      while (true) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(10);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint8_t t_6 = *iop_a_src++;
          v_c = t_6;
        }
        if (v_c == 0) {
          goto label__1__break;
        }
      }
      label__1__break:;
    }
    if ((v_flags & 2) != 0) {
      self->private_data.s_transform_io[0].scratch = 2;
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(11);
      if (self->private_data.s_transform_io[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
        self->private_data.s_transform_io[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
        iop_a_src = io2_a_src;
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        goto suspend;
      }
      iop_a_src += self->private_data.s_transform_io[0].scratch;
    }
    if ((v_flags & 224) != 0) {
      status = wuffs_base__make_status(wuffs_gzip__error__bad_encoding_flags);
      goto exit;
    }
    while (true) {
      v_mark = ((uint64_t)(iop_a_dst - io0_a_dst));
      {
        if (a_dst) {
          a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
        }
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        wuffs_base__status t_7 = wuffs_deflate__decoder__transform_io(&self->private_data.f_flate, a_dst, a_src, a_workbuf);
        if (a_dst) {
          iop_a_dst = a_dst->data.ptr + a_dst->meta.wi;
        }
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        v_status = t_7;
      }
      if ( ! self->private_impl.f_ignore_checksum) {
        v_checksum_got = wuffs_crc32__ieee_hasher__update_u32(&self->private_data.f_checksum, wuffs_base__io__since(v_mark, ((uint64_t)(iop_a_dst - io0_a_dst)), io0_a_dst));
        v_decoded_length_got += ((uint32_t)((wuffs_base__io__count_since(v_mark, ((uint64_t)(iop_a_dst - io0_a_dst))) & 4294967295)));
      }
      if (wuffs_base__status__is_ok(&v_status)) {
        goto label__2__break;
      }
      status = v_status;
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(12);
    }
    label__2__break:;
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(13);
      uint32_t t_8;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
        t_8 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
        iop_a_src += 4;
      } else {
        self->private_data.s_transform_io[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(14);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_transform_io[0].scratch;
          uint32_t num_bits_8 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_8;
          if (num_bits_8 == 24) {
            t_8 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_8 += 8;
          *scratch |= ((uint64_t)(num_bits_8)) << 56;
        }
      }
      v_checksum_want = t_8;
    }
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(15);
      uint32_t t_9;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
        t_9 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
        iop_a_src += 4;
      } else {
        self->private_data.s_transform_io[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(16);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_transform_io[0].scratch;
          uint32_t num_bits_9 = ((uint32_t)(*scratch >> 56));
          *scratch <<= 8;
          *scratch >>= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << num_bits_9;
          if (num_bits_9 == 24) {
            t_9 = ((uint32_t)(*scratch));
            break;
          }
          num_bits_9 += 8;
          *scratch |= ((uint64_t)(num_bits_9)) << 56;
        }
      }
      v_decoded_length_want = t_9;
    }
    if ( ! self->private_impl.f_ignore_checksum && ((v_checksum_got != v_checksum_want) || (v_decoded_length_got != v_decoded_length_want))) {
      status = wuffs_base__make_status(wuffs_gzip__error__bad_checksum);
      goto exit;
    }

    goto ok;
    ok:
    self->private_impl.p_transform_io[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_transform_io[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 1 : 0;
  self->private_data.s_transform_io[0].v_flags = v_flags;
  self->private_data.s_transform_io[0].v_checksum_got = v_checksum_got;
  self->private_data.s_transform_io[0].v_decoded_length_got = v_decoded_length_got;
  self->private_data.s_transform_io[0].v_checksum_want = v_checksum_want;

  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

#endif  // !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__GZIP)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__JSON)

// ---------------- Status Codes Implementations

const char* wuffs_json__error__bad_c0_control_code = "#json: bad C0 control code";
const char* wuffs_json__error__bad_utf_8 = "#json: bad UTF-8";
const char* wuffs_json__error__bad_backslash_escape = "#json: bad backslash-escape";
const char* wuffs_json__error__bad_input = "#json: bad input";
const char* wuffs_json__error__unsupported_number_length = "#json: unsupported number length";
const char* wuffs_json__error__unsupported_recursion_depth = "#json: unsupported recursion depth";
const char* wuffs_json__error__internal_error_inconsistent_i_o = "#json: internal error: inconsistent I/O";

// ---------------- Private Consts

#define WUFFS_JSON__DECODER_NUMBER_LENGTH_MAX_INCL 99

static const uint8_t
WUFFS_JSON__LUT_BACKSLASHES[256]WUFFS_BASE__POTENTIALLY_UNUSED = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 3, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 162, 0, 0, 0, 0, 5,
  0, 0, 0, 0, 0, 0, 0, 175,
  7, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 4,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 220, 0, 0, 0,
  0, 1, 136, 0, 0, 2, 140, 0,
  0, 0, 0, 0, 0, 0, 138, 0,
  0, 0, 141, 0, 137, 0, 6, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
};

static const uint8_t
WUFFS_JSON__LUT_QUIRKY_BACKSLASHES_QUIRKS[8]WUFFS_BASE__POTENTIALLY_UNUSED = {
  0, 1, 3, 4, 5, 6, 7, 9,
};

static const uint8_t
WUFFS_JSON__LUT_QUIRKY_BACKSLASHES_CHARS[8]WUFFS_BASE__POTENTIALLY_UNUSED = {
  0, 7, 27, 10, 63, 39, 11, 0,
};

static const uint8_t
WUFFS_JSON__LUT_CHARS[256]WUFFS_BASE__POTENTIALLY_UNUSED = {
  128, 129, 130, 131, 132, 133, 134, 135,
  136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 146, 147, 148, 149, 150, 151,
  152, 153, 154, 155, 156, 157, 158, 159,
  0, 0, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 2, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16,
  16, 16, 16, 16, 16, 16, 16, 16,
  32, 32, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3,
  4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4,
  5, 5, 5, 5, 5, 32, 32, 32,
  32, 32, 32, 32, 32, 32, 32, 32,
};

#define WUFFS_JSON__CLASS_WHITESPACE 0

#define WUFFS_JSON__CLASS_STRING 1

#define WUFFS_JSON__CLASS_COMMA 2

#define WUFFS_JSON__CLASS_COLON 3

#define WUFFS_JSON__CLASS_NUMBER 4

#define WUFFS_JSON__CLASS_OPEN_CURLY_BRACE 5

#define WUFFS_JSON__CLASS_CLOSE_CURLY_BRACE 6

#define WUFFS_JSON__CLASS_OPEN_SQUARE_BRACKET 7

#define WUFFS_JSON__CLASS_CLOSE_SQUARE_BRACKET 8

#define WUFFS_JSON__CLASS_FALSE 9

#define WUFFS_JSON__CLASS_TRUE 10

#define WUFFS_JSON__CLASS_NULL_NAN_INF 11

#define WUFFS_JSON__CLASS_COMMENT 12

#define WUFFS_JSON__EXPECT_VALUE 7858

#define WUFFS_JSON__EXPECT_NON_STRING_VALUE 7856

#define WUFFS_JSON__EXPECT_STRING 4098

#define WUFFS_JSON__EXPECT_COMMA 4100

#define WUFFS_JSON__EXPECT_COLON 4104

#define WUFFS_JSON__EXPECT_NUMBER 4112

#define WUFFS_JSON__EXPECT_CLOSE_CURLY_BRACE 4160

#define WUFFS_JSON__EXPECT_CLOSE_SQUARE_BRACKET 4352

static const uint8_t
WUFFS_JSON__LUT_CLASSES[256]WUFFS_BASE__POTENTIALLY_UNUSED = {
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 0, 0, 15, 15, 0, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  0, 15, 1, 15, 15, 15, 15, 15,
  15, 15, 15, 11, 2, 4, 15, 12,
  4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 3, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 11, 15, 15, 15, 15, 11, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 7, 15, 8, 15, 15,
  15, 15, 15, 15, 15, 15, 9, 15,
  15, 11, 15, 15, 15, 15, 11, 15,
  15, 15, 15, 15, 10, 15, 15, 15,
  15, 15, 15, 5, 15, 6, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
  15, 15, 15, 15, 15, 15, 15, 15,
};

static const uint8_t
WUFFS_JSON__LUT_DECIMAL_DIGITS[256]WUFFS_BASE__POTENTIALLY_UNUSED = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  128, 129, 130, 131, 132, 133, 134, 135,
  136, 137, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
};

static const uint8_t
WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[256]WUFFS_BASE__POTENTIALLY_UNUSED = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  128, 129, 130, 131, 132, 133, 134, 135,
  136, 137, 0, 0, 0, 0, 0, 0,
  0, 138, 139, 140, 141, 142, 143, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 138, 139, 140, 141, 142, 143, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
};

#define WUFFS_JSON__QUIRKS_BASE 1225364480

#define WUFFS_JSON__QUIRKS_COUNT 18

// ---------------- Private Initializer Prototypes

// ---------------- Private Function Prototypes

static uint32_t
wuffs_json__decoder__decode_number(
    wuffs_json__decoder* self,
    wuffs_base__io_buffer* a_src);

static uint32_t
wuffs_json__decoder__decode_digits(
    wuffs_json__decoder* self,
    wuffs_base__io_buffer* a_src,
    uint32_t a_n);

static wuffs_base__status
wuffs_json__decoder__decode_leading(
    wuffs_json__decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_json__decoder__decode_comment(
    wuffs_json__decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_json__decoder__decode_inf_nan(
    wuffs_json__decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src);

static wuffs_base__status
wuffs_json__decoder__decode_trailing_new_line(
    wuffs_json__decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src);

// ---------------- VTables

const wuffs_base__token_decoder__func_ptrs
wuffs_json__decoder__func_ptrs_for__wuffs_base__token_decoder = {
  (wuffs_base__status(*)(void*,
      wuffs_base__token_buffer*,
      wuffs_base__io_buffer*,
      wuffs_base__slice_u8))(&wuffs_json__decoder__decode_tokens),
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_json__decoder__set_quirk_enabled),
  (wuffs_base__range_ii_u64(*)(const void*))(&wuffs_json__decoder__workbuf_len),
};

// ---------------- Initializer Implementations

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_json__decoder__initialize(
    wuffs_json__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags){
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (sizeof(*self) != sizeof_star_self) {
    return wuffs_base__make_status(wuffs_base__error__bad_sizeof_receiver);
  }
  if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) ||
      (((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {
    return wuffs_base__make_status(wuffs_base__error__bad_wuffs_version);
  }

  if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {
    // The whole point of this if-check is to detect an uninitialized *self.
    // We disable the warning on GCC. Clang-5.0 does not have this warning.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if (self->private_impl.magic != 0) {
      return wuffs_base__make_status(wuffs_base__error__initialize_falsely_claimed_already_zeroed);
    }
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  } else {
    if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {
      memset(self, 0, sizeof(*self));
      initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;
    } else {
      memset(&(self->private_impl), 0, sizeof(self->private_impl));
    }
  }

  self->private_impl.magic = WUFFS_BASE__MAGIC;
  self->private_impl.vtable_for__wuffs_base__token_decoder.vtable_name =
      wuffs_base__token_decoder__vtable_name;
  self->private_impl.vtable_for__wuffs_base__token_decoder.function_pointers =
      (const void*)(&wuffs_json__decoder__func_ptrs_for__wuffs_base__token_decoder);
  return wuffs_base__make_status(NULL);
}

wuffs_json__decoder*
wuffs_json__decoder__alloc() {
  wuffs_json__decoder* x =
      (wuffs_json__decoder*)(calloc(sizeof(wuffs_json__decoder), 1));
  if (!x) {
    return NULL;
  }
  if (wuffs_json__decoder__initialize(
      x, sizeof(wuffs_json__decoder), WUFFS_VERSION, WUFFS_INITIALIZE__ALREADY_ZEROED).repr) {
    free(x);
    return NULL;
  }
  return x;
}

size_t
sizeof__wuffs_json__decoder() {
  return sizeof(wuffs_json__decoder);
}

// ---------------- Function Implementations

// -------- func json.decoder.set_quirk_enabled

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_json__decoder__set_quirk_enabled(
    wuffs_json__decoder* self,
    uint32_t a_quirk,
    bool a_enabled) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  if (a_quirk >= 1225364480) {
    a_quirk -= 1225364480;
    if (a_quirk < 18) {
      self->private_impl.f_quirks[a_quirk] = a_enabled;
    }
  }
  return wuffs_base__make_empty_struct();
}

// -------- func json.decoder.workbuf_len

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_json__decoder__workbuf_len(
    const wuffs_json__decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  return wuffs_base__utility__empty_range_ii_u64();
}

// -------- func json.decoder.decode_tokens

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_json__decoder__decode_tokens(
    wuffs_json__decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 1)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t v_vminor = 0;
  uint32_t v_number_length = 0;
  uint32_t v_number_status = 0;
  uint32_t v_string_length = 0;
  uint32_t v_whitespace_length = 0;
  uint32_t v_depth = 0;
  uint32_t v_stack_byte = 0;
  uint32_t v_stack_bit = 0;
  uint32_t v_match = 0;
  uint32_t v_c4 = 0;
  uint8_t v_c = 0;
  uint8_t v_backslash = 0;
  uint8_t v_char = 0;
  uint8_t v_class = 0;
  uint32_t v_multi_byte_utf8 = 0;
  uint32_t v_backslash_x_length = 0;
  uint8_t v_backslash_x_ok = 0;
  uint32_t v_backslash_x_string = 0;
  uint8_t v_uni4_ok = 0;
  uint64_t v_uni4_string = 0;
  uint32_t v_uni4_value = 0;
  uint32_t v_uni4_high_surrogate = 0;
  uint8_t v_uni8_ok = 0;
  uint64_t v_uni8_string = 0;
  uint32_t v_uni8_value = 0;
  uint32_t v_expect = 0;
  uint32_t v_expect_after_value = 0;

  wuffs_base__token* iop_a_dst = NULL;
  wuffs_base__token* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  wuffs_base__token* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  wuffs_base__token* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }
  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_tokens[0];
  if (coro_susp_point) {
    v_depth = self->private_data.s_decode_tokens[0].v_depth;
    v_expect = self->private_data.s_decode_tokens[0].v_expect;
    v_expect_after_value = self->private_data.s_decode_tokens[0].v_expect_after_value;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if (self->private_impl.f_end_of_data) {
      status = wuffs_base__make_status(wuffs_base__note__end_of_data);
      goto ok;
    }
    if (self->private_impl.f_quirks[14] || self->private_impl.f_quirks[15]) {
      if (a_dst) {
        a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
      }
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      status = wuffs_json__decoder__decode_leading(self, a_dst, a_src);
      if (a_dst) {
        iop_a_dst = a_dst->data.ptr + a_dst->meta.wi;
      }
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
    }
    v_expect = 7858;
    label__outer__continue:;
    while (true) {
      while (true) {
        if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_write);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(2);
          goto label__outer__continue;
        }
        v_whitespace_length = 0;
        v_c = 0;
        v_class = 0;
        while (true) {
          if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
            if (v_whitespace_length > 0) {
              *iop_a_dst++ = wuffs_base__make_token(
                  (((uint64_t)(0)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                  (((uint64_t)(v_whitespace_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
              v_whitespace_length = 0;
            }
            if (a_src && a_src->meta.closed) {
              status = wuffs_base__make_status(wuffs_json__error__bad_input);
              goto exit;
            }
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(3);
            v_whitespace_length = 0;
            goto label__outer__continue;
          }
          v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
          v_class = WUFFS_JSON__LUT_CLASSES[v_c];
          if (v_class != 0) {
            goto label__ws__break;
          }
          (iop_a_src += 1, wuffs_base__make_empty_struct());
          if (v_whitespace_length >= 65534) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(0)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(65535)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            v_whitespace_length = 0;
            goto label__outer__continue;
          }
          v_whitespace_length += 1;
        }
        label__ws__break:;
        if (v_whitespace_length > 0) {
          *iop_a_dst++ = wuffs_base__make_token(
              (((uint64_t)(0)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
              (((uint64_t)(v_whitespace_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
          v_whitespace_length = 0;
          if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
            goto label__outer__continue;
          }
        }
        if (0 == (v_expect & (((uint32_t)(1)) << v_class))) {
          status = wuffs_base__make_status(wuffs_json__error__bad_input);
          goto exit;
        }
        if (v_class == 1) {
          *iop_a_dst++ = wuffs_base__make_token(
              (((uint64_t)(4194323)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
              (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
              (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
          (iop_a_src += 1, wuffs_base__make_empty_struct());
          label__string_loop_outer__continue:;
          while (true) {
            if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_write);
              WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(4);
              goto label__string_loop_outer__continue;
            }
            v_string_length = 0;
            label__string_loop_inner__continue:;
            while (true) {
              if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
                if (v_string_length > 0) {
                  *iop_a_dst++ = wuffs_base__make_token(
                      (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                      (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                      (((uint64_t)(v_string_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                  v_string_length = 0;
                }
                if (a_src && a_src->meta.closed) {
                  status = wuffs_base__make_status(wuffs_json__error__bad_input);
                  goto exit;
                }
                status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(5);
                v_string_length = 0;
                goto label__string_loop_outer__continue;
              }
              while (((uint64_t)(io2_a_src - iop_a_src)) > 4) {
                v_c4 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
                if (0 != (WUFFS_JSON__LUT_CHARS[(255 & (v_c4 >> 0))] |
                    WUFFS_JSON__LUT_CHARS[(255 & (v_c4 >> 8))] |
                    WUFFS_JSON__LUT_CHARS[(255 & (v_c4 >> 16))] |
                    WUFFS_JSON__LUT_CHARS[(255 & (v_c4 >> 24))])) {
                  goto label__0__break;
                }
                (iop_a_src += 4, wuffs_base__make_empty_struct());
                if (v_string_length > 65527) {
                  *iop_a_dst++ = wuffs_base__make_token(
                      (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                      (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                      (((uint64_t)((v_string_length + 4))) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                  v_string_length = 0;
                  goto label__string_loop_outer__continue;
                }
                v_string_length += 4;
              }
              label__0__break:;
              v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
              v_char = WUFFS_JSON__LUT_CHARS[v_c];
              if (v_char == 0) {
                (iop_a_src += 1, wuffs_base__make_empty_struct());
                if (v_string_length >= 65531) {
                  *iop_a_dst++ = wuffs_base__make_token(
                      (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                      (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                      (((uint64_t)(65532)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                  v_string_length = 0;
                  goto label__string_loop_outer__continue;
                }
                v_string_length += 1;
                goto label__string_loop_inner__continue;
              } else if (v_char == 1) {
                if (v_string_length != 0) {
                  *iop_a_dst++ = wuffs_base__make_token(
                      (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                      (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                      (((uint64_t)(v_string_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                  v_string_length = 0;
                }
                goto label__string_loop_outer__break;
              } else if (v_char == 2) {
                if (v_string_length > 0) {
                  *iop_a_dst++ = wuffs_base__make_token(
                      (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                      (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                      (((uint64_t)(v_string_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                  v_string_length = 0;
                  if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
                    goto label__string_loop_outer__continue;
                  }
                }
                if (((uint64_t)(io2_a_src - iop_a_src)) < 2) {
                  if (a_src && a_src->meta.closed) {
                    status = wuffs_base__make_status(wuffs_json__error__bad_backslash_escape);
                    goto exit;
                  }
                  status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                  WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(6);
                  v_string_length = 0;
                  v_char = 0;
                  goto label__string_loop_outer__continue;
                }
                v_c = ((uint8_t)((wuffs_base__load_u16le__no_bounds_check(iop_a_src) >> 8)));
                v_backslash = WUFFS_JSON__LUT_BACKSLASHES[v_c];
                if ((v_backslash & 128) != 0) {
                  (iop_a_src += 2, wuffs_base__make_empty_struct());
                  *iop_a_dst++ = wuffs_base__make_token(
                      (((uint64_t)((6291456 | ((uint32_t)((v_backslash & 127)))))) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                      (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                      (((uint64_t)(2)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                  goto label__string_loop_outer__continue;
                } else if (v_backslash != 0) {
                  if (self->private_impl.f_quirks[WUFFS_JSON__LUT_QUIRKY_BACKSLASHES_QUIRKS[(v_backslash & 7)]]) {
                    (iop_a_src += 2, wuffs_base__make_empty_struct());
                    *iop_a_dst++ = wuffs_base__make_token(
                        (((uint64_t)((6291456 | ((uint32_t)(WUFFS_JSON__LUT_QUIRKY_BACKSLASHES_CHARS[(v_backslash & 7)]))))) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                        (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                        (((uint64_t)(2)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                    goto label__string_loop_outer__continue;
                  }
                } else if (v_c == 117) {
                  if (((uint64_t)(io2_a_src - iop_a_src)) < 6) {
                    if (a_src && a_src->meta.closed) {
                      status = wuffs_base__make_status(wuffs_json__error__bad_backslash_escape);
                      goto exit;
                    }
                    status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(7);
                    v_string_length = 0;
                    v_char = 0;
                    goto label__string_loop_outer__continue;
                  }
                  v_uni4_string = (((uint64_t)(wuffs_base__load_u48le__no_bounds_check(iop_a_src))) >> 16);
                  v_uni4_value = 0;
                  v_uni4_ok = 128;
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni4_string >> 0))];
                  v_uni4_ok &= v_c;
                  v_uni4_value |= (((uint32_t)((v_c & 15))) << 12);
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni4_string >> 8))];
                  v_uni4_ok &= v_c;
                  v_uni4_value |= (((uint32_t)((v_c & 15))) << 8);
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni4_string >> 16))];
                  v_uni4_ok &= v_c;
                  v_uni4_value |= (((uint32_t)((v_c & 15))) << 4);
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni4_string >> 24))];
                  v_uni4_ok &= v_c;
                  v_uni4_value |= (((uint32_t)((v_c & 15))) << 0);
                  if (v_uni4_ok == 0) {
                  } else if ((v_uni4_value < 55296) || (57343 < v_uni4_value)) {
                    (iop_a_src += 6, wuffs_base__make_empty_struct());
                    *iop_a_dst++ = wuffs_base__make_token(
                        (((uint64_t)((6291456 | v_uni4_value))) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                        (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                        (((uint64_t)(6)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                    goto label__string_loop_outer__continue;
                  } else if (v_uni4_value >= 56320) {
                  } else {
                    if (((uint64_t)(io2_a_src - iop_a_src)) < 12) {
                      if (a_src && a_src->meta.closed) {
                        if (self->private_impl.f_quirks[17]) {
                          (iop_a_src += 6, wuffs_base__make_empty_struct());
                          *iop_a_dst++ = wuffs_base__make_token(
                              (((uint64_t)(6356989)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                              (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                              (((uint64_t)(6)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                          goto label__string_loop_outer__continue;
                        }
                        status = wuffs_base__make_status(wuffs_json__error__bad_backslash_escape);
                        goto exit;
                      }
                      status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                      WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(8);
                      v_string_length = 0;
                      v_uni4_value = 0;
                      v_char = 0;
                      goto label__string_loop_outer__continue;
                    }
                    v_uni4_string = (wuffs_base__load_u64le__no_bounds_check(iop_a_src + 4) >> 16);
                    if (((255 & (v_uni4_string >> 0)) != 92) || ((255 & (v_uni4_string >> 8)) != 117)) {
                      v_uni4_high_surrogate = 0;
                      v_uni4_value = 0;
                      v_uni4_ok = 0;
                    } else {
                      v_uni4_high_surrogate = (65536 + ((v_uni4_value - 55296) << 10));
                      v_uni4_value = 0;
                      v_uni4_ok = 128;
                      v_uni4_string >>= 16;
                      v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni4_string >> 0))];
                      v_uni4_ok &= v_c;
                      v_uni4_value |= (((uint32_t)((v_c & 15))) << 12);
                      v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni4_string >> 8))];
                      v_uni4_ok &= v_c;
                      v_uni4_value |= (((uint32_t)((v_c & 15))) << 8);
                      v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni4_string >> 16))];
                      v_uni4_ok &= v_c;
                      v_uni4_value |= (((uint32_t)((v_c & 15))) << 4);
                      v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni4_string >> 24))];
                      v_uni4_ok &= v_c;
                      v_uni4_value |= (((uint32_t)((v_c & 15))) << 0);
                    }
                    if ((v_uni4_ok != 0) && (56320 <= v_uni4_value) && (v_uni4_value <= 57343)) {
                      v_uni4_value -= 56320;
                      (iop_a_src += 12, wuffs_base__make_empty_struct());
                      *iop_a_dst++ = wuffs_base__make_token(
                          (((uint64_t)((6291456 | v_uni4_high_surrogate | v_uni4_value))) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                          (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                          (((uint64_t)(12)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                      goto label__string_loop_outer__continue;
                    }
                  }
                  if (self->private_impl.f_quirks[17]) {
                    if (((uint64_t)(io2_a_src - iop_a_src)) < 6) {
                      status = wuffs_base__make_status(wuffs_json__error__internal_error_inconsistent_i_o);
                      goto exit;
                    }
                    (iop_a_src += 6, wuffs_base__make_empty_struct());
                    *iop_a_dst++ = wuffs_base__make_token(
                        (((uint64_t)(6356989)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                        (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                        (((uint64_t)(6)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                    goto label__string_loop_outer__continue;
                  }
                } else if ((v_c == 85) && self->private_impl.f_quirks[2]) {
                  if (((uint64_t)(io2_a_src - iop_a_src)) < 10) {
                    if (a_src && a_src->meta.closed) {
                      status = wuffs_base__make_status(wuffs_json__error__bad_backslash_escape);
                      goto exit;
                    }
                    status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(9);
                    v_string_length = 0;
                    v_char = 0;
                    goto label__string_loop_outer__continue;
                  }
                  v_uni8_string = wuffs_base__load_u64le__no_bounds_check(iop_a_src + 2);
                  v_uni8_value = 0;
                  v_uni8_ok = 128;
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni8_string >> 0))];
                  v_uni8_ok &= v_c;
                  v_uni8_value |= (((uint32_t)((v_c & 15))) << 28);
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni8_string >> 8))];
                  v_uni8_ok &= v_c;
                  v_uni8_value |= (((uint32_t)((v_c & 15))) << 24);
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni8_string >> 16))];
                  v_uni8_ok &= v_c;
                  v_uni8_value |= (((uint32_t)((v_c & 15))) << 20);
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni8_string >> 24))];
                  v_uni8_ok &= v_c;
                  v_uni8_value |= (((uint32_t)((v_c & 15))) << 16);
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni8_string >> 32))];
                  v_uni8_ok &= v_c;
                  v_uni8_value |= (((uint32_t)((v_c & 15))) << 12);
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni8_string >> 40))];
                  v_uni8_ok &= v_c;
                  v_uni8_value |= (((uint32_t)((v_c & 15))) << 8);
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni8_string >> 48))];
                  v_uni8_ok &= v_c;
                  v_uni8_value |= (((uint32_t)((v_c & 15))) << 4);
                  v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_uni8_string >> 56))];
                  v_uni8_ok &= v_c;
                  v_uni8_value |= (((uint32_t)((v_c & 15))) << 0);
                  if (v_uni8_ok == 0) {
                  } else if ((v_uni8_value < 55296) || ((57343 < v_uni8_value) && (v_uni8_value <= 1114111))) {
                    (iop_a_src += 10, wuffs_base__make_empty_struct());
                    *iop_a_dst++ = wuffs_base__make_token(
                        (((uint64_t)((6291456 | (v_uni8_value & 2097151)))) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                        (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                        (((uint64_t)(10)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                    goto label__string_loop_outer__continue;
                  } else if (self->private_impl.f_quirks[17]) {
                    (iop_a_src += 10, wuffs_base__make_empty_struct());
                    *iop_a_dst++ = wuffs_base__make_token(
                        (((uint64_t)(6356989)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                        (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                        (((uint64_t)(10)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                    goto label__string_loop_outer__continue;
                  }
                } else if ((v_c == 120) && self->private_impl.f_quirks[8]) {
                  if (((uint64_t)(io2_a_src - iop_a_src)) < 4) {
                    if (a_src && a_src->meta.closed) {
                      status = wuffs_base__make_status(wuffs_json__error__bad_backslash_escape);
                      goto exit;
                    }
                    status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(10);
                    v_string_length = 0;
                    v_char = 0;
                    goto label__string_loop_outer__continue;
                  }
                  v_backslash_x_length = 0;
                  while ((v_backslash_x_length <= 65531) && (((uint64_t)(io2_a_src - iop_a_src)) >= 4)) {
                    v_backslash_x_string = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
                    v_backslash_x_ok = 128;
                    v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_backslash_x_string >> 16))];
                    v_backslash_x_ok &= v_c;
                    v_c = WUFFS_JSON__LUT_HEXADECIMAL_DIGITS[(255 & (v_backslash_x_string >> 24))];
                    v_backslash_x_ok &= v_c;
                    if ((v_backslash_x_ok == 0) || ((v_backslash_x_string & 65535) != 30812)) {
                      goto label__1__break;
                    }
                    (iop_a_src += 4, wuffs_base__make_empty_struct());
                    v_backslash_x_length += 4;
                  }
                  label__1__break:;
                  if (v_backslash_x_length == 0) {
                    status = wuffs_base__make_status(wuffs_json__error__bad_backslash_escape);
                    goto exit;
                  }
                  *iop_a_dst++ = wuffs_base__make_token(
                      (((uint64_t)(4194432)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                      (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                      (((uint64_t)(v_backslash_x_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                  goto label__string_loop_outer__continue;
                }
                status = wuffs_base__make_status(wuffs_json__error__bad_backslash_escape);
                goto exit;
              } else if (v_char == 3) {
                if (((uint64_t)(io2_a_src - iop_a_src)) < 2) {
                  if (v_string_length > 0) {
                    *iop_a_dst++ = wuffs_base__make_token(
                        (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                        (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                        (((uint64_t)(v_string_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                    v_string_length = 0;
                    if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
                      goto label__string_loop_outer__continue;
                    }
                  }
                  if (a_src && a_src->meta.closed) {
                    if (self->private_impl.f_quirks[17]) {
                      *iop_a_dst++ = wuffs_base__make_token(
                          (((uint64_t)(6356989)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                          (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                          (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                      (iop_a_src += 1, wuffs_base__make_empty_struct());
                      goto label__string_loop_outer__continue;
                    }
                    status = wuffs_base__make_status(wuffs_json__error__bad_utf_8);
                    goto exit;
                  }
                  status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                  WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(11);
                  v_string_length = 0;
                  v_char = 0;
                  goto label__string_loop_outer__continue;
                }
                v_multi_byte_utf8 = ((uint32_t)(wuffs_base__load_u16le__no_bounds_check(iop_a_src)));
                if ((v_multi_byte_utf8 & 49152) == 32768) {
                  v_multi_byte_utf8 = ((1984 & (v_multi_byte_utf8 << 6)) | (63 & (v_multi_byte_utf8 >> 8)));
                  (iop_a_src += 2, wuffs_base__make_empty_struct());
                  if (v_string_length >= 65528) {
                    *iop_a_dst++ = wuffs_base__make_token(
                        (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                        (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                        (((uint64_t)((v_string_length + 2))) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                    v_string_length = 0;
                    goto label__string_loop_outer__continue;
                  }
                  v_string_length += 2;
                  goto label__string_loop_inner__continue;
                }
              } else if (v_char == 4) {
                if (((uint64_t)(io2_a_src - iop_a_src)) < 3) {
                  if (v_string_length > 0) {
                    *iop_a_dst++ = wuffs_base__make_token(
                        (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                        (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                        (((uint64_t)(v_string_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                    v_string_length = 0;
                    if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
                      goto label__string_loop_outer__continue;
                    }
                  }
                  if (a_src && a_src->meta.closed) {
                    if (self->private_impl.f_quirks[17]) {
                      *iop_a_dst++ = wuffs_base__make_token(
                          (((uint64_t)(6356989)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                          (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                          (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                      (iop_a_src += 1, wuffs_base__make_empty_struct());
                      goto label__string_loop_outer__continue;
                    }
                    status = wuffs_base__make_status(wuffs_json__error__bad_utf_8);
                    goto exit;
                  }
                  status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                  WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(12);
                  v_string_length = 0;
                  v_char = 0;
                  goto label__string_loop_outer__continue;
                }
                v_multi_byte_utf8 = ((uint32_t)(wuffs_base__load_u24le__no_bounds_check(iop_a_src)));
                if ((v_multi_byte_utf8 & 12632064) == 8421376) {
                  v_multi_byte_utf8 = ((61440 & (v_multi_byte_utf8 << 12)) | (4032 & (v_multi_byte_utf8 >> 2)) | (63 & (v_multi_byte_utf8 >> 16)));
                  if ((2047 < v_multi_byte_utf8) && ((v_multi_byte_utf8 < 55296) || (57343 < v_multi_byte_utf8))) {
                    (iop_a_src += 3, wuffs_base__make_empty_struct());
                    if (v_string_length >= 65528) {
                      *iop_a_dst++ = wuffs_base__make_token(
                          (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                          (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                          (((uint64_t)((v_string_length + 3))) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                      v_string_length = 0;
                      goto label__string_loop_outer__continue;
                    }
                    v_string_length += 3;
                    goto label__string_loop_inner__continue;
                  }
                }
              } else if (v_char == 5) {
                if (((uint64_t)(io2_a_src - iop_a_src)) < 4) {
                  if (v_string_length > 0) {
                    *iop_a_dst++ = wuffs_base__make_token(
                        (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                        (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                        (((uint64_t)(v_string_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                    v_string_length = 0;
                    if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
                      goto label__string_loop_outer__continue;
                    }
                  }
                  if (a_src && a_src->meta.closed) {
                    if (self->private_impl.f_quirks[17]) {
                      *iop_a_dst++ = wuffs_base__make_token(
                          (((uint64_t)(6356989)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                          (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                          (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                      (iop_a_src += 1, wuffs_base__make_empty_struct());
                      goto label__string_loop_outer__continue;
                    }
                    status = wuffs_base__make_status(wuffs_json__error__bad_utf_8);
                    goto exit;
                  }
                  status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                  WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(13);
                  v_string_length = 0;
                  v_char = 0;
                  goto label__string_loop_outer__continue;
                }
                v_multi_byte_utf8 = wuffs_base__load_u32le__no_bounds_check(iop_a_src);
                if ((v_multi_byte_utf8 & 3233857536) == 2155905024) {
                  v_multi_byte_utf8 = ((1835008 & (v_multi_byte_utf8 << 18)) |
                      (258048 & (v_multi_byte_utf8 << 4)) |
                      (4032 & (v_multi_byte_utf8 >> 10)) |
                      (63 & (v_multi_byte_utf8 >> 24)));
                  if ((65535 < v_multi_byte_utf8) && (v_multi_byte_utf8 <= 1114111)) {
                    (iop_a_src += 4, wuffs_base__make_empty_struct());
                    if (v_string_length >= 65528) {
                      *iop_a_dst++ = wuffs_base__make_token(
                          (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                          (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                          (((uint64_t)((v_string_length + 4))) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                      v_string_length = 0;
                      goto label__string_loop_outer__continue;
                    }
                    v_string_length += 4;
                    goto label__string_loop_inner__continue;
                  }
                }
              }
              if (v_string_length > 0) {
                *iop_a_dst++ = wuffs_base__make_token(
                    (((uint64_t)(4194337)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                    (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                    (((uint64_t)(v_string_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                v_string_length = 0;
                if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
                  goto label__string_loop_outer__continue;
                }
              }
              if ((v_char & 128) != 0) {
                if (self->private_impl.f_quirks[0]) {
                  *iop_a_dst++ = wuffs_base__make_token(
                      (((uint64_t)((6291456 | ((uint32_t)((v_char & 127)))))) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                      (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                      (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                  (iop_a_src += 1, wuffs_base__make_empty_struct());
                  goto label__string_loop_outer__continue;
                }
                status = wuffs_base__make_status(wuffs_json__error__bad_c0_control_code);
                goto exit;
              }
              if (self->private_impl.f_quirks[17]) {
                *iop_a_dst++ = wuffs_base__make_token(
                    (((uint64_t)(6356989)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                    (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                    (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
                (iop_a_src += 1, wuffs_base__make_empty_struct());
                goto label__string_loop_outer__continue;
              }
              status = wuffs_base__make_status(wuffs_json__error__bad_utf_8);
              goto exit;
            }
          }
          label__string_loop_outer__break:;
          label__2__continue:;
          while (true) {
            if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
              if (a_src && a_src->meta.closed) {
                status = wuffs_base__make_status(wuffs_json__error__bad_input);
                goto exit;
              }
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(14);
              goto label__2__continue;
            }
            if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_write);
              WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(15);
              goto label__2__continue;
            }
            (iop_a_src += 1, wuffs_base__make_empty_struct());
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(4194323)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            goto label__2__break;
          }
          label__2__break:;
          if (0 == (v_expect & (((uint32_t)(1)) << 4))) {
            v_expect = 4104;
            goto label__outer__continue;
          }
          goto label__goto_parsed_a_leaf_value__break;
        } else if (v_class == 2) {
          (iop_a_src += 1, wuffs_base__make_empty_struct());
          *iop_a_dst++ = wuffs_base__make_token(
              (((uint64_t)(0)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
              (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
          if (0 == (v_expect & (((uint32_t)(1)) << 8))) {
            if (self->private_impl.f_quirks[12]) {
              v_expect = 4162;
            } else {
              v_expect = 4098;
            }
          } else {
            if (self->private_impl.f_quirks[12]) {
              v_expect = 8114;
            } else {
              v_expect = 7858;
            }
          }
          goto label__outer__continue;
        } else if (v_class == 3) {
          (iop_a_src += 1, wuffs_base__make_empty_struct());
          *iop_a_dst++ = wuffs_base__make_token(
              (((uint64_t)(0)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
              (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
          v_expect = 7858;
          goto label__outer__continue;
        } else if (v_class == 4) {
          while (true) {
            if (a_src) {
              a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
            }
            v_number_length = wuffs_json__decoder__decode_number(self, a_src);
            if (a_src) {
              iop_a_src = a_src->data.ptr + a_src->meta.ri;
            }
            v_number_status = (v_number_length >> 8);
            v_vminor = 10486787;
            if ((v_number_length & 128) != 0) {
              v_vminor = 10486785;
            }
            v_number_length = (v_number_length & 127);
            if (v_number_status == 0) {
              *iop_a_dst++ = wuffs_base__make_token(
                  (((uint64_t)(v_vminor)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                  (((uint64_t)(v_number_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
              goto label__3__break;
            }
            while (v_number_length > 0) {
              v_number_length -= 1;
              if (iop_a_src > io1_a_src) {
                (iop_a_src--, wuffs_base__make_empty_struct());
              } else {
                status = wuffs_base__make_status(wuffs_json__error__internal_error_inconsistent_i_o);
                goto exit;
              }
            }
            if (v_number_status == 1) {
              if (self->private_impl.f_quirks[13]) {
                if (a_dst) {
                  a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
                }
                if (a_src) {
                  a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
                }
                WUFFS_BASE__COROUTINE_SUSPENSION_POINT(16);
                status = wuffs_json__decoder__decode_inf_nan(self, a_dst, a_src);
                if (a_dst) {
                  iop_a_dst = a_dst->data.ptr + a_dst->meta.wi;
                }
                if (a_src) {
                  iop_a_src = a_src->data.ptr + a_src->meta.ri;
                }
                if (status.repr) {
                  goto suspend;
                }
                goto label__3__break;
              }
              status = wuffs_base__make_status(wuffs_json__error__bad_input);
              goto exit;
            } else if (v_number_status == 2) {
              status = wuffs_base__make_status(wuffs_json__error__unsupported_number_length);
              goto exit;
            } else {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(17);
              while (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
                status = wuffs_base__make_status(wuffs_base__suspension__short_write);
                WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(18);
              }
            }
          }
          label__3__break:;
          goto label__goto_parsed_a_leaf_value__break;
        } else if (v_class == 5) {
          v_vminor = 2113553;
          if (v_depth == 0) {
          } else if (0 != (v_expect_after_value & (((uint32_t)(1)) << 6))) {
            v_vminor = 2113601;
          } else {
            v_vminor = 2113569;
          }
          if (v_depth >= 1024) {
            status = wuffs_base__make_status(wuffs_json__error__unsupported_recursion_depth);
            goto exit;
          }
          v_stack_byte = (v_depth / 32);
          v_stack_bit = (v_depth & 31);
          self->private_data.f_stack[v_stack_byte] |= (((uint32_t)(1)) << v_stack_bit);
          v_depth += 1;
          (iop_a_src += 1, wuffs_base__make_empty_struct());
          *iop_a_dst++ = wuffs_base__make_token(
              (((uint64_t)(v_vminor)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
              (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
          v_expect = 4162;
          v_expect_after_value = 4164;
          goto label__outer__continue;
        } else if (v_class == 6) {
          (iop_a_src += 1, wuffs_base__make_empty_struct());
          if (v_depth <= 1) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(2101314)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            goto label__outer__break;
          }
          v_depth -= 1;
          v_stack_byte = ((v_depth - 1) / 32);
          v_stack_bit = ((v_depth - 1) & 31);
          if (0 == (self->private_data.f_stack[v_stack_byte] & (((uint32_t)(1)) << v_stack_bit))) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(2105410)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            v_expect = 4356;
            v_expect_after_value = 4356;
          } else {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(2113602)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            v_expect = 4164;
            v_expect_after_value = 4164;
          }
          goto label__outer__continue;
        } else if (v_class == 7) {
          v_vminor = 2105361;
          if (v_depth == 0) {
          } else if (0 != (v_expect_after_value & (((uint32_t)(1)) << 6))) {
            v_vminor = 2105409;
          } else {
            v_vminor = 2105377;
          }
          if (v_depth >= 1024) {
            status = wuffs_base__make_status(wuffs_json__error__unsupported_recursion_depth);
            goto exit;
          }
          v_stack_byte = (v_depth / 32);
          v_stack_bit = (v_depth & 31);
          self->private_data.f_stack[v_stack_byte] &= (4294967295 ^ (((uint32_t)(1)) << v_stack_bit));
          v_depth += 1;
          (iop_a_src += 1, wuffs_base__make_empty_struct());
          *iop_a_dst++ = wuffs_base__make_token(
              (((uint64_t)(v_vminor)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
              (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
          v_expect = 8114;
          v_expect_after_value = 4356;
          goto label__outer__continue;
        } else if (v_class == 8) {
          (iop_a_src += 1, wuffs_base__make_empty_struct());
          if (v_depth <= 1) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(2101282)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            goto label__outer__break;
          }
          v_depth -= 1;
          v_stack_byte = ((v_depth - 1) / 32);
          v_stack_bit = ((v_depth - 1) & 31);
          if (0 == (self->private_data.f_stack[v_stack_byte] & (((uint32_t)(1)) << v_stack_bit))) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(2105378)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            v_expect = 4356;
            v_expect_after_value = 4356;
          } else {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(2113570)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            v_expect = 4164;
            v_expect_after_value = 4164;
          }
          goto label__outer__continue;
        } else if (v_class == 9) {
          v_match = wuffs_base__io_reader__match7(iop_a_src, io2_a_src, a_src,111546413966853);
          if (v_match == 0) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(8388612)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(5)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            if (((uint64_t)(io2_a_src - iop_a_src)) < 5) {
              status = wuffs_base__make_status(wuffs_json__error__internal_error_inconsistent_i_o);
              goto exit;
            }
            (iop_a_src += 5, wuffs_base__make_empty_struct());
            goto label__goto_parsed_a_leaf_value__break;
          } else if (v_match == 1) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(19);
            goto label__outer__continue;
          }
        } else if (v_class == 10) {
          v_match = wuffs_base__io_reader__match7(iop_a_src, io2_a_src, a_src,435762131972);
          if (v_match == 0) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(8388616)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(4)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            if (((uint64_t)(io2_a_src - iop_a_src)) < 4) {
              status = wuffs_base__make_status(wuffs_json__error__internal_error_inconsistent_i_o);
              goto exit;
            }
            (iop_a_src += 4, wuffs_base__make_empty_struct());
            goto label__goto_parsed_a_leaf_value__break;
          } else if (v_match == 1) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(20);
            goto label__outer__continue;
          }
        } else if (v_class == 11) {
          v_match = wuffs_base__io_reader__match7(iop_a_src, io2_a_src, a_src,465676103172);
          if (v_match == 0) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(8388610)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(4)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            if (((uint64_t)(io2_a_src - iop_a_src)) < 4) {
              status = wuffs_base__make_status(wuffs_json__error__internal_error_inconsistent_i_o);
              goto exit;
            }
            (iop_a_src += 4, wuffs_base__make_empty_struct());
            goto label__goto_parsed_a_leaf_value__break;
          } else if (v_match == 1) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(21);
            goto label__outer__continue;
          }
          if (self->private_impl.f_quirks[13]) {
            if (a_dst) {
              a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
            }
            if (a_src) {
              a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
            }
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(22);
            status = wuffs_json__decoder__decode_inf_nan(self, a_dst, a_src);
            if (a_dst) {
              iop_a_dst = a_dst->data.ptr + a_dst->meta.wi;
            }
            if (a_src) {
              iop_a_src = a_src->data.ptr + a_src->meta.ri;
            }
            if (status.repr) {
              goto suspend;
            }
            goto label__goto_parsed_a_leaf_value__break;
          }
        } else if (v_class == 12) {
          if (self->private_impl.f_quirks[10] || self->private_impl.f_quirks[11]) {
            if (a_dst) {
              a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
            }
            if (a_src) {
              a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
            }
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(23);
            status = wuffs_json__decoder__decode_comment(self, a_dst, a_src);
            if (a_dst) {
              iop_a_dst = a_dst->data.ptr + a_dst->meta.wi;
            }
            if (a_src) {
              iop_a_src = a_src->data.ptr + a_src->meta.ri;
            }
            if (status.repr) {
              goto suspend;
            }
            goto label__outer__continue;
          }
        }
        status = wuffs_base__make_status(wuffs_json__error__bad_input);
        goto exit;
      }
      label__goto_parsed_a_leaf_value__break:;
      if (v_depth == 0) {
        goto label__outer__break;
      }
      v_expect = v_expect_after_value;
    }
    label__outer__break:;
    if (self->private_impl.f_quirks[16]) {
      if (a_dst) {
        a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
      }
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(24);
      status = wuffs_json__decoder__decode_trailing_new_line(self, a_dst, a_src);
      if (a_dst) {
        iop_a_dst = a_dst->data.ptr + a_dst->meta.wi;
      }
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
    }
    self->private_impl.f_end_of_data = true;

    goto ok;
    ok:
    self->private_impl.p_decode_tokens[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_tokens[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 1 : 0;
  self->private_data.s_decode_tokens[0].v_depth = v_depth;
  self->private_data.s_decode_tokens[0].v_expect = v_expect;
  self->private_data.s_decode_tokens[0].v_expect_after_value = v_expect_after_value;

  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func json.decoder.decode_number

static uint32_t
wuffs_json__decoder__decode_number(
    wuffs_json__decoder* self,
    wuffs_base__io_buffer* a_src) {
  uint8_t v_c = 0;
  uint32_t v_n = 0;
  uint32_t v_floating_point = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  while (true) {
    v_n = 0;
    if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
      if ( ! (a_src && a_src->meta.closed)) {
        v_n |= 768;
      }
      goto label__goto_done__break;
    }
    v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
    if (v_c != 45) {
    } else {
      v_n += 1;
      (iop_a_src += 1, wuffs_base__make_empty_struct());
      if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
        if ( ! (a_src && a_src->meta.closed)) {
          v_n |= 768;
        }
        v_n |= 256;
        goto label__goto_done__break;
      }
      v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
    }
    if (v_c == 48) {
      v_n += 1;
      (iop_a_src += 1, wuffs_base__make_empty_struct());
    } else {
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      v_n = wuffs_json__decoder__decode_digits(self, a_src, v_n);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (v_n > 99) {
        goto label__goto_done__break;
      }
    }
    if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
      if ( ! (a_src && a_src->meta.closed)) {
        v_n |= 768;
      }
      goto label__goto_done__break;
    }
    v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
    if (v_c != 46) {
    } else {
      if (v_n >= 99) {
        v_n |= 512;
        goto label__goto_done__break;
      }
      v_n += 1;
      (iop_a_src += 1, wuffs_base__make_empty_struct());
      v_floating_point = 128;
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      v_n = wuffs_json__decoder__decode_digits(self, a_src, v_n);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (v_n > 99) {
        goto label__goto_done__break;
      }
      if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
        if ( ! (a_src && a_src->meta.closed)) {
          v_n |= 768;
        }
        goto label__goto_done__break;
      }
      v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
    }
    if ((v_c != 69) && (v_c != 101)) {
      goto label__goto_done__break;
    }
    if (v_n >= 99) {
      v_n |= 512;
      goto label__goto_done__break;
    }
    v_n += 1;
    (iop_a_src += 1, wuffs_base__make_empty_struct());
    v_floating_point = 128;
    if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
      if ( ! (a_src && a_src->meta.closed)) {
        v_n |= 768;
      }
      v_n |= 256;
      goto label__goto_done__break;
    }
    v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
    if ((v_c != 43) && (v_c != 45)) {
    } else {
      if (v_n >= 99) {
        v_n |= 512;
        goto label__goto_done__break;
      }
      v_n += 1;
      (iop_a_src += 1, wuffs_base__make_empty_struct());
    }
    if (a_src) {
      a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
    }
    v_n = wuffs_json__decoder__decode_digits(self, a_src, v_n);
    if (a_src) {
      iop_a_src = a_src->data.ptr + a_src->meta.ri;
    }
    goto label__goto_done__break;
  }
  label__goto_done__break:;
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }
  return (v_n | v_floating_point);
}

// -------- func json.decoder.decode_digits

static uint32_t
wuffs_json__decoder__decode_digits(
    wuffs_json__decoder* self,
    wuffs_base__io_buffer* a_src,
    uint32_t a_n) {
  uint8_t v_c = 0;
  uint32_t v_n = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  v_n = a_n;
  while (true) {
    if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
      if ( ! (a_src && a_src->meta.closed)) {
        v_n |= 768;
      }
      goto label__0__break;
    }
    v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
    if (0 == WUFFS_JSON__LUT_DECIMAL_DIGITS[v_c]) {
      goto label__0__break;
    }
    if (v_n >= 99) {
      v_n |= 512;
      goto label__0__break;
    }
    v_n += 1;
    (iop_a_src += 1, wuffs_base__make_empty_struct());
  }
  label__0__break:;
  if (v_n == a_n) {
    v_n |= 256;
  }
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }
  return v_n;
}

// -------- func json.decoder.decode_leading

static wuffs_base__status
wuffs_json__decoder__decode_leading(
    wuffs_json__decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_c = 0;
  uint32_t v_u = 0;

  wuffs_base__token* iop_a_dst = NULL;
  wuffs_base__token* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  wuffs_base__token* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  wuffs_base__token* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }
  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_leading[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    self->private_impl.f_allow_leading_ars = self->private_impl.f_quirks[14];
    self->private_impl.f_allow_leading_ubom = self->private_impl.f_quirks[15];
    label__0__continue:;
    while (self->private_impl.f_allow_leading_ars || self->private_impl.f_allow_leading_ubom) {
      if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_write);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(1);
        goto label__0__continue;
      }
      if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
        if (a_src && a_src->meta.closed) {
          goto label__0__break;
        }
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(2);
        goto label__0__continue;
      }
      v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
      if ((v_c == 30) && self->private_impl.f_allow_leading_ars) {
        self->private_impl.f_allow_leading_ars = false;
        (iop_a_src += 1, wuffs_base__make_empty_struct());
        *iop_a_dst++ = wuffs_base__make_token(
            (((uint64_t)(0)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
            (((uint64_t)(1)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
        goto label__0__continue;
      } else if ((v_c == 239) && self->private_impl.f_allow_leading_ubom) {
        if (((uint64_t)(io2_a_src - iop_a_src)) < 3) {
          if (a_src && a_src->meta.closed) {
            goto label__0__break;
          }
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(3);
          goto label__0__continue;
        }
        v_u = ((uint32_t)(wuffs_base__load_u24le__no_bounds_check(iop_a_src)));
        if (v_u == 12565487) {
          self->private_impl.f_allow_leading_ubom = false;
          (iop_a_src += 3, wuffs_base__make_empty_struct());
          *iop_a_dst++ = wuffs_base__make_token(
              (((uint64_t)(0)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
              (((uint64_t)(3)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
          goto label__0__continue;
        }
      }
      goto label__0__break;
    }
    label__0__break:;

    goto ok;
    ok:
    self->private_impl.p_decode_leading[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_leading[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func json.decoder.decode_comment

static wuffs_base__status
wuffs_json__decoder__decode_comment(
    wuffs_json__decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_c = 0;
  uint16_t v_c2 = 0;
  uint32_t v_length = 0;

  wuffs_base__token* iop_a_dst = NULL;
  wuffs_base__token* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  wuffs_base__token* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  wuffs_base__token* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }
  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_comment[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    label__0__continue:;
    while ((((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) || (((uint64_t)(io2_a_src - iop_a_src)) <= 1)) {
      if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_write);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(1);
        goto label__0__continue;
      }
      if (a_src && a_src->meta.closed) {
        status = wuffs_base__make_status(wuffs_json__error__bad_input);
        goto exit;
      }
      status = wuffs_base__make_status(wuffs_base__suspension__short_read);
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(2);
    }
    v_c2 = wuffs_base__load_u16le__no_bounds_check(iop_a_src);
    if ((v_c2 == 10799) && self->private_impl.f_quirks[10]) {
      (iop_a_src += 2, wuffs_base__make_empty_struct());
      v_length = 2;
      label__comment_block__continue:;
      while (true) {
        if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_write);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(3);
          v_length = 0;
          goto label__comment_block__continue;
        }
        while (true) {
          if (((uint64_t)(io2_a_src - iop_a_src)) <= 1) {
            if (v_length > 0) {
              *iop_a_dst++ = wuffs_base__make_token(
                  (((uint64_t)(2)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                  (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                  (((uint64_t)(v_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            }
            if (a_src && a_src->meta.closed) {
              status = wuffs_base__make_status(wuffs_json__error__bad_input);
              goto exit;
            }
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(4);
            v_length = 0;
            goto label__comment_block__continue;
          }
          v_c2 = wuffs_base__load_u16le__no_bounds_check(iop_a_src);
          if (v_c2 == 12074) {
            (iop_a_src += 2, wuffs_base__make_empty_struct());
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(2)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)((v_length + 2))) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            status = wuffs_base__make_status(NULL);
            goto ok;
          }
          (iop_a_src += 1, wuffs_base__make_empty_struct());
          if (v_length >= 65533) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(2)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                (((uint64_t)((v_length + 1))) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            v_length = 0;
            goto label__comment_block__continue;
          }
          v_length += 1;
        }
      }
    } else if ((v_c2 == 12079) && self->private_impl.f_quirks[11]) {
      (iop_a_src += 2, wuffs_base__make_empty_struct());
      v_length = 2;
      label__comment_line__continue:;
      while (true) {
        if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_write);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(5);
          v_length = 0;
          goto label__comment_line__continue;
        }
        while (true) {
          if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
            if (v_length > 0) {
              *iop_a_dst++ = wuffs_base__make_token(
                  (((uint64_t)(1)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                  (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                  (((uint64_t)(v_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            }
            if (a_src && a_src->meta.closed) {
              status = wuffs_base__make_status(wuffs_json__error__bad_input);
              goto exit;
            }
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(6);
            v_length = 0;
            goto label__comment_line__continue;
          }
          v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
          if (v_c == 10) {
            (iop_a_src += 1, wuffs_base__make_empty_struct());
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(1)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)((v_length + 1))) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            status = wuffs_base__make_status(NULL);
            goto ok;
          }
          (iop_a_src += 1, wuffs_base__make_empty_struct());
          if (v_length >= 65533) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(1)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(1)) << WUFFS_BASE__TOKEN__CONTINUED__SHIFT) |
                (((uint64_t)((v_length + 1))) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            v_length = 0;
            goto label__comment_line__continue;
          }
          v_length += 1;
        }
      }
    }
    status = wuffs_base__make_status(wuffs_json__error__bad_input);
    goto exit;

    goto ok;
    ok:
    self->private_impl.p_decode_comment[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_comment[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func json.decoder.decode_inf_nan

static wuffs_base__status
wuffs_json__decoder__decode_inf_nan(
    wuffs_json__decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint32_t v_c4 = 0;
  uint32_t v_neg = 0;

  wuffs_base__token* iop_a_dst = NULL;
  wuffs_base__token* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  wuffs_base__token* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  wuffs_base__token* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }
  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_inf_nan[0];
  if (coro_susp_point) {
    v_neg = self->private_data.s_decode_inf_nan[0].v_neg;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    label__0__continue:;
    while (true) {
      if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_write);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(1);
        goto label__0__continue;
      }
      if (((uint64_t)(io2_a_src - iop_a_src)) <= 2) {
        if (a_src && a_src->meta.closed) {
          status = wuffs_base__make_status(wuffs_json__error__bad_input);
          goto exit;
        }
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(2);
        goto label__0__continue;
      }
      v_c4 = ((uint32_t)(wuffs_base__load_u24le__no_bounds_check(iop_a_src)));
      if ((v_c4 | 2105376) == 6712937) {
        if (((uint64_t)(io2_a_src - iop_a_src)) > 7) {
          if ((wuffs_base__load_u64le__no_bounds_check(iop_a_src) | 2314885530818453536) == 8751735898823356009) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(10485792)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(8)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            (iop_a_src += 8, wuffs_base__make_empty_struct());
            status = wuffs_base__make_status(NULL);
            goto ok;
          }
        } else if ( ! (a_src && a_src->meta.closed)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(3);
          goto label__0__continue;
        }
        *iop_a_dst++ = wuffs_base__make_token(
            (((uint64_t)(10485792)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
            (((uint64_t)(3)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
        (iop_a_src += 3, wuffs_base__make_empty_struct());
        status = wuffs_base__make_status(NULL);
        goto ok;
      } else if ((v_c4 | 2105376) == 7233902) {
        *iop_a_dst++ = wuffs_base__make_token(
            (((uint64_t)(10485888)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
            (((uint64_t)(3)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
        (iop_a_src += 3, wuffs_base__make_empty_struct());
        status = wuffs_base__make_status(NULL);
        goto ok;
      } else if ((v_c4 & 255) == 43) {
        v_neg = 0;
      } else if ((v_c4 & 255) == 45) {
        v_neg = 1;
      } else {
        status = wuffs_base__make_status(wuffs_json__error__bad_input);
        goto exit;
      }
      if (((uint64_t)(io2_a_src - iop_a_src)) <= 3) {
        if (a_src && a_src->meta.closed) {
          status = wuffs_base__make_status(wuffs_json__error__bad_input);
          goto exit;
        }
        status = wuffs_base__make_status(wuffs_base__suspension__short_read);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(4);
        goto label__0__continue;
      }
      v_c4 = (wuffs_base__load_u32le__no_bounds_check(iop_a_src) >> 8);
      if ((v_c4 | 2105376) == 6712937) {
        if (((uint64_t)(io2_a_src - iop_a_src)) > 8) {
          if ((wuffs_base__load_u64le__no_bounds_check(iop_a_src + 1) | 2314885530818453536) == 8751735898823356009) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)((10485760 | (((uint32_t)(32)) >> v_neg)))) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(9)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            (iop_a_src += 9, wuffs_base__make_empty_struct());
            status = wuffs_base__make_status(NULL);
            goto ok;
          }
        } else if ( ! (a_src && a_src->meta.closed)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(5);
          goto label__0__continue;
        }
        *iop_a_dst++ = wuffs_base__make_token(
            (((uint64_t)((10485760 | (((uint32_t)(32)) >> v_neg)))) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
            (((uint64_t)(4)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
        (iop_a_src += 4, wuffs_base__make_empty_struct());
        status = wuffs_base__make_status(NULL);
        goto ok;
      } else if ((v_c4 | 2105376) == 7233902) {
        *iop_a_dst++ = wuffs_base__make_token(
            (((uint64_t)((10485760 | (((uint32_t)(128)) >> v_neg)))) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
            (((uint64_t)(4)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
        (iop_a_src += 4, wuffs_base__make_empty_struct());
        status = wuffs_base__make_status(NULL);
        goto ok;
      }
      status = wuffs_base__make_status(wuffs_json__error__bad_input);
      goto exit;
    }

    goto ok;
    ok:
    self->private_impl.p_decode_inf_nan[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_inf_nan[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_data.s_decode_inf_nan[0].v_neg = v_neg;

  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func json.decoder.decode_trailing_new_line

static wuffs_base__status
wuffs_json__decoder__decode_trailing_new_line(
    wuffs_json__decoder* self,
    wuffs_base__token_buffer* a_dst,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_c = 0;
  uint32_t v_whitespace_length = 0;

  wuffs_base__token* iop_a_dst = NULL;
  wuffs_base__token* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  wuffs_base__token* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  wuffs_base__token* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }
  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_trailing_new_line[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    label__outer__continue:;
    while (true) {
      if (((uint64_t)(io2_a_dst - iop_a_dst)) <= 0) {
        status = wuffs_base__make_status(wuffs_base__suspension__short_write);
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(1);
        v_whitespace_length = 0;
        goto label__outer__continue;
      }
      while (true) {
        if (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
          if (v_whitespace_length > 0) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(0)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(v_whitespace_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            v_whitespace_length = 0;
          }
          if (a_src && a_src->meta.closed) {
            goto label__outer__break;
          }
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(2);
          v_whitespace_length = 0;
          goto label__outer__continue;
        }
        v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
        if (WUFFS_JSON__LUT_CLASSES[v_c] != 0) {
          if (v_whitespace_length > 0) {
            *iop_a_dst++ = wuffs_base__make_token(
                (((uint64_t)(0)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
                (((uint64_t)(v_whitespace_length)) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
            v_whitespace_length = 0;
          }
          status = wuffs_base__make_status(wuffs_json__error__bad_input);
          goto exit;
        }
        (iop_a_src += 1, wuffs_base__make_empty_struct());
        if ((v_whitespace_length >= 65534) || (v_c == 10)) {
          *iop_a_dst++ = wuffs_base__make_token(
              (((uint64_t)(0)) << WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) |
              (((uint64_t)((v_whitespace_length + 1))) << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
          v_whitespace_length = 0;
          if (v_c == 10) {
            goto label__outer__break;
          }
          goto label__outer__continue;
        }
        v_whitespace_length += 1;
      }
    }
    label__outer__break:;

    goto ok;
    ok:
    self->private_impl.p_decode_trailing_new_line[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_trailing_new_line[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

#endif  // !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__JSON)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__WBMP)

// ---------------- Status Codes Implementations

const char* wuffs_wbmp__error__bad_header = "#wbmp: bad header";

// ---------------- Private Consts

// ---------------- Private Initializer Prototypes

// ---------------- Private Function Prototypes

static wuffs_base__status
wuffs_wbmp__decoder__skip_frame(
    wuffs_wbmp__decoder* self,
    wuffs_base__io_buffer* a_src);

// ---------------- VTables

const wuffs_base__image_decoder__func_ptrs
wuffs_wbmp__decoder__func_ptrs_for__wuffs_base__image_decoder = {
  (wuffs_base__status(*)(void*,
      wuffs_base__pixel_buffer*,
      wuffs_base__io_buffer*,
      wuffs_base__pixel_blend,
      wuffs_base__slice_u8,
      wuffs_base__decode_frame_options*))(&wuffs_wbmp__decoder__decode_frame),
  (wuffs_base__status(*)(void*,
      wuffs_base__frame_config*,
      wuffs_base__io_buffer*))(&wuffs_wbmp__decoder__decode_frame_config),
  (wuffs_base__status(*)(void*,
      wuffs_base__image_config*,
      wuffs_base__io_buffer*))(&wuffs_wbmp__decoder__decode_image_config),
  (wuffs_base__rect_ie_u32(*)(const void*))(&wuffs_wbmp__decoder__frame_dirty_rect),
  (uint32_t(*)(const void*))(&wuffs_wbmp__decoder__num_animation_loops),
  (uint64_t(*)(const void*))(&wuffs_wbmp__decoder__num_decoded_frame_configs),
  (uint64_t(*)(const void*))(&wuffs_wbmp__decoder__num_decoded_frames),
  (wuffs_base__status(*)(void*,
      uint64_t,
      uint64_t))(&wuffs_wbmp__decoder__restart_frame),
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_wbmp__decoder__set_quirk_enabled),
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_wbmp__decoder__set_report_metadata),
  (wuffs_base__status(*)(void*,
      wuffs_base__io_buffer*,
      wuffs_base__more_information*,
      wuffs_base__io_buffer*))(&wuffs_wbmp__decoder__tell_me_more),
  (wuffs_base__range_ii_u64(*)(const void*))(&wuffs_wbmp__decoder__workbuf_len),
};

// ---------------- Initializer Implementations

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_wbmp__decoder__initialize(
    wuffs_wbmp__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags){
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (sizeof(*self) != sizeof_star_self) {
    return wuffs_base__make_status(wuffs_base__error__bad_sizeof_receiver);
  }
  if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) ||
      (((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {
    return wuffs_base__make_status(wuffs_base__error__bad_wuffs_version);
  }

  if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {
    // The whole point of this if-check is to detect an uninitialized *self.
    // We disable the warning on GCC. Clang-5.0 does not have this warning.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if (self->private_impl.magic != 0) {
      return wuffs_base__make_status(wuffs_base__error__initialize_falsely_claimed_already_zeroed);
    }
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  } else {
    if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {
      memset(self, 0, sizeof(*self));
      initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;
    } else {
      memset(&(self->private_impl), 0, sizeof(self->private_impl));
    }
  }

  self->private_impl.magic = WUFFS_BASE__MAGIC;
  self->private_impl.vtable_for__wuffs_base__image_decoder.vtable_name =
      wuffs_base__image_decoder__vtable_name;
  self->private_impl.vtable_for__wuffs_base__image_decoder.function_pointers =
      (const void*)(&wuffs_wbmp__decoder__func_ptrs_for__wuffs_base__image_decoder);
  return wuffs_base__make_status(NULL);
}

wuffs_wbmp__decoder*
wuffs_wbmp__decoder__alloc() {
  wuffs_wbmp__decoder* x =
      (wuffs_wbmp__decoder*)(calloc(sizeof(wuffs_wbmp__decoder), 1));
  if (!x) {
    return NULL;
  }
  if (wuffs_wbmp__decoder__initialize(
      x, sizeof(wuffs_wbmp__decoder), WUFFS_VERSION, WUFFS_INITIALIZE__ALREADY_ZEROED).repr) {
    free(x);
    return NULL;
  }
  return x;
}

size_t
sizeof__wuffs_wbmp__decoder() {
  return sizeof(wuffs_wbmp__decoder);
}

// ---------------- Function Implementations

// -------- func wbmp.decoder.set_quirk_enabled

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_wbmp__decoder__set_quirk_enabled(
    wuffs_wbmp__decoder* self,
    uint32_t a_quirk,
    bool a_enabled) {
  return wuffs_base__make_empty_struct();
}

// -------- func wbmp.decoder.decode_image_config

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_wbmp__decoder__decode_image_config(
    wuffs_wbmp__decoder* self,
    wuffs_base__image_config* a_dst,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 1)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint8_t v_c = 0;
  uint32_t v_i = 0;
  uint32_t v_x32 = 0;
  uint64_t v_x64 = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_image_config[0];
  if (coro_susp_point) {
    v_i = self->private_data.s_decode_image_config[0].v_i;
    v_x32 = self->private_data.s_decode_image_config[0].v_x32;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if (self->private_impl.f_call_sequence != 0) {
      status = wuffs_base__make_status(wuffs_base__error__bad_call_sequence);
      goto exit;
    }
    v_i = 0;
    while (v_i < 2) {
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
          status = wuffs_base__make_status(wuffs_base__suspension__short_read);
          goto suspend;
        }
        uint8_t t_0 = *iop_a_src++;
        v_c = t_0;
      }
      if (v_c != 0) {
        status = wuffs_base__make_status(wuffs_wbmp__error__bad_header);
        goto exit;
      }
      v_i += 1;
    }
    v_i = 0;
    while (v_i < 2) {
      v_x32 = 0;
      while (true) {
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint8_t t_1 = *iop_a_src++;
          v_c = t_1;
        }
        v_x32 |= ((uint32_t)((v_c & 127)));
        if ((v_c >> 7) == 0) {
          goto label__0__break;
        }
        v_x64 = (((uint64_t)(v_x32)) << 7);
        if (v_x64 > 4294967295) {
          status = wuffs_base__make_status(wuffs_wbmp__error__bad_header);
          goto exit;
        }
        v_x32 = ((uint32_t)(v_x64));
      }
      label__0__break:;
      if (v_i == 0) {
        self->private_impl.f_width = v_x32;
      } else {
        self->private_impl.f_height = v_x32;
      }
      v_i += 1;
    }
    self->private_impl.f_frame_config_io_position = wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)));
    if (a_dst != NULL) {
      wuffs_base__image_config__set(
          a_dst,
          2198077448,
          0,
          self->private_impl.f_width,
          self->private_impl.f_height,
          self->private_impl.f_frame_config_io_position,
          true);
    }
    self->private_impl.f_call_sequence = 1;

    goto ok;
    ok:
    self->private_impl.p_decode_image_config[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_image_config[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 1 : 0;
  self->private_data.s_decode_image_config[0].v_i = v_i;
  self->private_data.s_decode_image_config[0].v_x32 = v_x32;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func wbmp.decoder.decode_frame_config

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_wbmp__decoder__decode_frame_config(
    wuffs_wbmp__decoder* self,
    wuffs_base__frame_config* a_dst,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 2)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_frame_config[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if (self->private_impl.f_call_sequence < 1) {
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      status = wuffs_wbmp__decoder__decode_image_config(self, NULL, a_src);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
    } else if (self->private_impl.f_call_sequence == 1) {
      if (self->private_impl.f_frame_config_io_position != wuffs_base__u64__sat_add(a_src->meta.pos, ((uint64_t)(iop_a_src - io0_a_src)))) {
        status = wuffs_base__make_status(wuffs_base__error__bad_restart);
        goto exit;
      }
    } else if (self->private_impl.f_call_sequence == 2) {
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
      status = wuffs_wbmp__decoder__skip_frame(self, a_src);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
      status = wuffs_base__make_status(wuffs_base__note__end_of_data);
      goto ok;
    } else {
      status = wuffs_base__make_status(wuffs_base__note__end_of_data);
      goto ok;
    }
    if (a_dst != NULL) {
      wuffs_base__frame_config__set(
          a_dst,
          wuffs_base__utility__make_rect_ie_u32(
          0,
          0,
          self->private_impl.f_width,
          self->private_impl.f_height),
          ((wuffs_base__flicks)(0)),
          0,
          self->private_impl.f_frame_config_io_position,
          0,
          true,
          false,
          4278190080);
    }
    self->private_impl.f_call_sequence = 2;

    goto ok;
    ok:
    self->private_impl.p_decode_frame_config[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_frame_config[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 2 : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func wbmp.decoder.decode_frame

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_wbmp__decoder__decode_frame(
    wuffs_wbmp__decoder* self,
    wuffs_base__pixel_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__pixel_blend a_blend,
    wuffs_base__slice_u8 a_workbuf,
    wuffs_base__decode_frame_options* a_opts) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 3)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  wuffs_base__status v_status = wuffs_base__make_status(NULL);
  wuffs_base__pixel_format v_dst_pixfmt = {0};
  uint32_t v_dst_bits_per_pixel = 0;
  uint64_t v_dst_bytes_per_pixel = 0;
  uint64_t v_dst_x_in_bytes = 0;
  uint32_t v_dst_x = 0;
  uint32_t v_dst_y = 0;
  wuffs_base__table_u8 v_tab = {0};
  wuffs_base__slice_u8 v_dst = {0};
  uint8_t v_src[1] = {0};
  uint8_t v_c = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_decode_frame[0];
  if (coro_susp_point) {
    v_dst_bytes_per_pixel = self->private_data.s_decode_frame[0].v_dst_bytes_per_pixel;
    v_dst_x = self->private_data.s_decode_frame[0].v_dst_x;
    v_dst_y = self->private_data.s_decode_frame[0].v_dst_y;
    memcpy(v_src, self->private_data.s_decode_frame[0].v_src, sizeof(v_src));
    v_c = self->private_data.s_decode_frame[0].v_c;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if (self->private_impl.f_call_sequence < 2) {
      if (a_src) {
        a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
      }
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
      status = wuffs_wbmp__decoder__decode_frame_config(self, NULL, a_src);
      if (a_src) {
        iop_a_src = a_src->data.ptr + a_src->meta.ri;
      }
      if (status.repr) {
        goto suspend;
      }
    } else if (self->private_impl.f_call_sequence == 2) {
    } else {
      status = wuffs_base__make_status(wuffs_base__note__end_of_data);
      goto ok;
    }
    v_status = wuffs_base__pixel_swizzler__prepare(&self->private_impl.f_swizzler,
        wuffs_base__pixel_buffer__pixel_format(a_dst),
        wuffs_base__pixel_buffer__palette(a_dst),
        wuffs_base__utility__make_pixel_format(536870920),
        wuffs_base__utility__empty_slice_u8(),
        a_blend);
    if ( ! wuffs_base__status__is_ok(&v_status)) {
      status = v_status;
      if (wuffs_base__status__is_error(&status)) {
        goto exit;
      } else if (wuffs_base__status__is_suspension(&status)) {
        status = wuffs_base__make_status(wuffs_base__error__cannot_return_a_suspension);
        goto exit;
      }
      goto ok;
    }
    v_dst_pixfmt = wuffs_base__pixel_buffer__pixel_format(a_dst);
    v_dst_bits_per_pixel = wuffs_base__pixel_format__bits_per_pixel(&v_dst_pixfmt);
    if ((v_dst_bits_per_pixel & 7) != 0) {
      status = wuffs_base__make_status(wuffs_base__error__unsupported_option);
      goto exit;
    }
    v_dst_bytes_per_pixel = ((uint64_t)((v_dst_bits_per_pixel / 8)));
    if (self->private_impl.f_width > 0) {
      v_tab = wuffs_base__pixel_buffer__plane(a_dst, 0);
      while (v_dst_y < self->private_impl.f_height) {
        v_dst = wuffs_base__table_u8__row(v_tab, v_dst_y);
        v_dst_x = 0;
        while (v_dst_x < self->private_impl.f_width) {
          if ((v_dst_x & 7) == 0) {
            while (((uint64_t)(io2_a_src - iop_a_src)) <= 0) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(2);
              v_tab = wuffs_base__pixel_buffer__plane(a_dst, 0);
              v_dst = wuffs_base__table_u8__row(v_tab, v_dst_y);
              v_dst_x_in_bytes = (((uint64_t)(v_dst_x)) * v_dst_bytes_per_pixel);
              if (v_dst_x_in_bytes <= ((uint64_t)(v_dst.len))) {
                v_dst = wuffs_base__slice_u8__subslice_i(v_dst, v_dst_x_in_bytes);
              }
            }
            v_c = wuffs_base__load_u8be__no_bounds_check(iop_a_src);
            (iop_a_src += 1, wuffs_base__make_empty_struct());
          }
          if ((v_c & 128) == 0) {
            v_src[0] = 0;
          } else {
            v_src[0] = 255;
          }
          v_c = ((uint8_t)(((((uint32_t)(v_c)) << 1) & 255)));
          wuffs_base__pixel_swizzler__swizzle_interleaved_from_slice(&self->private_impl.f_swizzler, v_dst, wuffs_base__utility__empty_slice_u8(), wuffs_base__make_slice_u8(v_src, 1));
          if (v_dst_bytes_per_pixel <= ((uint64_t)(v_dst.len))) {
            v_dst = wuffs_base__slice_u8__subslice_i(v_dst, v_dst_bytes_per_pixel);
          }
          v_dst_x += 1;
        }
        v_dst_y += 1;
      }
    }
    self->private_impl.f_call_sequence = 3;

    goto ok;
    ok:
    self->private_impl.p_decode_frame[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_decode_frame[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 3 : 0;
  self->private_data.s_decode_frame[0].v_dst_bytes_per_pixel = v_dst_bytes_per_pixel;
  self->private_data.s_decode_frame[0].v_dst_x = v_dst_x;
  self->private_data.s_decode_frame[0].v_dst_y = v_dst_y;
  memcpy(self->private_data.s_decode_frame[0].v_src, v_src, sizeof(v_src));
  self->private_data.s_decode_frame[0].v_c = v_c;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func wbmp.decoder.skip_frame

static wuffs_base__status
wuffs_wbmp__decoder__skip_frame(
    wuffs_wbmp__decoder* self,
    wuffs_base__io_buffer* a_src) {
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint64_t v_bytes_per_row = 0;
  uint64_t v_total_bytes = 0;

  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_skip_frame[0];
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    v_bytes_per_row = ((((uint64_t)(self->private_impl.f_width)) + 7) / 8);
    v_total_bytes = (v_bytes_per_row * ((uint64_t)(self->private_impl.f_height)));
    self->private_data.s_skip_frame[0].scratch = v_total_bytes;
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
    if (self->private_data.s_skip_frame[0].scratch > ((uint64_t)(io2_a_src - iop_a_src))) {
      self->private_data.s_skip_frame[0].scratch -= ((uint64_t)(io2_a_src - iop_a_src));
      iop_a_src = io2_a_src;
      status = wuffs_base__make_status(wuffs_base__suspension__short_read);
      goto suspend;
    }
    iop_a_src += self->private_data.s_skip_frame[0].scratch;
    self->private_impl.f_call_sequence = 3;

    goto ok;
    ok:
    self->private_impl.p_skip_frame[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_skip_frame[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;

  goto exit;
  exit:
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  return status;
}

// -------- func wbmp.decoder.frame_dirty_rect

WUFFS_BASE__MAYBE_STATIC wuffs_base__rect_ie_u32
wuffs_wbmp__decoder__frame_dirty_rect(
    const wuffs_wbmp__decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_rect_ie_u32();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_rect_ie_u32();
  }

  return wuffs_base__utility__make_rect_ie_u32(
      0,
      0,
      self->private_impl.f_width,
      self->private_impl.f_height);
}

// -------- func wbmp.decoder.num_animation_loops

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_wbmp__decoder__num_animation_loops(
    const wuffs_wbmp__decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  return 0;
}

// -------- func wbmp.decoder.num_decoded_frame_configs

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_wbmp__decoder__num_decoded_frame_configs(
    const wuffs_wbmp__decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  if (self->private_impl.f_call_sequence > 1) {
    return 1;
  }
  return 0;
}

// -------- func wbmp.decoder.num_decoded_frames

WUFFS_BASE__MAYBE_STATIC uint64_t
wuffs_wbmp__decoder__num_decoded_frames(
    const wuffs_wbmp__decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  if (self->private_impl.f_call_sequence > 2) {
    return 1;
  }
  return 0;
}

// -------- func wbmp.decoder.restart_frame

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_wbmp__decoder__restart_frame(
    wuffs_wbmp__decoder* self,
    uint64_t a_index,
    uint64_t a_io_position) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }

  if (self->private_impl.f_call_sequence == 0) {
    return wuffs_base__make_status(wuffs_base__error__bad_call_sequence);
  }
  if (a_index != 0) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  self->private_impl.f_call_sequence = 1;
  self->private_impl.f_frame_config_io_position = a_io_position;
  return wuffs_base__make_status(NULL);
}

// -------- func wbmp.decoder.set_report_metadata

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_wbmp__decoder__set_report_metadata(
    wuffs_wbmp__decoder* self,
    uint32_t a_fourcc,
    bool a_report) {
  return wuffs_base__make_empty_struct();
}

// -------- func wbmp.decoder.tell_me_more

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_wbmp__decoder__tell_me_more(
    wuffs_wbmp__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__more_information* a_minfo,
    wuffs_base__io_buffer* a_src) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 4)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  status = wuffs_base__make_status(wuffs_base__error__no_more_information);
  goto exit;

  goto ok;
  ok:
  goto exit;
  exit:
  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

// -------- func wbmp.decoder.workbuf_len

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_wbmp__decoder__workbuf_len(
    const wuffs_wbmp__decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  return wuffs_base__utility__make_range_ii_u64(0, 0);
}

#endif  // !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__WBMP)

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__ZLIB)

// ---------------- Status Codes Implementations

const char* wuffs_zlib__note__dictionary_required = "@zlib: dictionary required";
const char* wuffs_zlib__error__bad_checksum = "#zlib: bad checksum";
const char* wuffs_zlib__error__bad_compression_method = "#zlib: bad compression method";
const char* wuffs_zlib__error__bad_compression_window_size = "#zlib: bad compression window size";
const char* wuffs_zlib__error__bad_parity_check = "#zlib: bad parity check";
const char* wuffs_zlib__error__incorrect_dictionary = "#zlib: incorrect dictionary";

// ---------------- Private Consts

// ---------------- Private Initializer Prototypes

// ---------------- Private Function Prototypes

// ---------------- VTables

const wuffs_base__io_transformer__func_ptrs
wuffs_zlib__decoder__func_ptrs_for__wuffs_base__io_transformer = {
  (wuffs_base__empty_struct(*)(void*,
      uint32_t,
      bool))(&wuffs_zlib__decoder__set_quirk_enabled),
  (wuffs_base__status(*)(void*,
      wuffs_base__io_buffer*,
      wuffs_base__io_buffer*,
      wuffs_base__slice_u8))(&wuffs_zlib__decoder__transform_io),
  (wuffs_base__range_ii_u64(*)(const void*))(&wuffs_zlib__decoder__workbuf_len),
};

// ---------------- Initializer Implementations

wuffs_base__status WUFFS_BASE__WARN_UNUSED_RESULT
wuffs_zlib__decoder__initialize(
    wuffs_zlib__decoder* self,
    size_t sizeof_star_self,
    uint64_t wuffs_version,
    uint32_t initialize_flags){
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (sizeof(*self) != sizeof_star_self) {
    return wuffs_base__make_status(wuffs_base__error__bad_sizeof_receiver);
  }
  if (((wuffs_version >> 32) != WUFFS_VERSION_MAJOR) ||
      (((wuffs_version >> 16) & 0xFFFF) > WUFFS_VERSION_MINOR)) {
    return wuffs_base__make_status(wuffs_base__error__bad_wuffs_version);
  }

  if ((initialize_flags & WUFFS_INITIALIZE__ALREADY_ZEROED) != 0) {
    // The whole point of this if-check is to detect an uninitialized *self.
    // We disable the warning on GCC. Clang-5.0 does not have this warning.
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    if (self->private_impl.magic != 0) {
      return wuffs_base__make_status(wuffs_base__error__initialize_falsely_claimed_already_zeroed);
    }
#if !defined(__clang__) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  } else {
    if ((initialize_flags & WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED) == 0) {
      memset(self, 0, sizeof(*self));
      initialize_flags |= WUFFS_INITIALIZE__ALREADY_ZEROED;
    } else {
      memset(&(self->private_impl), 0, sizeof(self->private_impl));
    }
  }

  {
    wuffs_base__status z = wuffs_adler32__hasher__initialize(
        &self->private_data.f_checksum, sizeof(self->private_data.f_checksum), WUFFS_VERSION, initialize_flags);
    if (z.repr) {
      return z;
    }
  }
  {
    wuffs_base__status z = wuffs_adler32__hasher__initialize(
        &self->private_data.f_dict_id_hasher, sizeof(self->private_data.f_dict_id_hasher), WUFFS_VERSION, initialize_flags);
    if (z.repr) {
      return z;
    }
  }
  {
    wuffs_base__status z = wuffs_deflate__decoder__initialize(
        &self->private_data.f_flate, sizeof(self->private_data.f_flate), WUFFS_VERSION, initialize_flags);
    if (z.repr) {
      return z;
    }
  }
  self->private_impl.magic = WUFFS_BASE__MAGIC;
  self->private_impl.vtable_for__wuffs_base__io_transformer.vtable_name =
      wuffs_base__io_transformer__vtable_name;
  self->private_impl.vtable_for__wuffs_base__io_transformer.function_pointers =
      (const void*)(&wuffs_zlib__decoder__func_ptrs_for__wuffs_base__io_transformer);
  return wuffs_base__make_status(NULL);
}

wuffs_zlib__decoder*
wuffs_zlib__decoder__alloc() {
  wuffs_zlib__decoder* x =
      (wuffs_zlib__decoder*)(calloc(sizeof(wuffs_zlib__decoder), 1));
  if (!x) {
    return NULL;
  }
  if (wuffs_zlib__decoder__initialize(
      x, sizeof(wuffs_zlib__decoder), WUFFS_VERSION, WUFFS_INITIALIZE__ALREADY_ZEROED).repr) {
    free(x);
    return NULL;
  }
  return x;
}

size_t
sizeof__wuffs_zlib__decoder() {
  return sizeof(wuffs_zlib__decoder);
}

// ---------------- Function Implementations

// -------- func zlib.decoder.dictionary_id

WUFFS_BASE__MAYBE_STATIC uint32_t
wuffs_zlib__decoder__dictionary_id(
    const wuffs_zlib__decoder* self) {
  if (!self) {
    return 0;
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return 0;
  }

  return self->private_impl.f_dict_id_want;
}

// -------- func zlib.decoder.add_dictionary

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_zlib__decoder__add_dictionary(
    wuffs_zlib__decoder* self,
    wuffs_base__slice_u8 a_dict) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  if (self->private_impl.f_header_complete) {
    self->private_impl.f_bad_call_sequence = true;
  } else {
    self->private_impl.f_dict_id_got = wuffs_adler32__hasher__update_u32(&self->private_data.f_dict_id_hasher, a_dict);
    wuffs_deflate__decoder__add_history(&self->private_data.f_flate, a_dict);
  }
  self->private_impl.f_got_dictionary = true;
  return wuffs_base__make_empty_struct();
}

// -------- func zlib.decoder.set_ignore_checksum

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_zlib__decoder__set_ignore_checksum(
    wuffs_zlib__decoder* self,
    bool a_ic) {
  if (!self) {
    return wuffs_base__make_empty_struct();
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_empty_struct();
  }

  self->private_impl.f_ignore_checksum = a_ic;
  return wuffs_base__make_empty_struct();
}

// -------- func zlib.decoder.set_quirk_enabled

WUFFS_BASE__MAYBE_STATIC wuffs_base__empty_struct
wuffs_zlib__decoder__set_quirk_enabled(
    wuffs_zlib__decoder* self,
    uint32_t a_quirk,
    bool a_enabled) {
  return wuffs_base__make_empty_struct();
}

// -------- func zlib.decoder.workbuf_len

WUFFS_BASE__MAYBE_STATIC wuffs_base__range_ii_u64
wuffs_zlib__decoder__workbuf_len(
    const wuffs_zlib__decoder* self) {
  if (!self) {
    return wuffs_base__utility__empty_range_ii_u64();
  }
  if ((self->private_impl.magic != WUFFS_BASE__MAGIC) &&
      (self->private_impl.magic != WUFFS_BASE__DISABLED)) {
    return wuffs_base__utility__empty_range_ii_u64();
  }

  return wuffs_base__utility__make_range_ii_u64(1, 1);
}

// -------- func zlib.decoder.transform_io

WUFFS_BASE__MAYBE_STATIC wuffs_base__status
wuffs_zlib__decoder__transform_io(
    wuffs_zlib__decoder* self,
    wuffs_base__io_buffer* a_dst,
    wuffs_base__io_buffer* a_src,
    wuffs_base__slice_u8 a_workbuf) {
  if (!self) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if (self->private_impl.magic != WUFFS_BASE__MAGIC) {
    return wuffs_base__make_status(
        (self->private_impl.magic == WUFFS_BASE__DISABLED)
        ? wuffs_base__error__disabled_by_previous_error
        : wuffs_base__error__initialize_not_called);
  }
  if (!a_dst || !a_src) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if ((self->private_impl.active_coroutine != 0) &&
      (self->private_impl.active_coroutine != 1)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
    return wuffs_base__make_status(wuffs_base__error__interleaved_coroutine_calls);
  }
  self->private_impl.active_coroutine = 0;
  wuffs_base__status status = wuffs_base__make_status(NULL);

  uint16_t v_x = 0;
  uint32_t v_checksum_got = 0;
  wuffs_base__status v_status = wuffs_base__make_status(NULL);
  uint32_t v_checksum_want = 0;
  uint64_t v_mark = 0;

  uint8_t* iop_a_dst = NULL;
  uint8_t* io0_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io1_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  uint8_t* io2_a_dst WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_dst) {
    io0_a_dst = a_dst->data.ptr;
    io1_a_dst = io0_a_dst + a_dst->meta.wi;
    iop_a_dst = io1_a_dst;
    io2_a_dst = io0_a_dst + a_dst->data.len;
    if (a_dst->meta.closed) {
      io2_a_dst = iop_a_dst;
    }
  }
  const uint8_t* iop_a_src = NULL;
  const uint8_t* io0_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io1_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  const uint8_t* io2_a_src WUFFS_BASE__POTENTIALLY_UNUSED = NULL;
  if (a_src) {
    io0_a_src = a_src->data.ptr;
    io1_a_src = io0_a_src + a_src->meta.ri;
    iop_a_src = io1_a_src;
    io2_a_src = io0_a_src + a_src->meta.wi;
  }

  uint32_t coro_susp_point = self->private_impl.p_transform_io[0];
  if (coro_susp_point) {
    v_checksum_got = self->private_data.s_transform_io[0].v_checksum_got;
  }
  switch (coro_susp_point) {
    WUFFS_BASE__COROUTINE_SUSPENSION_POINT_0;

    if (self->private_impl.f_bad_call_sequence) {
      status = wuffs_base__make_status(wuffs_base__error__bad_call_sequence);
      goto exit;
    } else if ( ! self->private_impl.f_want_dictionary) {
      {
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(1);
        uint16_t t_0;
        if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 2)) {
          t_0 = wuffs_base__load_u16be__no_bounds_check(iop_a_src);
          iop_a_src += 2;
        } else {
          self->private_data.s_transform_io[0].scratch = 0;
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(2);
          while (true) {
            if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
              status = wuffs_base__make_status(wuffs_base__suspension__short_read);
              goto suspend;
            }
            uint64_t* scratch = &self->private_data.s_transform_io[0].scratch;
            uint32_t num_bits_0 = ((uint32_t)(*scratch & 0xFF));
            *scratch >>= 8;
            *scratch <<= 8;
            *scratch |= ((uint64_t)(*iop_a_src++)) << (56 - num_bits_0);
            if (num_bits_0 == 8) {
              t_0 = ((uint16_t)(*scratch >> 48));
              break;
            }
            num_bits_0 += 8;
            *scratch |= ((uint64_t)(num_bits_0));
          }
        }
        v_x = t_0;
      }
      if (((v_x >> 8) & 15) != 8) {
        status = wuffs_base__make_status(wuffs_zlib__error__bad_compression_method);
        goto exit;
      }
      if ((v_x >> 12) > 7) {
        status = wuffs_base__make_status(wuffs_zlib__error__bad_compression_window_size);
        goto exit;
      }
      if ((v_x % 31) != 0) {
        status = wuffs_base__make_status(wuffs_zlib__error__bad_parity_check);
        goto exit;
      }
      self->private_impl.f_want_dictionary = ((v_x & 32) != 0);
      if (self->private_impl.f_want_dictionary) {
        self->private_impl.f_dict_id_got = 1;
        {
          WUFFS_BASE__COROUTINE_SUSPENSION_POINT(3);
          uint32_t t_1;
          if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
            t_1 = wuffs_base__load_u32be__no_bounds_check(iop_a_src);
            iop_a_src += 4;
          } else {
            self->private_data.s_transform_io[0].scratch = 0;
            WUFFS_BASE__COROUTINE_SUSPENSION_POINT(4);
            while (true) {
              if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
                status = wuffs_base__make_status(wuffs_base__suspension__short_read);
                goto suspend;
              }
              uint64_t* scratch = &self->private_data.s_transform_io[0].scratch;
              uint32_t num_bits_1 = ((uint32_t)(*scratch & 0xFF));
              *scratch >>= 8;
              *scratch <<= 8;
              *scratch |= ((uint64_t)(*iop_a_src++)) << (56 - num_bits_1);
              if (num_bits_1 == 24) {
                t_1 = ((uint32_t)(*scratch >> 32));
                break;
              }
              num_bits_1 += 8;
              *scratch |= ((uint64_t)(num_bits_1));
            }
          }
          self->private_impl.f_dict_id_want = t_1;
        }
        status = wuffs_base__make_status(wuffs_zlib__note__dictionary_required);
        goto ok;
      } else if (self->private_impl.f_got_dictionary) {
        status = wuffs_base__make_status(wuffs_zlib__error__incorrect_dictionary);
        goto exit;
      }
    } else if (self->private_impl.f_dict_id_got != self->private_impl.f_dict_id_want) {
      if (self->private_impl.f_got_dictionary) {
        status = wuffs_base__make_status(wuffs_zlib__error__incorrect_dictionary);
        goto exit;
      }
      status = wuffs_base__make_status(wuffs_zlib__note__dictionary_required);
      goto ok;
    }
    self->private_impl.f_header_complete = true;
    while (true) {
      v_mark = ((uint64_t)(iop_a_dst - io0_a_dst));
      {
        if (a_dst) {
          a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
        }
        if (a_src) {
          a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
        }
        wuffs_base__status t_2 = wuffs_deflate__decoder__transform_io(&self->private_data.f_flate, a_dst, a_src, a_workbuf);
        if (a_dst) {
          iop_a_dst = a_dst->data.ptr + a_dst->meta.wi;
        }
        if (a_src) {
          iop_a_src = a_src->data.ptr + a_src->meta.ri;
        }
        v_status = t_2;
      }
      if ( ! self->private_impl.f_ignore_checksum) {
        v_checksum_got = wuffs_adler32__hasher__update_u32(&self->private_data.f_checksum, wuffs_base__io__since(v_mark, ((uint64_t)(iop_a_dst - io0_a_dst)), io0_a_dst));
      }
      if (wuffs_base__status__is_ok(&v_status)) {
        goto label__0__break;
      }
      status = v_status;
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(5);
    }
    label__0__break:;
    {
      WUFFS_BASE__COROUTINE_SUSPENSION_POINT(6);
      uint32_t t_3;
      if (WUFFS_BASE__LIKELY(io2_a_src - iop_a_src >= 4)) {
        t_3 = wuffs_base__load_u32be__no_bounds_check(iop_a_src);
        iop_a_src += 4;
      } else {
        self->private_data.s_transform_io[0].scratch = 0;
        WUFFS_BASE__COROUTINE_SUSPENSION_POINT(7);
        while (true) {
          if (WUFFS_BASE__UNLIKELY(iop_a_src == io2_a_src)) {
            status = wuffs_base__make_status(wuffs_base__suspension__short_read);
            goto suspend;
          }
          uint64_t* scratch = &self->private_data.s_transform_io[0].scratch;
          uint32_t num_bits_3 = ((uint32_t)(*scratch & 0xFF));
          *scratch >>= 8;
          *scratch <<= 8;
          *scratch |= ((uint64_t)(*iop_a_src++)) << (56 - num_bits_3);
          if (num_bits_3 == 24) {
            t_3 = ((uint32_t)(*scratch >> 32));
            break;
          }
          num_bits_3 += 8;
          *scratch |= ((uint64_t)(num_bits_3));
        }
      }
      v_checksum_want = t_3;
    }
    if ( ! self->private_impl.f_ignore_checksum && (v_checksum_got != v_checksum_want)) {
      status = wuffs_base__make_status(wuffs_zlib__error__bad_checksum);
      goto exit;
    }

    goto ok;
    ok:
    self->private_impl.p_transform_io[0] = 0;
    goto exit;
  }

  goto suspend;
  suspend:
  self->private_impl.p_transform_io[0] = wuffs_base__status__is_suspension(&status) ? coro_susp_point : 0;
  self->private_impl.active_coroutine = wuffs_base__status__is_suspension(&status) ? 1 : 0;
  self->private_data.s_transform_io[0].v_checksum_got = v_checksum_got;

  goto exit;
  exit:
  if (a_dst) {
    a_dst->meta.wi = ((size_t)(iop_a_dst - a_dst->data.ptr));
  }
  if (a_src) {
    a_src->meta.ri = ((size_t)(iop_a_src - a_src->data.ptr));
  }

  if (wuffs_base__status__is_error(&status)) {
    self->private_impl.magic = WUFFS_BASE__DISABLED;
  }
  return status;
}

#endif  // !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__ZLIB)

#endif  // WUFFS_IMPLEMENTATION

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif  // WUFFS_INCLUDE_GUARD
