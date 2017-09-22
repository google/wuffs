// After editing this file, run "go generate" in this directory.

#ifndef PUFFS_BASE_IMPL_H
#define PUFFS_BASE_IMPL_H

// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#define PUFFS_IGNORE_POTENTIALLY_UNUSED_VARIABLE(x) (void)(x)

// TODO: look for (ifdef) the x86 architecture and cast the pointer? Only do so
// if a benchmark justifies the additional code path.
#define PUFFS_U16BE(p) (((uint16_t)(p[0]) << 8) | ((uint16_t)(p[1]) << 0))
#define PUFFS_U16LE(p) (((uint16_t)(p[0]) << 0) | ((uint16_t)(p[1]) << 8))
#define PUFFS_U32BE(p)                                   \
  (((uint32_t)(p[0]) << 24) | ((uint32_t)(p[1]) << 16) | \
   ((uint32_t)(p[2]) << 8) | ((uint32_t)(p[3]) << 0))
#define PUFFS_U32LE(p)                                 \
  (((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) | \
   ((uint32_t)(p[2]) << 16) | ((uint32_t)(p[3]) << 24))

// Use switch cases for coroutine suspension points, similar to the technique
// in https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
//
// We use a trivial macro instead of an explicit assignment and case statement
// so that clang-format doesn't get confused by the unusual "case"s.
#define PUFFS_COROUTINE_SUSPENSION_POINT(n) \
  coro_susp_point = n;                      \
  case n:

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

#endif  // PUFFS_BASE_IMPL_H
