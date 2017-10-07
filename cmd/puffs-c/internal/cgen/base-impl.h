// After editing this file, run "go generate" in this directory.

#ifndef PUFFS_BASE_IMPL_H
#define PUFFS_BASE_IMPL_H

// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#define PUFFS_IGNORE_POTENTIALLY_UNUSED_VARIABLE(x) (void)(x)

// Use switch cases for coroutine suspension points, similar to the technique
// in https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
//
// We use a trivial macro instead of an explicit assignment and case statement
// so that clang-format doesn't get confused by the unusual "case"s.
#define PUFFS_COROUTINE_SUSPENSION_POINT(n) \
  coro_susp_point = n;                      \
  case n:;

#define PUFFS_COROUTINE_SUSPENSION_POINT_MAYBE_SUSPEND(n) \
  if (status <= 0) {                                      \
    goto exit;                                            \
  }                                                       \
  coro_susp_point = n;                                    \
  goto suspend;                                           \
  case n:;

// Clang also defines "__GNUC__".
#if defined(__GNUC__)
#define PUFFS_LIKELY(expr) (__builtin_expect(!!(expr), 1))
#define PUFFS_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
// Declare the printf prototype. The generated code shouldn't need this at all,
// but it's useful for manual printf debugging.
extern int printf(const char* __restrict __format, ...);
#else
#define PUFFS_LIKELY(expr) (expr)
#define PUFFS_UNLIKELY(expr) (expr)
#endif

// ---------------- Static Inline Functions
//
// The helpers below are functions, instead of macros, because their arguments
// can be an expression that we shouldn't evaluate more than once.
//
// They are in base-impl.h and hence copy/pasted into every generated C file,
// instead of being in some "base.c" file, since a design goal is that users of
// the generated C code can often just #include a single .c file, such as
// "gif.c", without having to additionally include or otherwise build and link
// a "base.c" file.
//
// They are static, so that linking multiple puffs .o files won't complain about
// duplicate function definitions.
//
// They are explicitly marked inline, even if modern compilers don't use the
// inline attribute to guide optimizations such as inlining, to avoid the
// -Wunused-function warning, and we like to compile with -Wall -Werror.

static inline uint16_t puffs_base_load_u16be(uint8_t* p) {
  return ((uint16_t)(p[0]) << 8) | ((uint16_t)(p[1]) << 0);
}

static inline uint16_t puffs_base_load_u16le(uint8_t* p) {
  return ((uint16_t)(p[0]) << 0) | ((uint16_t)(p[1]) << 8);
}

static inline uint32_t puffs_base_load_u32be(uint8_t* p) {
  return ((uint32_t)(p[0]) << 24) | ((uint32_t)(p[1]) << 16) |
         ((uint32_t)(p[2]) << 8) | ((uint32_t)(p[3]) << 0);
}

static inline uint32_t puffs_base_load_u32le(uint8_t* p) {
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16) | ((uint32_t)(p[3]) << 24);
}

static inline puffs_base_slice_u8 puffs_base_slice_u8_subslice_i(
    puffs_base_slice_u8 s,
    uint64_t i) {
  if ((i <= SIZE_MAX) && (i <= s.len)) {
    return ((puffs_base_slice_u8){
        .ptr = s.ptr + i,
        .len = s.len - i,
    });
  }
  return ((puffs_base_slice_u8){});
}

static inline puffs_base_slice_u8 puffs_base_slice_u8_subslice_j(
    puffs_base_slice_u8 s,
    uint64_t j) {
  if ((j <= SIZE_MAX) && (j <= s.len)) {
    return ((puffs_base_slice_u8){.ptr = s.ptr, .len = j});
  }
  return ((puffs_base_slice_u8){});
}

static inline puffs_base_slice_u8
puffs_base_slice_u8_subslice_ij(puffs_base_slice_u8 s, uint64_t i, uint64_t j) {
  if ((i <= j) && (j <= SIZE_MAX) && (j <= s.len)) {
    return ((puffs_base_slice_u8){
        .ptr = s.ptr + i,
        .len = j - i,
    });
  }
  return ((puffs_base_slice_u8){});
}

// puffs_base_slice_u8_prefix returns up to the first n bytes of s.
static inline puffs_base_slice_u8 puffs_base_slice_u8_prefix(
    puffs_base_slice_u8 s,
    uint64_t n) {
  if ((uint64_t)(s.len) > n) {
    s.len = n;
  }
  return s;
}

// puffs_base_slice_u8_suffix returns up to the last n bytes of s.
static inline puffs_base_slice_u8 puffs_base_slice_u8_suffix(
    puffs_base_slice_u8 s,
    uint64_t n) {
  if ((uint64_t)(s.len) > n) {
    s.ptr += (uint64_t)(s.len) - n;
    s.len = n;
  }
  return s;
}

// puffs_base_slice_u8_copy_from calls memmove(dst.ptr, src.ptr, n) where n is
// the minimum of dst.len and src.len.
//
// Passing a puffs_base_slice_u8 with all fields NULL or zero (a valid, empty
// slice) is valid and results in a no-op.
static inline uint64_t puffs_base_slice_u8_copy_from(puffs_base_slice_u8 dst,
                                                     puffs_base_slice_u8 src) {
  size_t n = dst.len < src.len ? dst.len : src.len;
  if (n > 0) {
    memmove(dst.ptr, src.ptr, n);
  }
  return n;
}

static inline uint32_t puffs_base_writer1_copy_from32(uint8_t** ptr_wptr,
                                                      uint8_t* wend,
                                                      uint8_t** ptr_rptr,
                                                      uint8_t* rend,
                                                      uint32_t length) {
  uint8_t* wptr = *ptr_wptr;
  if (length > (wend - wptr)) {
    length = wend - wptr;
  }
  uint8_t* rptr = *ptr_rptr;
  if (length > (rend - rptr)) {
    length = rend - rptr;
  }
  if (length > 0) {
    memmove(wptr, rptr, length);
    *ptr_wptr += length;
    *ptr_rptr += length;
  }
  return length;
}

static inline uint32_t puffs_base_writer1_copy_history32(uint8_t** ptr_ptr,
                                                         uint8_t* start,
                                                         uint8_t* end,
                                                         uint32_t distance,
                                                         uint32_t length) {
  uint8_t* ptr = *ptr_ptr;
  size_t d = ptr - start;
  if ((d == 0) || (d < (size_t)(distance))) {
    return 0;
  }
  start = ptr - distance;
  size_t l = end - ptr;
  if ((size_t)(length) > l) {
    length = l;
  } else {
    l = length;
  }
  // TODO: is manual unrolling actually helpful?
  for (; l >= 8; l -= 8) {
    *ptr++ = *start++;
    *ptr++ = *start++;
    *ptr++ = *start++;
    *ptr++ = *start++;
    *ptr++ = *start++;
    *ptr++ = *start++;
    *ptr++ = *start++;
    *ptr++ = *start++;
  }
  for (; l; l--) {
    *ptr++ = *start++;
  }
  *ptr_ptr = ptr;
  return length;
}

#endif  // PUFFS_BASE_IMPL_H
