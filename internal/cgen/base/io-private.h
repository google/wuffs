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

// ---------------- I/O

static inline uint64_t  //
wuffs_base__io__count_since(uint64_t mark, uint64_t index) {
  if (index >= mark) {
    return index - mark;
  }
  return 0;
}

static inline wuffs_base__slice_u8  //
wuffs_base__io__since(uint64_t mark, uint64_t index, uint8_t* ptr) {
  if (index >= mark) {
    return wuffs_base__make_slice_u8(ptr + mark, index - mark);
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_history(uint8_t** ptr_iop_w,
                                           uint8_t* io1_w,
                                           uint8_t* io2_w,
                                           uint32_t length,
                                           uint32_t distance) {
  if (!distance) {
    return 0;
  }
  uint8_t* p = *ptr_iop_w;
  if ((size_t)(p - io1_w) < (size_t)(distance)) {
    return 0;
  }
  uint8_t* q = p - distance;
  size_t n = (size_t)(io2_w - p);
  if ((size_t)(length) > n) {
    length = (uint32_t)(n);
  } else {
    n = (size_t)(length);
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
    *p++ = *q++;
    *p++ = *q++;
    *p++ = *q++;
  }
  for (; n; n--) {
    *p++ = *q++;
  }
  *ptr_iop_w = p;
  return length;
}

// wuffs_base__io_writer__copy_n_from_history_fast is like the
// wuffs_base__io_writer__copy_n_from_history function above, but has stronger
// pre-conditions. The caller needs to prove that:
//  - distance >  0
//  - distance <= (*ptr_iop_w - io1_w)
//  - length   <= (io2_w      - *ptr_iop_w)
static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_history_fast(uint8_t** ptr_iop_w,
                                                uint8_t* io1_w,
                                                uint8_t* io2_w,
                                                uint32_t length,
                                                uint32_t distance) {
  uint8_t* p = *ptr_iop_w;
  uint8_t* q = p - distance;
  uint32_t n = length;
  for (; n >= 3; n -= 3) {
    *p++ = *q++;
    *p++ = *q++;
    *p++ = *q++;
  }
  for (; n; n--) {
    *p++ = *q++;
  }
  *ptr_iop_w = p;
  return length;
}

static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_reader(uint8_t** ptr_iop_w,
                                          uint8_t* io2_w,
                                          uint32_t length,
                                          uint8_t** ptr_iop_r,
                                          uint8_t* io2_r) {
  uint8_t* iop_w = *ptr_iop_w;
  size_t n = length;
  if (n > ((size_t)(io2_w - iop_w))) {
    n = (size_t)(io2_w - iop_w);
  }
  uint8_t* iop_r = *ptr_iop_r;
  if (n > ((size_t)(io2_r - iop_r))) {
    n = (size_t)(io2_r - iop_r);
  }
  if (n > 0) {
    memmove(iop_w, iop_r, n);
    *ptr_iop_w += n;
    *ptr_iop_r += n;
  }
  return (uint32_t)(n);
}

static inline uint64_t  //
wuffs_base__io_writer__copy_from_slice(uint8_t** ptr_iop_w,
                                       uint8_t* io2_w,
                                       wuffs_base__slice_u8 src) {
  uint8_t* iop_w = *ptr_iop_w;
  size_t n = src.len;
  if (n > ((size_t)(io2_w - iop_w))) {
    n = (size_t)(io2_w - iop_w);
  }
  if (n > 0) {
    memmove(iop_w, src.ptr, n);
    *ptr_iop_w += n;
  }
  return (uint64_t)(n);
}

static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_slice(uint8_t** ptr_iop_w,
                                         uint8_t* io2_w,
                                         uint32_t length,
                                         wuffs_base__slice_u8 src) {
  uint8_t* iop_w = *ptr_iop_w;
  size_t n = src.len;
  if (n > length) {
    n = length;
  }
  if (n > ((size_t)(io2_w - iop_w))) {
    n = (size_t)(io2_w - iop_w);
  }
  if (n > 0) {
    memmove(iop_w, src.ptr, n);
    *ptr_iop_w += n;
  }
  return (uint32_t)(n);
}

static inline wuffs_base__io_buffer*  //
wuffs_base__io_reader__set(wuffs_base__io_buffer* b,
                           uint8_t** ptr_iop_r,
                           uint8_t** ptr_io0_r,
                           uint8_t** ptr_io1_r,
                           uint8_t** ptr_io2_r,
                           wuffs_base__slice_u8 data) {
  b->data = data;
  b->meta.wi = data.len;
  b->meta.ri = 0;
  b->meta.pos = 0;
  b->meta.closed = false;

  *ptr_iop_r = data.ptr;
  *ptr_io0_r = data.ptr;
  *ptr_io1_r = data.ptr;
  *ptr_io2_r = data.ptr + data.len;

  return b;
}

static inline wuffs_base__slice_u8  //
wuffs_base__io_reader__take(uint8_t** ptr_iop_r, uint8_t* io2_r, uint64_t n) {
  if (n <= ((size_t)(io2_r - *ptr_iop_r))) {
    uint8_t* p = *ptr_iop_r;
    *ptr_iop_r += n;
    return wuffs_base__make_slice_u8(p, n);
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

static inline wuffs_base__io_buffer*  //
wuffs_base__io_writer__set(wuffs_base__io_buffer* b,
                           uint8_t** ptr_iop_w,
                           uint8_t** ptr_io0_w,
                           uint8_t** ptr_io1_w,
                           uint8_t** ptr_io2_w,
                           wuffs_base__slice_u8 data) {
  b->data = data;
  b->meta.wi = 0;
  b->meta.ri = 0;
  b->meta.pos = 0;
  b->meta.closed = false;

  *ptr_iop_w = data.ptr;
  *ptr_io0_w = data.ptr;
  *ptr_io1_w = data.ptr;
  *ptr_io2_w = data.ptr + data.len;

  return b;
}

// ---------------- I/O (Utility)

#define wuffs_base__utility__empty_io_reader wuffs_base__empty_io_reader
#define wuffs_base__utility__empty_io_writer wuffs_base__empty_io_writer
