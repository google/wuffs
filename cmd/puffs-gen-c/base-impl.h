// After editing this file, run "go generate" in this directory.

#ifndef PUFFS_BASE_IMPL_H
#define PUFFS_BASE_IMPL_H

// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

// Use switch cases for coroutine state, similar to the technique in
// https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
//
// We use a trivial macro instead of an explicit assignment and case statement
// so that clang-format doesn't get confused by the unusual "case"s.
#define PUFFS_COROUTINE_STATE(n) \
  coro_state = n;                \
  case n:

#define PUFFS_LOW_BITS(x, n) ((x) & ((1 << (n)) - 1))

// Clang also defines "__GNUC__".
#if defined(__GNUC__)
#define PUFFS_LIKELY(expr) (__builtin_expect(!!(expr), 1))
#define PUFFS_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
#else
#define PUFFS_LIKELY(expr) (expr)
#define PUFFS_UNLIKELY(expr) (expr)
#endif

#endif  // PUFFS_BASE_IMPL_H
