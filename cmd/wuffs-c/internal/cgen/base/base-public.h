// After editing this file, run "go generate" in the parent directory.

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
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Wuffs assumes that:
//  - converting a uint32_t to a size_t will never overflow.
//  - converting a size_t to a uint64_t will never overflow.
#ifdef __WORDSIZE
#if (__WORDSIZE != 32) && (__WORDSIZE != 64)
#error "Wuffs requires a word size of either 32 or 64 bits"
#endif
#endif

// WUFFS_VERSION is the major.minor.patch version, as per https://semver.org/,
// as a uint64_t. The major number is the high 32 bits. The minor number is the
// middle 16 bits. The patch number is the low 16 bits. The version extension
// (such as "", "beta" or "rc.1") is part of the string representation (such as
// "1.2.3-beta") but not the uint64_t representation.
//
// All three of major, minor and patch being zero means that this is a
// work-in-progress version, not a release version, and has no backwards or
// forwards compatibility guarantees.
//
// !! Some code generation programs can override WUFFS_VERSION.
#define WUFFS_VERSION ((uint64_t)0)
#define WUFFS_VERSION_MAJOR ((uint64_t)0)
#define WUFFS_VERSION_MINOR ((uint64_t)0)
#define WUFFS_VERSION_PATCH ((uint64_t)0)
#define WUFFS_VERSION_EXTENSION ""
#define WUFFS_VERSION_STRING "0.0.0"

// Define WUFFS_CONFIG__STATIC_FUNCTIONS to make all of Wuffs' functions have
// static storage. The motivation is discussed in the "ALLOW STATIC
// IMPLEMENTATION" section of
// https://raw.githubusercontent.com/nothings/stb/master/docs/stb_howto.txt
#ifdef WUFFS_CONFIG__STATIC_FUNCTIONS
#define WUFFS_BASE__MAYBE_STATIC static
#else
#define WUFFS_BASE__MAYBE_STATIC
#endif

// Clang also defines "__GNUC__".
#if defined(__GNUC__)
#define WUFFS_BASE__WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WUFFS_BASE__WARN_UNUSED_RESULT
#endif

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

// --------

// A status is either NULL (meaning OK) or a string message. That message is
// human-readable, for programmers, but it is not for end users. It is not
// localized, and does not contain additional contextual information such as a
// source filename.
//
// Status strings are statically allocated and should never be free'd. They can
// be compared by the == operator and not just by strcmp.
typedef const char* wuffs_base__status;

// !! INSERT wuffs_base__status names.

static inline bool  //
wuffs_base__status__is_error(wuffs_base__status z) {
  return z && (*z == '?');
}

static inline bool  //
wuffs_base__status__is_ok(wuffs_base__status z) {
  return z == NULL;
}

static inline bool  //
wuffs_base__status__is_suspension(wuffs_base__status z) {
  return z && (*z == '$');
}

static inline bool  //
wuffs_base__status__is_warning(wuffs_base__status z) {
  return z && (*z != '$') && (*z != '?');
}

// --------

// Flicks are a unit of time. One flick (frame-tick) is 1 / 705_600_000 of a
// second. See https://github.com/OculusVR/Flicks
typedef int64_t wuffs_base__flicks;

#define WUFFS_BASE__FLICKS_PER_SECOND ((uint64_t)705600000)
#define WUFFS_BASE__FLICKS_PER_MILLISECOND ((uint64_t)705600)

// ---------------- Numeric Types

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
  uint8_t res = x + y;
  res |= -(res < x);
  return res;
}

static inline uint8_t  //
wuffs_base__u8__sat_sub(uint8_t x, uint8_t y) {
  uint8_t res = x - y;
  res &= -(res <= x);
  return res;
}

static inline uint16_t  //
wuffs_base__u16__sat_add(uint16_t x, uint16_t y) {
  uint16_t res = x + y;
  res |= -(res < x);
  return res;
}

static inline uint16_t  //
wuffs_base__u16__sat_sub(uint16_t x, uint16_t y) {
  uint16_t res = x - y;
  res &= -(res <= x);
  return res;
}

static inline uint32_t  //
wuffs_base__u32__sat_add(uint32_t x, uint32_t y) {
  uint32_t res = x + y;
  res |= -(res < x);
  return res;
}

