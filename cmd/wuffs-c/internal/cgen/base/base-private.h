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

#ifdef __cplusplus
extern "C" {
#endif

#define WUFFS_BASE__IGNORE_POTENTIALLY_UNUSED_VARIABLE(x) (void)(x)

static inline void wuffs_base__ignore_check_wuffs_version_status(
    wuffs_base__status z) {}

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
#if defined(__clang__) && __cplusplus >= 201103L
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

static inline wuffs_base__empty_struct  //
wuffs_base__return_empty_struct() {
  return ((wuffs_base__empty_struct){});
}

// ---------------- Numeric Types

static inline uint8_t  //
wuffs_base__load_u8be(uint8_t* p) {
  return p[0];
}

static inline uint16_t  //
wuffs_base__load_u16be(uint8_t* p) {
  return ((uint16_t)(p[0]) << 8) | ((uint16_t)(p[1]) << 0);
}

static inline uint16_t  //
wuffs_base__load_u16le(uint8_t* p) {
  return ((uint16_t)(p[0]) << 0) | ((uint16_t)(p[1]) << 8);
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
  p[0] = x >> 8;
  p[1] = x >> 0;
}

static inline void  //
wuffs_base__store_u16le(uint8_t* p, uint16_t x) {
  p[0] = x >> 0;
  p[1] = x >> 8;
}

static inline void  //
wuffs_base__store_u24be(uint8_t* p, uint32_t x) {
  p[0] = x >> 16;
  p[1] = x >> 8;
  p[2] = x >> 0;
}

static inline void  //
wuffs_base__store_u24le(uint8_t* p, uint32_t x) {
  p[0] = x >> 0;
  p[1] = x >> 8;
  p[2] = x >> 16;
}

static inline void  //
wuffs_base__store_u32be(uint8_t* p, uint32_t x) {
  p[0] = x >> 24;
  p[1] = x >> 16;
  p[2] = x >> 8;
  p[3] = x >> 0;
}

static inline void  //
wuffs_base__store_u32le(uint8_t* p, uint32_t x) {
  p[0] = x >> 0;
  p[1] = x >> 8;
  p[2] = x >> 16;
  p[3] = x >> 24;
}

static inline void  //
wuffs_base__store_u40be(uint8_t* p, uint64_t x) {
  p[0] = x >> 32;
  p[1] = x >> 24;
  p[2] = x >> 16;
  p[3] = x >> 8;
  p[4] = x >> 0;
}

static inline void  //
wuffs_base__store_u40le(uint8_t* p, uint64_t x) {
  p[0] = x >> 0;
  p[1] = x >> 8;
  p[2] = x >> 16;
  p[3] = x >> 24;
  p[4] = x >> 32;
}

static inline void  //
wuffs_base__store_u48be(uint8_t* p, uint64_t x) {
  p[0] = x >> 40;
  p[1] = x >> 32;
  p[2] = x >> 24;
  p[3] = x >> 16;
  p[4] = x >> 8;
  p[5] = x >> 0;
}

static inline void  //
wuffs_base__store_u48le(uint8_t* p, uint64_t x) {
  p[0] = x >> 0;
  p[1] = x >> 8;
  p[2] = x >> 16;
  p[3] = x >> 24;
  p[4] = x >> 32;
  p[5] = x >> 40;
}

static inline void  //
wuffs_base__store_u56be(uint8_t* p, uint64_t x) {
  p[0] = x >> 48;
  p[1] = x >> 40;
  p[2] = x >> 32;
  p[3] = x >> 24;
  p[4] = x >> 16;
  p[5] = x >> 8;
  p[6] = x >> 0;
}

static inline void  //
wuffs_base__store_u56le(uint8_t* p, uint64_t x) {
  p[0] = x >> 0;
  p[1] = x >> 8;
  p[2] = x >> 16;
  p[3] = x >> 24;
  p[4] = x >> 32;
  p[5] = x >> 40;
  p[6] = x >> 48;
}

static inline void  //
wuffs_base__store_u64be(uint8_t* p, uint64_t x) {
  p[0] = x >> 56;
  p[1] = x >> 48;
  p[2] = x >> 40;
  p[3] = x >> 32;
  p[4] = x >> 24;
  p[5] = x >> 16;
  p[6] = x >> 8;
  p[7] = x >> 0;
}

static inline void  //
wuffs_base__store_u64le(uint8_t* p, uint64_t x) {
  p[0] = x >> 0;
  p[1] = x >> 8;
  p[2] = x >> 16;
  p[3] = x >> 24;
  p[4] = x >> 32;
  p[5] = x >> 40;
  p[6] = x >> 48;
  p[7] = x >> 56;
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

// wuffs_base__slice_u8__copy_from_slice calls memmove(dst.ptr, src.ptr,
// length) where length is the minimum of dst.len and src.len.
//
// Passing a wuffs_base__slice_u8 with all fields NULL or zero (a valid, empty
// slice) is valid and results in a no-op.
static inline uint64_t  //
wuffs_base__slice_u8__copy_from_slice(wuffs_base__slice_u8 dst,
                                      wuffs_base__slice_u8 src) {
  size_t length = dst.len < src.len ? dst.len : src.len;
  if (length > 0) {
    memmove(dst.ptr, src.ptr, length);
  }
  return length;
}

// --------

static inline wuffs_base__slice_u8  //
wuffs_base__table_u8__row(wuffs_base__table_u8 t, uint32_t y) {
  if (y < t.height) {
    return ((wuffs_base__slice_u8){
        .ptr = t.ptr + (t.stride * y),
        .len = t.width,
    });
  }
  return ((wuffs_base__slice_u8){});
}

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

// ---------------- Utility

static inline wuffs_base__range_ii_u32  //
wuffs_base__utility__make_range_ii_u32(const wuffs_base__utility* ignored,
                                       uint32_t min_incl,
                                       uint32_t max_incl) {
  return ((wuffs_base__range_ii_u32){
      .min_incl = min_incl,
      .max_incl = max_incl,
  });
}

static inline wuffs_base__range_ie_u32  //
wuffs_base__utility__make_range_ie_u32(const wuffs_base__utility* ignored,
                                       uint32_t min_incl,
                                       uint32_t max_excl) {
  return ((wuffs_base__range_ie_u32){
      .min_incl = min_incl,
      .max_excl = max_excl,
  });
}

static inline wuffs_base__range_ii_u64  //
wuffs_base__utility__make_range_ii_u64(const wuffs_base__utility* ignored,
                                       uint64_t min_incl,
                                       uint64_t max_incl) {
  return ((wuffs_base__range_ii_u64){
      .min_incl = min_incl,
      .max_incl = max_incl,
  });
}

static inline wuffs_base__range_ie_u64  //
wuffs_base__utility__make_range_ie_u64(const wuffs_base__utility* ignored,
                                       uint64_t min_incl,
                                       uint64_t max_excl) {
  return ((wuffs_base__range_ie_u64){
      .min_incl = min_incl,
      .max_excl = max_excl,
  });
}

static inline wuffs_base__rect_ii_u32  //
wuffs_base__utility__make_rect_ii_u32(const wuffs_base__utility* ignored,
                                      uint32_t min_incl_x,
                                      uint32_t min_incl_y,
                                      uint32_t max_incl_x,
                                      uint32_t max_incl_y) {
  return ((wuffs_base__rect_ii_u32){
      .min_incl_x = min_incl_x,
      .min_incl_y = min_incl_y,
      .max_incl_x = max_incl_x,
      .max_incl_y = max_incl_y,
  });
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__utility__make_rect_ie_u32(const wuffs_base__utility* ignored,
                                      uint32_t min_incl_x,
                                      uint32_t min_incl_y,
                                      uint32_t max_excl_x,
                                      uint32_t max_excl_y) {
  return ((wuffs_base__rect_ie_u32){
      .min_incl_x = min_incl_x,
      .min_incl_y = min_incl_y,
      .max_excl_x = max_excl_x,
      .max_excl_y = max_excl_y,
  });
}

// ---------------- I/O

static inline bool  //
wuffs_base__io_buffer__is_valid(wuffs_base__io_buffer buf) {
  return (buf.data.ptr || (buf.data.len == 0)) &&
         (buf.data.len >= buf.meta.wi) && (buf.meta.wi >= buf.meta.ri);
}

// TODO: wuffs_base__io_reader__is_eof is no longer used by Wuffs per se, but
// it might be handy to programs that use Wuffs. Either delete it, or promote
// it to the public API.
//
// If making this function public (i.e. moving it to base-header.h), it also
// needs to allow NULL (i.e. implicit, callee-calculated) mark/limit.

static inline bool  //
wuffs_base__io_reader__is_eof(wuffs_base__io_reader o) {
  wuffs_base__io_buffer* buf = o.private_impl.buf;
  return buf && buf->meta.closed &&
         (buf->data.ptr + buf->meta.wi == o.private_impl.limit);
}

static inline bool  //
wuffs_base__io_reader__is_valid(wuffs_base__io_reader o) {
  wuffs_base__io_buffer* buf = o.private_impl.buf;
  // Note: if making this function public (i.e. moving it to base-header.h), it
  // also needs to allow NULL (i.e. implicit, callee-calculated) mark/limit.
  return buf ? ((buf->data.ptr <= o.private_impl.mark) &&
                (o.private_impl.mark <= o.private_impl.limit) &&
                (o.private_impl.limit <= buf->data.ptr + buf->data.len))
             : ((o.private_impl.mark == NULL) &&
                (o.private_impl.limit == NULL));
}

static inline bool  //
wuffs_base__io_writer__is_valid(wuffs_base__io_writer o) {
  wuffs_base__io_buffer* buf = o.private_impl.buf;
  // Note: if making this function public (i.e. moving it to base-header.h), it
  // also needs to allow NULL (i.e. implicit, callee-calculated) mark/limit.
  return buf ? ((buf->data.ptr <= o.private_impl.mark) &&
                (o.private_impl.mark <= o.private_impl.limit) &&
                (o.private_impl.limit <= buf->data.ptr + buf->data.len))
             : ((o.private_impl.mark == NULL) &&
                (o.private_impl.limit == NULL));
}

static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_history(uint8_t** ptr_ptr,
                                           uint8_t* start,
                                           uint8_t* end,
                                           uint32_t length,
                                           uint32_t distance) {
  if (!distance) {
    return 0;
  }
  uint8_t* ptr = *ptr_ptr;
  if ((size_t)(ptr - start) < (size_t)(distance)) {
    return 0;
  }
  start = ptr - distance;
  size_t n = end - ptr;
  if ((size_t)(length) > n) {
    length = n;
  } else {
    n = length;
  }
  // TODO: unrolling by 3 seems best for the std/deflate benchmarks, but that
  // is mostly because 3 is the minimum length for the deflate format. This
  // function implementation shouldn't overfit to that one format. Perhaps the
  // copy_n_from_history Wuffs method should also take an unroll hint argument,
  // and the cgen can look if that argument is the constant expression '3'.
  //
  // See also wuffs_base__io_writer__copy_n_from_history_fast below.
  //
  // Alternatively, or additionally, have a sloppy_copy_n_from_history method
  // that copies 8 bytes at a time, possibly writing more than length bytes?
  for (; n >= 3; n -= 3) {
    *ptr++ = *start++;
    *ptr++ = *start++;
    *ptr++ = *start++;
  }
  for (; n; n--) {
    *ptr++ = *start++;
  }
  *ptr_ptr = ptr;
  return length;
}

// wuffs_base__io_writer__copy_n_from_history_fast is like the
// wuffs_base__io_writer__copy_n_from_history function above, but has stronger
// pre-conditions. The caller needs to prove that:
//  - distance >  0
//  - distance <= (*ptr_ptr - start)
//  - length   <= (end      - *ptr_ptr)
static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_history_fast(uint8_t** ptr_ptr,
                                                uint8_t* start,
                                                uint8_t* end,
                                                uint32_t length,
                                                uint32_t distance) {
  uint8_t* ptr = *ptr_ptr;
  start = ptr - distance;
  uint32_t n = length;
  for (; n >= 3; n -= 3) {
    *ptr++ = *start++;
    *ptr++ = *start++;
    *ptr++ = *start++;
  }
  for (; n; n--) {
    *ptr++ = *start++;
  }
  *ptr_ptr = ptr;
  return length;
}

static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_reader(uint8_t** ptr_ioptr_w,
                                          uint8_t* iobounds1_w,
                                          uint32_t length,
                                          uint8_t** ptr_ioptr_r,
                                          uint8_t* iobounds1_r) {
  uint8_t* ioptr_w = *ptr_ioptr_w;
  size_t n = length;
  if (n > ((size_t)(iobounds1_w - ioptr_w))) {
    n = iobounds1_w - ioptr_w;
  }
  uint8_t* ioptr_r = *ptr_ioptr_r;
  if (n > ((size_t)(iobounds1_r - ioptr_r))) {
    n = iobounds1_r - ioptr_r;
  }
  if (n > 0) {
    memmove(ioptr_w, ioptr_r, n);
    *ptr_ioptr_w += n;
    *ptr_ioptr_r += n;
  }
  return n;
}

static inline uint64_t  //
wuffs_base__io_writer__copy_from_slice(uint8_t** ptr_ioptr_w,
                                       uint8_t* iobounds1_w,
                                       wuffs_base__slice_u8 src) {
  uint8_t* ioptr_w = *ptr_ioptr_w;
  size_t n = src.len;
  if (n > ((size_t)(iobounds1_w - ioptr_w))) {
    n = iobounds1_w - ioptr_w;
  }
  if (n > 0) {
    memmove(ioptr_w, src.ptr, n);
    *ptr_ioptr_w += n;
  }
  return n;
}

static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_slice(uint8_t** ptr_ioptr_w,
                                         uint8_t* iobounds1_w,
                                         uint32_t length,
                                         wuffs_base__slice_u8 src) {
  uint8_t* ioptr_w = *ptr_ioptr_w;
  size_t n = src.len;
  if (n > length) {
    n = length;
  }
  if (n > ((size_t)(iobounds1_w - ioptr_w))) {
    n = iobounds1_w - ioptr_w;
  }
  if (n > 0) {
    memmove(ioptr_w, src.ptr, n);
    *ptr_ioptr_w += n;
  }
  return n;
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_reader__set_limit(wuffs_base__io_reader* o,
                                 uint8_t* ioptr_r,
                                 uint64_t limit) {
  if (o && (((size_t)(o->private_impl.limit - ioptr_r)) > limit)) {
    o->private_impl.limit = ioptr_r + limit;
  }
  return ((wuffs_base__empty_struct){});
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_reader__set_mark(wuffs_base__io_reader* o, uint8_t* mark) {
  o->private_impl.mark = mark;
  return ((wuffs_base__empty_struct){});
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_writer__set(wuffs_base__io_writer* o,
                           wuffs_base__io_buffer* b,
                           uint8_t** ioptr1_ptr,
                           uint8_t** ioptr2_ptr,
                           wuffs_base__slice_u8 s) {
  b->data.ptr = s.ptr;
  b->data.len = s.len;
  b->meta.wi = 0;
  b->meta.ri = 0;
  b->meta.pos = 0;
  b->meta.closed = false;

  o->private_impl.buf = b;
  o->private_impl.mark = s.ptr;
  o->private_impl.limit = s.ptr + s.len;
  *ioptr1_ptr = s.ptr;
  *ioptr2_ptr = s.ptr + s.len;
  return ((wuffs_base__empty_struct){});
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_writer__set_mark(wuffs_base__io_writer* o, uint8_t* mark) {
  o->private_impl.mark = mark;
  return ((wuffs_base__empty_struct){});
}

#ifdef __cplusplus
}  // extern "C"
#endif
