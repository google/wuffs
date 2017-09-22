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

#endif  // PUFFS_BASE_IMPL_H