static inline uint32_t  //
wuffs_base__u32__sat_sub(uint32_t x, uint32_t y) {
  uint32_t res = x - y;
  res &= -(res <= x);
  return res;
}

static inline uint64_t  //
wuffs_base__u64__sat_add(uint64_t x, uint64_t y) {
  uint64_t res = x + y;
  res |= -(res < x);
  return res;
}

static inline uint64_t  //
wuffs_base__u64__sat_sub(uint64_t x, uint64_t y) {
  uint64_t res = x - y;
  res &= -(res <= x);
  return res;
}

// --------

// Clang also defines "__GNUC__".

static inline uint16_t  //
wuffs_base__u16__byte_swapped(uint16_t x) {
#if defined(__GNUC__)
  return __builtin_bswap16(x);
#else
  return (x >> 8) | (x << 8);
#endif
}

static inline uint32_t  //
wuffs_base__u32__byte_swapped(uint32_t x) {
#if defined(__GNUC__)
  return __builtin_bswap32(x);
#else
  static const uint32_t mask8 = 0x00FF00FF;
  x = ((x >> 8) & mask8) | ((x & mask8) << 8);
  return (x >> 16) | (x << 16);
#endif
}

static inline uint64_t  //
wuffs_base__u64__byte_swapped(uint64_t x) {
#if defined(__GNUC__)
  return __builtin_bswap64(x);
#else
  static const uint64_t mask8 = 0x00FF00FF00FF00FF;
  static const uint64_t mask16 = 0x0000FFFF0000FFFF;
  x = ((x >> 8) & mask8) | ((x & mask8) << 8);
  x = ((x >> 16) & mask16) | ((x & mask16) << 16);
  return (x >> 32) | (x << 32);
#endif
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

// wuffs_base__slice_u8__subslice_i returns s[i:].
//
// It returns an empty slice if i is out of bounds.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__subslice_i(wuffs_base__slice_u8 s, uint64_t i) {
  if ((i <= SIZE_MAX) && (i <= s.len)) {
    return ((wuffs_base__slice_u8){
        .ptr = s.ptr + i,
        .len = s.len - i,
    });
  }
  return ((wuffs_base__slice_u8){});
}

// wuffs_base__slice_u8__subslice_j returns s[:j].
//
// It returns an empty slice if j is out of bounds.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__subslice_j(wuffs_base__slice_u8 s, uint64_t j) {
  if ((j <= SIZE_MAX) && (j <= s.len)) {
    return ((wuffs_base__slice_u8){
        .ptr = s.ptr,
        .len = j,
    });
  }
  return ((wuffs_base__slice_u8){});
}

// wuffs_base__slice_u8__subslice_ij returns s[i:j].
//
// It returns an empty slice if i or j is out of bounds.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__subslice_ij(wuffs_base__slice_u8 s,
                                  uint64_t i,
                                  uint64_t j) {
  if ((i <= j) && (j <= SIZE_MAX) && (j <= s.len)) {
    return ((wuffs_base__slice_u8){
        .ptr = s.ptr + i,
        .len = j - i,
    });
  }
  return ((wuffs_base__slice_u8){});
}

// ---------------- Ranges and Rects

// Ranges are either inclusive ("range_ii") or exclusive ("range_ie") on the
// high end. Both the "ii" and "ie" flavors are useful in practice.
//
// The "ei" and "ee" flavors also exist in theory, but aren't widely used. In
// Wuffs, the low end is always inclusive.
//
// The "ii" (closed interval) flavor is useful when refining e.g. "the set of
// all uint32_t values" to a contiguous subset: "uint32_t values in the closed
// interval [M, N]", for uint32_t values M and N. An unrefined type (in other
// words, the set of all uint32_t values) is not representable in the "ie"
// flavor because if N equals ((1<<32) - 1) then (N + 1) will overflow.
//
// On the other hand, the "ie" (half-open interval) flavor is recommended by
// Dijkstra's "Why numbering should start at zero" at
// http://www.cs.utexas.edu/users/EWD/ewd08xx/EWD831.PDF and a further
// discussion of motivating rationale is at
// https://www.quora.com/Why-are-Python-ranges-half-open-exclusive-instead-of-closed-inclusive
//
// For example, with "ie", the number of elements in "uint32_t values in the
// half-open interval [M, N)" is equal to max(0, N-M). Furthermore, that number
// of elements (in one dimension, a length, in two dimensions, a width or
// height) is itself representable as a uint32_t without overflow, again for
// uint32_t values M and N. In the contrasting "ii" flavor, the length of the
// closed interval [0, (1<<32) - 1] is 1<<32, which cannot be represented as a
// uint32_t. In Wuffs, because of this potential overflow, the "ie" flavor has
// length / width / height methods, but the "ii" flavor does not.
//
// It is valid for min > max (for range_ii) or for min >= max (for range_ie),
// in which case the range is empty. There are multiple representations of an
// empty range.

