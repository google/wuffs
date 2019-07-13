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

static inline wuffs_base__empty_struct  //
wuffs_base__ignore_status(wuffs_base__status z) {
  return wuffs_base__make_empty_struct();
}

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
  if (!status) {                                                \
    goto ok;                                                    \
  } else if (*status != '$') {                                  \
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

// The helpers below are functions, instead of macros, because their arguments
// can be an expression that we shouldn't evaluate more than once.
//
// They are static, so that linking multiple wuffs .o files won't complain about
// duplicate function definitions.
//
// They are explicitly marked inline, even if modern compilers don't use the
// inline attribute to guide optimizations such as inlining, to avoid the
// -Wunused-function warning, and we like to compile with -Wall -Werror.

// ---------------- Numeric Types

static inline uint8_t  //
wuffs_base__load_u8be(uint8_t* p) {
  return p[0];
}

static inline uint16_t  //
wuffs_base__load_u16be(uint8_t* p) {
  return (uint16_t)(((uint16_t)(p[0]) << 8) | ((uint16_t)(p[1]) << 0));
}

static inline uint16_t  //
wuffs_base__load_u16le(uint8_t* p) {
  return (uint16_t)(((uint16_t)(p[0]) << 0) | ((uint16_t)(p[1]) << 8));
}

static inline uint32_t  //
wuffs_base__load_u24be(uint8_t* p) {
  return ((uint32_t)(p[0]) << 16) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 0);
}

static inline uint32_t  //
wuffs_base__load_u24le(uint8_t* p) {
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16);
}

static inline uint32_t  //
wuffs_base__load_u32be(uint8_t* p) {
  return ((uint32_t)(p[0]) << 24) | ((uint32_t)(p[1]) << 16) |
         ((uint32_t)(p[2]) << 8) | ((uint32_t)(p[3]) << 0);
}

static inline uint32_t  //
wuffs_base__load_u32le(uint8_t* p) {
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16) | ((uint32_t)(p[3]) << 24);
}

static inline uint64_t  //
wuffs_base__load_u40be(uint8_t* p) {
  return ((uint64_t)(p[0]) << 32) | ((uint64_t)(p[1]) << 24) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 8) |
         ((uint64_t)(p[4]) << 0);
}

static inline uint64_t  //
wuffs_base__load_u40le(uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32);
}

static inline uint64_t  //
wuffs_base__load_u48be(uint8_t* p) {
  return ((uint64_t)(p[0]) << 40) | ((uint64_t)(p[1]) << 32) |
         ((uint64_t)(p[2]) << 24) | ((uint64_t)(p[3]) << 16) |
         ((uint64_t)(p[4]) << 8) | ((uint64_t)(p[5]) << 0);
}

static inline uint64_t  //
wuffs_base__load_u48le(uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32) | ((uint64_t)(p[5]) << 40);
}

static inline uint64_t  //
wuffs_base__load_u56be(uint8_t* p) {
  return ((uint64_t)(p[0]) << 48) | ((uint64_t)(p[1]) << 40) |
         ((uint64_t)(p[2]) << 32) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 16) | ((uint64_t)(p[5]) << 8) |
         ((uint64_t)(p[6]) << 0);
}

static inline uint64_t  //
wuffs_base__load_u56le(uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32) | ((uint64_t)(p[5]) << 40) |
         ((uint64_t)(p[6]) << 48);
}

static inline uint64_t  //
wuffs_base__load_u64be(uint8_t* p) {
  return ((uint64_t)(p[0]) << 56) | ((uint64_t)(p[1]) << 48) |
         ((uint64_t)(p[2]) << 40) | ((uint64_t)(p[3]) << 32) |
         ((uint64_t)(p[4]) << 24) | ((uint64_t)(p[5]) << 16) |
         ((uint64_t)(p[6]) << 8) | ((uint64_t)(p[7]) << 0);
}

static inline uint64_t  //
wuffs_base__load_u64le(uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32) | ((uint64_t)(p[5]) << 40) |
         ((uint64_t)(p[6]) << 48) | ((uint64_t)(p[7]) << 56);
}

// --------

static inline void  //
wuffs_base__store_u8be(uint8_t* p, uint8_t x) {
  p[0] = x;
}

static inline void  //
wuffs_base__store_u16be(uint8_t* p, uint16_t x) {
  p[0] = (uint8_t)(x >> 8);
  p[1] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u16le(uint8_t* p, uint16_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
}

static inline void  //
wuffs_base__store_u24be(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 16);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u24le(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
}

static inline void  //
wuffs_base__store_u32be(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 24);
  p[1] = (uint8_t)(x >> 16);
  p[2] = (uint8_t)(x >> 8);
  p[3] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u32le(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
}

static inline void  //
wuffs_base__store_u40be(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 32);
  p[1] = (uint8_t)(x >> 24);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 8);
  p[4] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u40le(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
}

static inline void  //
wuffs_base__store_u48be(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 40);
  p[1] = (uint8_t)(x >> 32);
  p[2] = (uint8_t)(x >> 24);
  p[3] = (uint8_t)(x >> 16);
  p[4] = (uint8_t)(x >> 8);
  p[5] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u48le(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
  p[5] = (uint8_t)(x >> 40);
}

static inline void  //
wuffs_base__store_u56be(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 48);
  p[1] = (uint8_t)(x >> 40);
  p[2] = (uint8_t)(x >> 32);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 16);
  p[5] = (uint8_t)(x >> 8);
  p[6] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__store_u56le(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
  p[5] = (uint8_t)(x >> 40);
  p[6] = (uint8_t)(x >> 48);
}

static inline void  //
wuffs_base__store_u64be(uint8_t* p, uint64_t x) {
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
wuffs_base__store_u64le(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
  p[5] = (uint8_t)(x >> 40);
  p[6] = (uint8_t)(x >> 48);
  p[7] = (uint8_t)(x >> 56);
}

// --------

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
