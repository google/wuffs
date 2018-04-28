// After editing this file, run "go generate" in this directory.

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

#define WUFFS_BASE__IGNORE_POTENTIALLY_UNUSED_VARIABLE(x) (void)(x)

// WUFFS_BASE__MAGIC is a magic number to check that initializers are called.
// It's not foolproof, given C doesn't automatically zero memory before use,
// but it should catch 99.99% of cases.
//
// Its (non-zero) value is arbitrary, based on md5sum("wuffs").
#define WUFFS_BASE__MAGIC ((uint32_t)0x3CCB6C71)

// WUFFS_BASE__ALREADY_ZEROED is passed from a container struct's initializer
// to a containee struct's initializer when the container has already zeroed
// the containee's memory.
//
// Its (non-zero) value is arbitrary, based on md5sum("zeroed").
#define WUFFS_BASE__ALREADY_ZEROED ((uint32_t)0x68602EF1)

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
  if (status < 0) {                                             \
    goto exit;                                                  \
  } else if (status == 0) {                                     \
    goto ok;                                                    \
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

// Uncomment this #include for printf-debugging.
// #include <stdio.h>

// The helpers below are functions, instead of macros, because their arguments
// can be an expression that we shouldn't evaluate more than once.
//
// They are in base-impl.h and hence copy/pasted into every generated C file,
// instead of being in some "base.c" file, since a design goal is that users of
// the generated C code can often just #include a single .c file, such as
// "gif.c", without having to additionally include or otherwise build and link
// a "base.c" file.
//
// They are static, so that linking multiple wuffs .o files won't complain about
// duplicate function definitions.
//
// They are explicitly marked inline, even if modern compilers don't use the
// inline attribute to guide optimizations such as inlining, to avoid the
// -Wunused-function warning, and we like to compile with -Wall -Werror.

// ---------------- Fundamentals

static inline uint16_t wuffs_base__load_u16be(uint8_t* p) {
  return ((uint16_t)(p[0]) << 8) | ((uint16_t)(p[1]) << 0);
}

static inline uint16_t wuffs_base__load_u16le(uint8_t* p) {
  return ((uint16_t)(p[0]) << 0) | ((uint16_t)(p[1]) << 8);
}

static inline uint32_t wuffs_base__load_u32be(uint8_t* p) {
  return ((uint32_t)(p[0]) << 24) | ((uint32_t)(p[1]) << 16) |
         ((uint32_t)(p[2]) << 8) | ((uint32_t)(p[3]) << 0);
}

static inline uint32_t wuffs_base__load_u32le(uint8_t* p) {
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16) | ((uint32_t)(p[3]) << 24);
}

static inline uint64_t wuffs_base__load_u64be(uint8_t* p) {
  return ((uint64_t)(p[0]) << 56) | ((uint64_t)(p[1]) << 48) |
         ((uint64_t)(p[2]) << 40) | ((uint64_t)(p[3]) << 32) |
         ((uint64_t)(p[4]) << 24) | ((uint64_t)(p[5]) << 16) |
         ((uint64_t)(p[6]) << 8) | ((uint64_t)(p[7]) << 0);
}

static inline uint64_t wuffs_base__load_u64le(uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32) | ((uint64_t)(p[5]) << 40) |
         ((uint64_t)(p[6]) << 48) | ((uint64_t)(p[7]) << 56);
}

// --------

static inline wuffs_base__slice_u8 wuffs_base__slice_u8__subslice_i(
    wuffs_base__slice_u8 s,
    uint64_t i) {
  if ((i <= SIZE_MAX) && (i <= s.len)) {
    return ((wuffs_base__slice_u8){
        .ptr = s.ptr + i,
        .len = s.len - i,
    });
  }
  return ((wuffs_base__slice_u8){});
}

static inline wuffs_base__slice_u8 wuffs_base__slice_u8__subslice_j(
    wuffs_base__slice_u8 s,
    uint64_t j) {
  if ((j <= SIZE_MAX) && (j <= s.len)) {
    return ((wuffs_base__slice_u8){.ptr = s.ptr, .len = j});
  }
  return ((wuffs_base__slice_u8){});
}