typedef struct wuffs_base__range_ii_u32__struct {
  uint32_t min_incl;
  uint32_t max_incl;

#ifdef __cplusplus
  inline bool is_empty();
  inline bool equals(wuffs_base__range_ii_u32__struct s);
  inline bool contains(uint32_t x);
  inline wuffs_base__range_ii_u32__struct intersect(
      wuffs_base__range_ii_u32__struct s);
  inline wuffs_base__range_ii_u32__struct unite(
      wuffs_base__range_ii_u32__struct s);
#endif  // __cplusplus

} wuffs_base__range_ii_u32;

static inline bool  //
wuffs_base__range_ii_u32__is_empty(wuffs_base__range_ii_u32* r) {
  return r->min_incl > r->max_incl;
}

static inline bool  //
wuffs_base__range_ii_u32__equals(wuffs_base__range_ii_u32* r,
                                 wuffs_base__range_ii_u32 s) {
  return (r->min_incl == s.min_incl && r->max_incl == s.max_incl) ||
         (wuffs_base__range_ii_u32__is_empty(r) &&
          wuffs_base__range_ii_u32__is_empty(&s));
}

static inline bool  //
wuffs_base__range_ii_u32__contains(wuffs_base__range_ii_u32* r, uint32_t x) {
  return (r->min_incl <= x) && (x <= r->max_incl);
}

static inline  //
    wuffs_base__range_ii_u32
    wuffs_base__range_ii_u32__intersect(wuffs_base__range_ii_u32* r,
                                        wuffs_base__range_ii_u32 s) {
  wuffs_base__range_ii_u32 t;
  t.min_incl = wuffs_base__u32__max(r->min_incl, s.min_incl);
  t.max_incl = wuffs_base__u32__min(r->max_incl, s.max_incl);
  return t;
}

