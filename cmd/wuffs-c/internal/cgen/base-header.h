// After editing this file, run "go generate" in this directory.

#ifndef WUFFS_BASE_HEADER_H
#define WUFFS_BASE_HEADER_H

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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Wuffs requires a word size of at least 32 bits because it assumes that
// converting a u32 to usize will never overflow. For example, the size of a
// decoded image is often represented, explicitly or implicitly in an image
// file, as a u32, and it is convenient to compare that to a buffer size.
//
// Similarly, the word size is at most 64 bits because it assumes that
// converting a usize to u64 will never overflow.
#if __WORDSIZE < 32
#error "Wuffs requires a word size of at least 32 bits"
#elif __WORDSIZE > 64
#error "Wuffs requires a word size of at most 64 bits"
#endif

// WUFFS_VERSION is the major.minor version number as a uint32. The major
// number is the high 16 bits. The minor number is the low 16 bits.
//
// The intention is to bump the version number at least on every API / ABI
// backwards incompatible change.
//
// For now, the API and ABI are simply unstable and can change at any time.
//
// TODO: don't hard code this in base-header.h.
#define WUFFS_VERSION (0x00001)

// ---------------- I/O

// wuffs_base__slice_u8 is a 1-dimensional buffer (a pointer and length).
//
// A value with all fields NULL or zero is a valid, empty slice.
typedef struct {
  uint8_t* ptr;
  size_t len;
} wuffs_base__slice_u8;

// wuffs_base__buf1 is a 1-dimensional buffer (a pointer and length), plus
// additional indexes into that buffer, plus an opened / closed flag.
//
// A value with all fields NULL or zero is a valid, empty buffer.
typedef struct {
  uint8_t* ptr;  // Pointer.
  size_t len;    // Length.
  size_t wi;     // Write index. Invariant: wi <= len.
  size_t ri;     // Read  index. Invariant: ri <= wi.
  bool closed;   // No further writes are expected.
} wuffs_base__buf1;

// wuffs_base__limit1 provides a limited view of a 1-dimensional byte stream:
// its first N bytes. That N can be greater than a buffer's current read or
// write capacity. N decreases naturally over time as bytes are read from or
// written to the stream.
//
// A value with all fields NULL or zero is a valid, unlimited view.
typedef struct wuffs_base__limit1 {
  uint64_t* ptr_to_len;             // Pointer to N.
  struct wuffs_base__limit1* next;  // Linked list of limits.
} wuffs_base__limit1;

typedef struct {
  // TODO: move buf into private_impl? As it is, it looks like users can modify
  // the buf field to point to a different buffer, which can turn the limit and
  // mark fields into dangling pointers.
  wuffs_base__buf1* buf;
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    wuffs_base__limit1 limit;
    uint8_t* mark;
  } private_impl;
} wuffs_base__reader1;

typedef struct {
  // TODO: move buf into private_impl? As it is, it looks like users can modify
  // the buf field to point to a different buffer, which can turn the limit and
  // mark fields into dangling pointers.
  wuffs_base__buf1* buf;
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    wuffs_base__limit1 limit;
    uint8_t* mark;
  } private_impl;
} wuffs_base__writer1;

// ---------------- Images

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    uint32_t flags;
    uint32_t w;
    uint32_t h;
    // TODO: color model, including both packed RGBA and planar,
    // chroma-subsampled YCbCr.
  } private_impl;
} wuffs_base__image_config;

static inline void wuffs_base__image_config__invalidate(
    wuffs_base__image_config* c) {
  if (c) {
    *c = ((wuffs_base__image_config){});
  }
}

static inline bool wuffs_base__image_config__valid(
    wuffs_base__image_config* c) {
  if (!c || !(c->private_impl.flags & 1)) {
    return false;
  }
  uint64_t wh = ((uint64_t)c->private_impl.w) * ((uint64_t)c->private_impl.h);
  // TODO: handle things other than 1 byte per pixel.
  return wh <= ((uint64_t)SIZE_MAX);
}

static inline uint32_t wuffs_base__image_config__width(
    wuffs_base__image_config* c) {
  return wuffs_base__image_config__valid(c) ? c->private_impl.w : 0;
}

static inline uint32_t wuffs_base__image_config__height(
    wuffs_base__image_config* c) {
  return wuffs_base__image_config__valid(c) ? c->private_impl.h : 0;
}

// TODO: this is the right API for planar (not packed) pixbufs? Should it allow
// decoding into a color model different from the format's intrinsic one? For
// example, decoding a JPEG image straight to RGBA instead of to YCbCr?
static inline size_t wuffs_base__image_config__pixbuf_size(
    wuffs_base__image_config* c) {
  if (wuffs_base__image_config__valid(c)) {
    uint64_t wh = ((uint64_t)c->private_impl.w) * ((uint64_t)c->private_impl.h);
    // TODO: handle things other than 1 byte per pixel.
    return (size_t)wh;
  }
  return 0;
}

static inline void wuffs_base__image_config__initialize(
    wuffs_base__image_config* c,
    uint32_t width,
    uint32_t height,
    uint32_t TODO_color_model) {
  if (!c) {
    return;
  }
  c->private_impl.flags = 1;
  c->private_impl.w = width;
  c->private_impl.h = height;
  // TODO: color model.
}

#endif  // WUFFS_BASE_HEADER_H