static inline wuffs_base__slice_u8 wuffs_base__slice_u8__subslice_ij(
    wuffs_base__slice_u8 s,
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

// wuffs_base__slice_u8__prefix returns up to the first up_to bytes of s.
static inline wuffs_base__slice_u8 wuffs_base__slice_u8__prefix(
    wuffs_base__slice_u8 s,
    uint64_t up_to) {
  if ((uint64_t)(s.len) > up_to) {
    s.len = up_to;
  }
  return s;
}

// wuffs_base__slice_u8__suffix returns up to the last up_to bytes of s.
static inline wuffs_base__slice_u8 wuffs_base__slice_u8_suffix(
    wuffs_base__slice_u8 s,
    uint64_t up_to) {
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
static inline uint64_t wuffs_base__slice_u8__copy_from_slice(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 src) {
  size_t length = dst.len < src.len ? dst.len : src.len;
  if (length > 0) {
    memmove(dst.ptr, src.ptr, length);
  }
  return length;
}

// --------

static inline void wuffs_base__u8__sat_add_indirect(uint8_t* x, uint8_t y) {
  *x = wuffs_base__u8__sat_add(*x, y);
}

static inline void wuffs_base__u8__sat_sub_indirect(uint8_t* x, uint8_t y) {
  *x = wuffs_base__u8__sat_sub(*x, y);
}

static inline void wuffs_base__u16__sat_add_indirect(uint16_t* x, uint16_t y) {
  *x = wuffs_base__u16__sat_add(*x, y);
}

static inline void wuffs_base__u16__sat_sub_indirect(uint16_t* x, uint16_t y) {
  *x = wuffs_base__u16__sat_sub(*x, y);
}

static inline void wuffs_base__u32__sat_add_indirect(uint32_t* x, uint32_t y) {
  *x = wuffs_base__u32__sat_add(*x, y);
}

static inline void wuffs_base__u32__sat_sub_indirect(uint32_t* x, uint32_t y) {
  *x = wuffs_base__u32__sat_sub(*x, y);
}

static inline void wuffs_base__u64__sat_add_indirect(uint64_t* x, uint64_t y) {
  *x = wuffs_base__u64__sat_add(*x, y);
}

static inline void wuffs_base__u64__sat_sub_indirect(uint64_t* x, uint64_t y) {
  *x = wuffs_base__u64__sat_sub(*x, y);
}

// ---------------- I/O

static inline bool wuffs_base__io_buffer__is_valid(wuffs_base__io_buffer buf) {
  return (buf.ptr || (buf.len == 0)) && (buf.len >= buf.wi) &&
         (buf.wi >= buf.ri);
}

static inline bool wuffs_base__io_reader__is_valid(wuffs_base__io_reader o) {
  wuffs_base__io_buffer* buf = o.private_impl.buf;
  return buf ? ((buf->ptr <= o.private_impl.bounds[0]) &&
                (o.private_impl.bounds[0] <= o.private_impl.bounds[1]) &&
                (o.private_impl.bounds[1] <= buf->ptr + buf->len))
             : ((o.private_impl.bounds[0] == NULL) &&
                (o.private_impl.bounds[1] == NULL));
}

static inline bool wuffs_base__io_writer__is_valid(wuffs_base__io_writer o) {
  wuffs_base__io_buffer* buf = o.private_impl.buf;
  return buf ? ((buf->ptr <= o.private_impl.bounds[0]) &&
                (o.private_impl.bounds[0] <= o.private_impl.bounds[1]) &&
                (o.private_impl.bounds[1] <= buf->ptr + buf->len))
             : ((o.private_impl.bounds[0] == NULL) &&
                (o.private_impl.bounds[1] == NULL));
}

static inline uint32_t wuffs_base__io_writer__copy_from_history32(
    uint8_t** ptr_ptr,
    uint8_t* start,
    uint8_t* end,
    uint32_t distance,
    uint32_t length) {
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
  // copy_from_history32 Wuffs method should also take an unroll hint argument,
  // and the cgen can look if that argument is the constant expression '3'.
  //
  // See also wuffs_base__io_writer__copy_from_history32__bco below.
  //
  // Alternatively, or additionally, have a sloppy_copy_from_history32 method
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

// wuffs_base__io_writer__copy_from_history32__bco is a Bounds Check Optimized
// version of the wuffs_base__io_writer__copy_from_history32 function above.
// The caller needs to prove that:
//  - distance >  0
//  - distance <= (*ptr_ptr - start)
//  - length   <= (end      - *ptr_ptr)
static inline uint32_t wuffs_base__io_writer__copy_from_history32__bco(
    uint8_t** ptr_ptr,
    uint8_t* start,
    uint8_t* end,
    uint32_t distance,
    uint32_t length) {
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

static inline uint32_t wuffs_base__io_writer__copy_from_reader32(
    uint8_t** ptr_wptr,
    uint8_t* wend,
    uint8_t** ptr_rptr,
    uint8_t* rend,
    uint32_t length) {
  uint8_t* wptr = *ptr_wptr;
  size_t n = length;
  if (n > wend - wptr) {
    n = wend - wptr;
  }
  uint8_t* rptr = *ptr_rptr;
  if (n > rend - rptr) {
    n = rend - rptr;
  }
  if (n > 0) {
    memmove(wptr, rptr, n);
    *ptr_wptr += n;
    *ptr_rptr += n;
  }
  return n;
}

static inline uint64_t wuffs_base__io_writer__copy_from_slice(
    uint8_t** ptr_wptr,
    uint8_t* wend,
    wuffs_base__slice_u8 src) {
  uint8_t* wptr = *ptr_wptr;
  size_t n = src.len;
  if (n > wend - wptr) {
    n = wend - wptr;
  }
  if (n > 0) {
    memmove(wptr, src.ptr, n);
    *ptr_wptr += n;
  }
  return n;
}

static inline uint32_t wuffs_base__io_writer__copy_from_slice32(
    uint8_t** ptr_wptr,
    uint8_t* wend,
    wuffs_base__slice_u8 src,
    uint32_t length) {
  uint8_t* wptr = *ptr_wptr;
  size_t n = src.len;
  if (n > length) {
    n = length;
  }
  if (n > wend - wptr) {
    n = wend - wptr;
  }
  if (n > 0) {
    memmove(wptr, src.ptr, n);
    *ptr_wptr += n;
  }
  return n;
}

// TODO: delete.
static inline wuffs_base__io_reader wuffs_base__io_reader__limit(
    wuffs_base__io_reader* o,
    uint64_t* ptr_to_len) {
  wuffs_base__io_reader ret = *o;
  ret.private_impl.limit.ptr_to_len = ptr_to_len;
  ret.private_impl.limit.next = &o->private_impl.limit;
  return ret;
}

static inline wuffs_base__empty_struct wuffs_base__io_reader__set_limit(
    wuffs_base__io_reader* o,
    uint64_t limit) {
  if (o && o->private_impl.buf) {
    uint8_t* p = o->private_impl.buf->ptr + o->private_impl.buf->ri;
    if ((o->private_impl.bounds[1] - p) > limit) {
      o->private_impl.bounds[1] = p + limit;
    }
  }
  return ((wuffs_base__empty_struct){});
}

static inline wuffs_base__empty_struct wuffs_base__io_reader__mark(
    wuffs_base__io_reader* o,
    uint8_t* mark) {
  o->private_impl.bounds[0] = mark;
  return ((wuffs_base__empty_struct){});
}

static inline wuffs_base__empty_struct wuffs_base__io_writer__set_limit(
    wuffs_base__io_writer* o,
    uint64_t limit) {
  if (o && o->private_impl.buf) {
    uint8_t* p = o->private_impl.buf->ptr + o->private_impl.buf->wi;
    if ((o->private_impl.bounds[1] - p) > limit) {
      o->private_impl.bounds[1] = p + limit;
    }
  }
  return ((wuffs_base__empty_struct){});
}

static inline wuffs_base__empty_struct wuffs_base__io_writer__mark(
    wuffs_base__io_writer* o,
    uint8_t* mark) {
  o->private_impl.bounds[0] = mark;
  return ((wuffs_base__empty_struct){});
}