static inline  //
    wuffs_base__range_ii_u32
    wuffs_base__range_ii_u32__unite(wuffs_base__range_ii_u32* r,
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

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ii_u32::is_empty() {
  return wuffs_base__range_ii_u32__is_empty(this);
}

inline bool  //
wuffs_base__range_ii_u32::equals(wuffs_base__range_ii_u32 s) {
  return wuffs_base__range_ii_u32__equals(this, s);
}

inline bool  //
wuffs_base__range_ii_u32::contains(uint32_t x) {
  return wuffs_base__range_ii_u32__contains(this, x);
}

inline wuffs_base__range_ii_u32  //
wuffs_base__range_ii_u32::intersect(wuffs_base__range_ii_u32 s) {
  return wuffs_base__range_ii_u32__intersect(this, s);
}

inline wuffs_base__range_ii_u32  //
wuffs_base__range_ii_u32::unite(wuffs_base__range_ii_u32 s) {
  return wuffs_base__range_ii_u32__unite(this, s);
}

#endif  // __cplusplus

// --------

typedef struct wuffs_base__range_ie_u32__struct {
  uint32_t min_incl;
  uint32_t max_excl;

#ifdef __cplusplus
  inline bool is_empty();
  inline bool equals(wuffs_base__range_ie_u32__struct s);
  inline bool contains(uint32_t x);
  inline wuffs_base__range_ie_u32__struct intersect(
      wuffs_base__range_ie_u32__struct s);
  inline wuffs_base__range_ie_u32__struct unite(
      wuffs_base__range_ie_u32__struct s);
  inline uint32_t length();
#endif  // __cplusplus

} wuffs_base__range_ie_u32;

static inline bool  //
wuffs_base__range_ie_u32__is_empty(wuffs_base__range_ie_u32* r) {
  return r->min_incl >= r->max_excl;
}

static inline bool  //
wuffs_base__range_ie_u32__equals(wuffs_base__range_ie_u32* r,
                                 wuffs_base__range_ie_u32 s) {
  return (r->min_incl == s.min_incl && r->max_excl == s.max_excl) ||
         (wuffs_base__range_ie_u32__is_empty(r) &&
          wuffs_base__range_ie_u32__is_empty(&s));
}

static inline bool  //
wuffs_base__range_ie_u32__contains(wuffs_base__range_ie_u32* r, uint32_t x) {
  return (r->min_incl <= x) && (x < r->max_excl);
}

static inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32__intersect(wuffs_base__range_ie_u32* r,
                                    wuffs_base__range_ie_u32 s) {
  wuffs_base__range_ie_u32 t;
  t.min_incl = wuffs_base__u32__max(r->min_incl, s.min_incl);
  t.max_excl = wuffs_base__u32__min(r->max_excl, s.max_excl);
  return t;
}

static inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32__unite(wuffs_base__range_ie_u32* r,
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

static inline uint32_t  //
wuffs_base__range_ie_u32__length(wuffs_base__range_ie_u32* r) {
  return wuffs_base__u32__sat_sub(r->max_excl, r->min_incl);
}

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ie_u32::is_empty() {
  return wuffs_base__range_ie_u32__is_empty(this);
}

inline bool  //
wuffs_base__range_ie_u32::equals(wuffs_base__range_ie_u32 s) {
  return wuffs_base__range_ie_u32__equals(this, s);
}

inline bool  //
wuffs_base__range_ie_u32::contains(uint32_t x) {
  return wuffs_base__range_ie_u32__contains(this, x);
}

inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32::intersect(wuffs_base__range_ie_u32 s) {
  return wuffs_base__range_ie_u32__intersect(this, s);
}

inline wuffs_base__range_ie_u32  //
wuffs_base__range_ie_u32::unite(wuffs_base__range_ie_u32 s) {
  return wuffs_base__range_ie_u32__unite(this, s);
}

inline uint32_t  //
wuffs_base__range_ie_u32::length() {
  return wuffs_base__range_ie_u32__length(this);
}

#endif  // __cplusplus

// --------

typedef struct wuffs_base__range_ii_u64__struct {
  uint64_t min_incl;
  uint64_t max_incl;

#ifdef __cplusplus
  inline bool is_empty();
  inline bool equals(wuffs_base__range_ii_u64__struct s);
  inline bool contains(uint64_t x);
  inline wuffs_base__range_ii_u64__struct intersect(
      wuffs_base__range_ii_u64__struct s);
  inline wuffs_base__range_ii_u64__struct unite(
      wuffs_base__range_ii_u64__struct s);
#endif  // __cplusplus

} wuffs_base__range_ii_u64;

static inline bool  //
wuffs_base__range_ii_u64__is_empty(wuffs_base__range_ii_u64* r) {
  return r->min_incl > r->max_incl;
}

static inline bool  //
wuffs_base__range_ii_u64__equals(wuffs_base__range_ii_u64* r,
                                 wuffs_base__range_ii_u64 s) {
  return (r->min_incl == s.min_incl && r->max_incl == s.max_incl) ||
         (wuffs_base__range_ii_u64__is_empty(r) &&
          wuffs_base__range_ii_u64__is_empty(&s));
}

static inline bool  //
wuffs_base__range_ii_u64__contains(wuffs_base__range_ii_u64* r, uint64_t x) {
  return (r->min_incl <= x) && (x <= r->max_incl);
}

static inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64__intersect(wuffs_base__range_ii_u64* r,
                                    wuffs_base__range_ii_u64 s) {
  wuffs_base__range_ii_u64 t;
  t.min_incl = wuffs_base__u64__max(r->min_incl, s.min_incl);
  t.max_incl = wuffs_base__u64__min(r->max_incl, s.max_incl);
  return t;
}

static inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64__unite(wuffs_base__range_ii_u64* r,
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

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ii_u64::is_empty() {
  return wuffs_base__range_ii_u64__is_empty(this);
}

inline bool  //
wuffs_base__range_ii_u64::equals(wuffs_base__range_ii_u64 s) {
  return wuffs_base__range_ii_u64__equals(this, s);
}

inline bool  //
wuffs_base__range_ii_u64::contains(uint64_t x) {
  return wuffs_base__range_ii_u64__contains(this, x);
}

inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64::intersect(wuffs_base__range_ii_u64 s) {
  return wuffs_base__range_ii_u64__intersect(this, s);
}

inline wuffs_base__range_ii_u64  //
wuffs_base__range_ii_u64::unite(wuffs_base__range_ii_u64 s) {
  return wuffs_base__range_ii_u64__unite(this, s);
}

#endif  // __cplusplus

// --------

typedef struct wuffs_base__range_ie_u64__struct {
  uint64_t min_incl;
  uint64_t max_excl;

#ifdef __cplusplus
  inline bool is_empty();
  inline bool equals(wuffs_base__range_ie_u64__struct s);
  inline bool contains(uint64_t x);
  inline wuffs_base__range_ie_u64__struct intersect(
      wuffs_base__range_ie_u64__struct s);
  inline wuffs_base__range_ie_u64__struct unite(
      wuffs_base__range_ie_u64__struct s);
  inline uint64_t length();
#endif  // __cplusplus

} wuffs_base__range_ie_u64;

static inline bool  //
wuffs_base__range_ie_u64__is_empty(wuffs_base__range_ie_u64* r) {
  return r->min_incl >= r->max_excl;
}

static inline bool  //
wuffs_base__range_ie_u64__equals(wuffs_base__range_ie_u64* r,
                                 wuffs_base__range_ie_u64 s) {
  return (r->min_incl == s.min_incl && r->max_excl == s.max_excl) ||
         (wuffs_base__range_ie_u64__is_empty(r) &&
          wuffs_base__range_ie_u64__is_empty(&s));
}

static inline bool  //
wuffs_base__range_ie_u64__contains(wuffs_base__range_ie_u64* r, uint64_t x) {
  return (r->min_incl <= x) && (x < r->max_excl);
}

static inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64__intersect(wuffs_base__range_ie_u64* r,
                                    wuffs_base__range_ie_u64 s) {
  wuffs_base__range_ie_u64 t;
  t.min_incl = wuffs_base__u64__max(r->min_incl, s.min_incl);
  t.max_excl = wuffs_base__u64__min(r->max_excl, s.max_excl);
  return t;
}

static inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64__unite(wuffs_base__range_ie_u64* r,
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

static inline uint64_t  //
wuffs_base__range_ie_u64__length(wuffs_base__range_ie_u64* r) {
  return wuffs_base__u64__sat_sub(r->max_excl, r->min_incl);
}

#ifdef __cplusplus

inline bool  //
wuffs_base__range_ie_u64::is_empty() {
  return wuffs_base__range_ie_u64__is_empty(this);
}

inline bool  //
wuffs_base__range_ie_u64::equals(wuffs_base__range_ie_u64 s) {
  return wuffs_base__range_ie_u64__equals(this, s);
}

inline bool  //
wuffs_base__range_ie_u64::contains(uint64_t x) {
  return wuffs_base__range_ie_u64__contains(this, x);
}

inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64::intersect(wuffs_base__range_ie_u64 s) {
  return wuffs_base__range_ie_u64__intersect(this, s);
}

inline wuffs_base__range_ie_u64  //
wuffs_base__range_ie_u64::unite(wuffs_base__range_ie_u64 s) {
  return wuffs_base__range_ie_u64__unite(this, s);
}

inline uint64_t  //
wuffs_base__range_ie_u64::length() {
  return wuffs_base__range_ie_u64__length(this);
}

#endif  // __cplusplus

// --------

// wuffs_base__rect_ii_u32 is a rectangle (a 2-dimensional range) on the
// integer grid. The "ii" means that the bounds are inclusive on the low end
// and inclusive on the high end. It contains all points (x, y) such that
// ((min_incl_x <= x) && (x <= max_incl_x)) and likewise for y.
//
// It is valid for min > max, in which case the rectangle is empty. There are
// multiple representations of an empty rectangle.
//
// The X and Y axes increase right and down.
typedef struct wuffs_base__rect_ii_u32__struct {
  uint32_t min_incl_x;
  uint32_t min_incl_y;
  uint32_t max_incl_x;
  uint32_t max_incl_y;

#ifdef __cplusplus
  inline bool is_empty();
  inline bool equals(wuffs_base__rect_ii_u32__struct s);
  inline bool contains(uint32_t x, uint32_t y);
  inline wuffs_base__rect_ii_u32__struct intersect(
      wuffs_base__rect_ii_u32__struct s);
  inline wuffs_base__rect_ii_u32__struct unite(
      wuffs_base__rect_ii_u32__struct s);
#endif  // __cplusplus

} wuffs_base__rect_ii_u32;

static inline bool  //
wuffs_base__rect_ii_u32__is_empty(wuffs_base__rect_ii_u32* r) {
  return (r->min_incl_x > r->max_incl_x) || (r->min_incl_y > r->max_incl_y);
}

static inline bool  //
wuffs_base__rect_ii_u32__equals(wuffs_base__rect_ii_u32* r,
                                wuffs_base__rect_ii_u32 s) {
  return (r->min_incl_x == s.min_incl_x && r->min_incl_y == s.min_incl_y &&
          r->max_incl_x == s.max_incl_x && r->max_incl_y == s.max_incl_y) ||
         (wuffs_base__rect_ii_u32__is_empty(r) &&
          wuffs_base__rect_ii_u32__is_empty(&s));
}

static inline bool  //
wuffs_base__rect_ii_u32__contains(wuffs_base__rect_ii_u32* r,
                                  uint32_t x,
                                  uint32_t y) {
  return (r->min_incl_x <= x) && (x <= r->max_incl_x) && (r->min_incl_y <= y) &&
         (y <= r->max_incl_y);
}

static inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32__intersect(wuffs_base__rect_ii_u32* r,
                                   wuffs_base__rect_ii_u32 s) {
  wuffs_base__rect_ii_u32 t;
  t.min_incl_x = wuffs_base__u32__max(r->min_incl_x, s.min_incl_x);
  t.min_incl_y = wuffs_base__u32__max(r->min_incl_y, s.min_incl_y);
  t.max_incl_x = wuffs_base__u32__min(r->max_incl_x, s.max_incl_x);
  t.max_incl_y = wuffs_base__u32__min(r->max_incl_y, s.max_incl_y);
  return t;
}

static inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32__unite(wuffs_base__rect_ii_u32* r,
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

#ifdef __cplusplus

inline bool  //
wuffs_base__rect_ii_u32::is_empty() {
  return wuffs_base__rect_ii_u32__is_empty(this);
}

inline bool  //
wuffs_base__rect_ii_u32::equals(wuffs_base__rect_ii_u32 s) {
  return wuffs_base__rect_ii_u32__equals(this, s);
}

inline bool  //
wuffs_base__rect_ii_u32::contains(uint32_t x, uint32_t y) {
  return wuffs_base__rect_ii_u32__contains(this, x, y);
}

inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32::intersect(wuffs_base__rect_ii_u32 s) {
  return wuffs_base__rect_ii_u32__intersect(this, s);
}

inline wuffs_base__rect_ii_u32  //
wuffs_base__rect_ii_u32::unite(wuffs_base__rect_ii_u32 s) {
  return wuffs_base__rect_ii_u32__unite(this, s);
}

#endif  // __cplusplus

// --------

// wuffs_base__rect_ie_u32 is a rectangle (a 2-dimensional range) on the
// integer grid. The "ie" means that the bounds are inclusive on the low end
// and exclusive on the high end. It contains all points (x, y) such that
// ((min_incl_x <= x) && (x < max_excl_x)) and likewise for y.
//
// It is valid for min >= max, in which case the rectangle is empty. There are
// multiple representations of an empty rectangle, including a value with all
// fields zero.
//
// The X and Y axes increase right and down.
typedef struct wuffs_base__rect_ie_u32__struct {
  uint32_t min_incl_x;
  uint32_t min_incl_y;
  uint32_t max_excl_x;
  uint32_t max_excl_y;

#ifdef __cplusplus
  inline bool is_empty();
  inline bool equals(wuffs_base__rect_ie_u32__struct s);
  inline bool contains(uint32_t x, uint32_t y);
  inline wuffs_base__rect_ie_u32__struct intersect(
      wuffs_base__rect_ie_u32__struct s);
  inline wuffs_base__rect_ie_u32__struct unite(
      wuffs_base__rect_ie_u32__struct s);
  inline uint32_t width();
  inline uint32_t height();
#endif  // __cplusplus

} wuffs_base__rect_ie_u32;

static inline bool  //
wuffs_base__rect_ie_u32__is_empty(wuffs_base__rect_ie_u32* r) {
  return (r->min_incl_x >= r->max_excl_x) || (r->min_incl_y >= r->max_excl_y);
}

static inline bool  //
wuffs_base__rect_ie_u32__equals(wuffs_base__rect_ie_u32* r,
                                wuffs_base__rect_ie_u32 s) {
  return (r->min_incl_x == s.min_incl_x && r->min_incl_y == s.min_incl_y &&
          r->max_excl_x == s.max_excl_x && r->max_excl_y == s.max_excl_y) ||
         (wuffs_base__rect_ie_u32__is_empty(r) &&
          wuffs_base__rect_ie_u32__is_empty(&s));
}

static inline bool  //
wuffs_base__rect_ie_u32__contains(wuffs_base__rect_ie_u32* r,
                                  uint32_t x,
                                  uint32_t y) {
  return (r->min_incl_x <= x) && (x < r->max_excl_x) && (r->min_incl_y <= y) &&
         (y < r->max_excl_y);
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32__intersect(wuffs_base__rect_ie_u32* r,
                                   wuffs_base__rect_ie_u32 s) {
  wuffs_base__rect_ie_u32 t;
  t.min_incl_x = wuffs_base__u32__max(r->min_incl_x, s.min_incl_x);
  t.min_incl_y = wuffs_base__u32__max(r->min_incl_y, s.min_incl_y);
  t.max_excl_x = wuffs_base__u32__min(r->max_excl_x, s.max_excl_x);
  t.max_excl_y = wuffs_base__u32__min(r->max_excl_y, s.max_excl_y);
  return t;
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32__unite(wuffs_base__rect_ie_u32* r,
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

static inline uint32_t  //
wuffs_base__rect_ie_u32__width(wuffs_base__rect_ie_u32* r) {
  return wuffs_base__u32__sat_sub(r->max_excl_x, r->min_incl_x);
}

static inline uint32_t  //
wuffs_base__rect_ie_u32__height(wuffs_base__rect_ie_u32* r) {
  return wuffs_base__u32__sat_sub(r->max_excl_y, r->min_incl_y);
}

#ifdef __cplusplus

inline bool  //
wuffs_base__rect_ie_u32::is_empty() {
  return wuffs_base__rect_ie_u32__is_empty(this);
}

inline bool  //
wuffs_base__rect_ie_u32::equals(wuffs_base__rect_ie_u32 s) {
  return wuffs_base__rect_ie_u32__equals(this, s);
}

inline bool  //
wuffs_base__rect_ie_u32::contains(uint32_t x, uint32_t y) {
  return wuffs_base__rect_ie_u32__contains(this, x, y);
}

inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32::intersect(wuffs_base__rect_ie_u32 s) {
  return wuffs_base__rect_ie_u32__intersect(this, s);
}

inline wuffs_base__rect_ie_u32  //
wuffs_base__rect_ie_u32::unite(wuffs_base__rect_ie_u32 s) {
  return wuffs_base__rect_ie_u32__unite(this, s);
}

inline uint32_t  //
wuffs_base__rect_ie_u32::width() {
  return wuffs_base__rect_ie_u32__width(this);
}

inline uint32_t  //
wuffs_base__rect_ie_u32::height() {
  return wuffs_base__rect_ie_u32__height(this);
}

#endif  // __cplusplus

// ---------------- I/O

struct wuffs_base__io_buffer__struct;

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    struct wuffs_base__io_buffer__struct* buf;
    // The bounds values are typically NULL, when created by the Wuffs public
    // API. NULL means that the callee substitutes the implicit bounds derived
    // from buf.
    uint8_t* mark;
    uint8_t* limit;
  } private_impl;
} wuffs_base__io_reader;

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    struct wuffs_base__io_buffer__struct* buf;
    // The bounds values are typically NULL, when created by the Wuffs public
    // API. NULL means that the callee substitutes the implicit bounds derived
    // from buf.
    uint8_t* mark;
    uint8_t* limit;
  } private_impl;
} wuffs_base__io_writer;

// wuffs_base__io_buffer is a 1-dimensional buffer (a pointer and length), plus
// additional indexes into that buffer, plus a buffer position and an opened /
// closed flag.
//
// A value with all fields NULL or zero is a valid, empty buffer.
typedef struct wuffs_base__io_buffer__struct {
  uint8_t* ptr;  // Pointer.
  size_t len;    // Length.
  size_t wi;     // Write index. Invariant: wi <= len.
  size_t ri;     // Read  index. Invariant: ri <= wi.
  uint64_t pos;  // Position of the buffer start relative to the stream start.
  bool closed;   // No further writes are expected.

#ifdef __cplusplus
  inline void compact();
  inline uint64_t io_position_reader();
  inline uint64_t io_position_writer();
  inline wuffs_base__io_reader reader();
  inline wuffs_base__io_writer writer();
#endif  // __cplusplus

} wuffs_base__io_buffer;

// wuffs_base__io_buffer__compact moves any written but unread bytes to the
// start of the buffer.
static inline void  //
wuffs_base__io_buffer__compact(wuffs_base__io_buffer* buf) {
  if (!buf || (buf->ri == 0)) {
    return;
  }
  buf->pos = wuffs_base__u64__sat_add(buf->pos, buf->ri);
  size_t n = buf->wi - buf->ri;
  if (n != 0) {
    memmove(buf->ptr, buf->ptr + buf->ri, n);
  }
  buf->wi = n;
  buf->ri = 0;
}

static inline uint64_t  //
wuffs_base__io_buffer__io_position_reader(wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->pos, buf->ri) : 0;
}

static inline uint64_t  //
wuffs_base__io_buffer__io_position_writer(wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->pos, buf->wi) : 0;
}

static inline wuffs_base__io_reader  //
wuffs_base__io_buffer__reader(wuffs_base__io_buffer* buf) {
  wuffs_base__io_reader ret = ((wuffs_base__io_reader){});
  ret.private_impl.buf = buf;
  return ret;
}

static inline wuffs_base__io_writer  //
wuffs_base__io_buffer__writer(wuffs_base__io_buffer* buf) {
  wuffs_base__io_writer ret = ((wuffs_base__io_writer){});
  ret.private_impl.buf = buf;
  return ret;
}

#ifdef __cplusplus

inline void  //
wuffs_base__io_buffer__struct::compact() {
  wuffs_base__io_buffer__compact(this);
}

inline uint64_t  //
wuffs_base__io_buffer__struct::io_position_reader() {
  return wuffs_base__io_buffer__io_position_reader(this);
}

inline uint64_t  //
wuffs_base__io_buffer__struct::io_position_writer() {
  return wuffs_base__io_buffer__io_position_writer(this);
}

inline wuffs_base__io_reader  //
wuffs_base__io_buffer__struct::reader() {
  return wuffs_base__io_buffer__reader(this);
}

inline wuffs_base__io_writer  //
wuffs_base__io_buffer__struct::writer() {
  return wuffs_base__io_buffer__writer(this);
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
      return ((wuffs_base__slice_u8){
          .ptr = p,
          .len = num_u8,
      });
    }
  }
  return ((wuffs_base__slice_u8){});
}

