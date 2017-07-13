// After editing this file, run "go generate" in this directory.

#ifndef PUFFS_BASE_IMPL_H
#define PUFFS_BASE_IMPL_H

// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// TODO: look for (ifdef) the x86 architecture and cast the pointer? Only do so
// if a benchmark justifies the additional code path.
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
#else
#define PUFFS_LIKELY(expr) (expr)
#define PUFFS_UNLIKELY(expr) (expr)
#endif

#endif  // PUFFS_BASE_IMPL_H
