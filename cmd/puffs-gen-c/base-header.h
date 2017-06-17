// After editing this file, run "go generate" in this directory.

#ifndef PUFFS_BASE_HEADER_H
#define PUFFS_BASE_HEADER_H

// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Puffs requires a word size of at least 32 bits because it assumes that
// converting a u32 to usize will never overflow. For example, the size of a
// decoded image is often represented, explicitly or implicitly in an image
// file, as a u32, and it is convenient to compare that to a buffer size.
//
// Similarly, the word size is at most 64 bits because it assumes that
// converting a usize to u64 will never overflow.
#if __WORDSIZE < 32
#error "Puffs requires a word size of at least 32 bits"
#elif __WORDSIZE > 64
#error "Puffs requires a word size of at most 64 bits"
#endif

// PUFFS_VERSION is the major.minor version number as a uint32. The major
// number is the high 16 bits. The minor number is the low 16 bits.
//
// The intention is to bump the version number at least on every API / ABI
// backwards incompatible change.
//
// For now, the API and ABI are simply unstable and can change at any time.
//
// TODO: don't hard code this in base-header.h.
#define PUFFS_VERSION (0x00001)

// puffs_base_buf1 is a 1-dimensional buffer (a pointer and length) plus
// additional indexes into that buffer.
//
// A value with all fields NULL or zero is a valid, empty buffer.
typedef struct {
  uint8_t* ptr;  // Pointer.
  size_t len;    // Length.
  size_t wi;     // Write index. Invariant: wi <= len.
  size_t ri;     // Read  index. Invariant: ri <= wi.
  bool closed;   // No further writes are expected.
} puffs_base_buf1;

#endif  // PUFFS_BASE_HEADER_H