static inline wuffs_base__slice_u16  //
wuffs_base__malloc_slice_u16(void* (*malloc_func)(size_t), uint64_t num_u16) {
  if (malloc_func && (num_u16 <= (SIZE_MAX / sizeof(uint16_t)))) {
    void* p = (*malloc_func)(num_u16 * sizeof(uint16_t));
    if (p) {
      return ((wuffs_base__slice_u16){
          .ptr = p,
          .len = num_u16,
      });
    }
  }
  return ((wuffs_base__slice_u16){});
}

static inline wuffs_base__slice_u32  //
wuffs_base__malloc_slice_u32(void* (*malloc_func)(size_t), uint64_t num_u32) {
  if (malloc_func && (num_u32 <= (SIZE_MAX / sizeof(uint32_t)))) {
    void* p = (*malloc_func)(num_u32 * sizeof(uint32_t));
    if (p) {
      return ((wuffs_base__slice_u32){
          .ptr = p,
          .len = num_u32,
      });
    }
  }
  return ((wuffs_base__slice_u32){});
}

static inline wuffs_base__slice_u64  //
wuffs_base__malloc_slice_u64(void* (*malloc_func)(size_t), uint64_t num_u64) {
  if (malloc_func && (num_u64 <= (SIZE_MAX / sizeof(uint64_t)))) {
    void* p = (*malloc_func)(num_u64 * sizeof(uint64_t));
    if (p) {
      return ((wuffs_base__slice_u64){
          .ptr = p,
          .len = num_u64,
      });
    }
  }
  return ((wuffs_base__slice_u64){});
}

#ifdef __cplusplus
}  // extern "C"
#endif
