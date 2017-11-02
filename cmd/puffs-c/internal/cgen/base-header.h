// After editing this file, run "go generate" in this directory.

#ifndef PUFFS_BASE_HEADER_H
#define PUFFS_BASE_HEADER_H

// Copyright 2017 The Puffs Authors.
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

// PUFFS_USE_NO_OP_PERFORMANCE_HACKS enables code paths that look like
// redundant no-ops, but for reasons to be investigated, can have dramatic
// performance effects with gcc 4.8.4 (e.g. 1.2x on some benchmarks).
//
// TODO: investigate; delete these hacks (without regressing performance).
// The order matters here. Clang also defines "__GNUC__".
#if defined(__clang__)
#elif defined(__GNUC__)
#define PUFFS_USE_NO_OP_PERFORMANCE_HACKS 1
#endif

// puffs_base__slice_u8 is a 1-dimensional buffer (a pointer and length).
//
// A value with all fields NULL or zero is a valid, empty slice.
typedef struct {
  uint8_t* ptr;
  size_t len;
} puffs_base__slice_u8;

// puffs_base__buf1 is a 1-dimensional buffer (a pointer and length), plus
// additional indexes into that buffer, plus an opened / closed flag.
//
// A value with all fields NULL or zero is a valid, empty buffer.
typedef struct {
  uint8_t* ptr;  // Pointer.
  size_t len;    // Length.
  size_t wi;     // Write index. Invariant: wi <= len.
  size_t ri;     // Read  index. Invariant: ri <= wi.
  bool closed;   // No further writes are expected.
} puffs_base__buf1;

#ifdef PUFFS_USE_NO_OP_PERFORMANCE_HACKS
typedef struct {
  void* always_null0;
  void* always_null1;
} puffs_base__paired_nulls;
#endif

typedef struct {
  // TODO: move buf into private_impl? As it is, it looks like users can modify
  // the buf field to point to a different buffer, which can turn the limit and
  // mark fields into dangling pointers.
  puffs_base__buf1* buf;
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    uint8_t* limit;
    uint8_t* mark;
    // use_limit is redundant, in that it always equals (limit != NULL), but
    // having a separate bool can have a significant performance effect with
    // gcc 4.8.4 (e.g. 1.1x on some benchmarks).
    bool use_limit;
#ifdef PUFFS_USE_NO_OP_PERFORMANCE_HACKS
    struct {
      puffs_base__paired_nulls* noph0;
      uint32_t noph1;
    } * no_op_performance_hacks;
#endif
  } private_impl;
} puffs_base__reader1;

typedef struct {
  // TODO: move buf into private_impl? As it is, it looks like users can modify
  // the buf field to point to a different buffer, which can turn the limit and
  // mark fields into dangling pointers.
  puffs_base__buf1* buf;
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    uint8_t* limit;
    uint8_t* mark;
    // use_limit is redundant, in that it always equals (limit != NULL), but
    // having a separate bool can have a significant performance effect with
    // gcc 4.8.4 (e.g. 1.1x on some benchmarks).
    // TODO: bool use_limit;
  } private_impl;
} puffs_base__writer1;

#endif  // PUFFS_BASE_HEADER_H
