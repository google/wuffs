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

static inline bool  //
wuffs_base__io_buffer__is_valid(wuffs_base__io_buffer buf) {
  return (buf.data.ptr || (buf.data.len == 0)) &&
         (buf.data.len >= buf.meta.wi) && (buf.meta.wi >= buf.meta.ri);
}

// TODO: wuffs_base__io_reader__is_eof is no longer used by Wuffs per se, but
// it might be handy to programs that use Wuffs. Either delete it, or promote
// it to the public API.
//
// If making this function public (i.e. moving it to base-header.h), it also
// needs to allow NULL (i.e. implicit, callee-calculated) mark/limit.

static inline bool  //
wuffs_base__io_reader__is_eof(wuffs_base__io_reader o) {
  wuffs_base__io_buffer* buf = o.private_impl.buf;
  return buf && buf->meta.closed &&
         (buf->data.ptr + buf->meta.wi == o.private_impl.limit);
}

static inline bool  //
wuffs_base__io_reader__is_valid(wuffs_base__io_reader o) {
  wuffs_base__io_buffer* buf = o.private_impl.buf;
  // Note: if making this function public (i.e. moving it to base-header.h), it
  // also needs to allow NULL (i.e. implicit, callee-calculated) mark/limit.
  return buf ? ((buf->data.ptr <= o.private_impl.mark) &&
                (o.private_impl.mark <= o.private_impl.limit) &&
                (o.private_impl.limit <= buf->data.ptr + buf->data.len))
             : ((o.private_impl.mark == NULL) &&
                (o.private_impl.limit == NULL));
}

static inline bool  //
wuffs_base__io_writer__is_valid(wuffs_base__io_writer o) {
  wuffs_base__io_buffer* buf = o.private_impl.buf;
  // Note: if making this function public (i.e. moving it to base-header.h), it
  // also needs to allow NULL (i.e. implicit, callee-calculated) mark/limit.
  return buf ? ((buf->data.ptr <= o.private_impl.mark) &&
                (o.private_impl.mark <= o.private_impl.limit) &&
                (o.private_impl.limit <= buf->data.ptr + buf->data.len))
             : ((o.private_impl.mark == NULL) &&
                (o.private_impl.limit == NULL));
}

static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_history(uint8_t** ptr_ptr,
                                           uint8_t* start,
                                           uint8_t* end,
                                           uint32_t length,
                                           uint32_t distance) {
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
  // copy_n_from_history Wuffs method should also take an unroll hint argument,
  // and the cgen can look if that argument is the constant expression '3'.
  //
  // See also wuffs_base__io_writer__copy_n_from_history_fast below.
  //
  // Alternatively, or additionally, have a sloppy_copy_n_from_history method
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

// wuffs_base__io_writer__copy_n_from_history_fast is like the
// wuffs_base__io_writer__copy_n_from_history function above, but has stronger
// pre-conditions. The caller needs to prove that:
//  - distance >  0
//  - distance <= (*ptr_ptr - start)
//  - length   <= (end      - *ptr_ptr)
static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_history_fast(uint8_t** ptr_ptr,
                                                uint8_t* start,
                                                uint8_t* end,
                                                uint32_t length,
                                                uint32_t distance) {
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

static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_reader(uint8_t** ptr_ioptr_w,
                                          uint8_t* iobounds1_w,
                                          uint32_t length,
                                          uint8_t** ptr_ioptr_r,
                                          uint8_t* iobounds1_r) {
  uint8_t* ioptr_w = *ptr_ioptr_w;
  size_t n = length;
  if (n > ((size_t)(iobounds1_w - ioptr_w))) {
    n = iobounds1_w - ioptr_w;
  }
  uint8_t* ioptr_r = *ptr_ioptr_r;
  if (n > ((size_t)(iobounds1_r - ioptr_r))) {
    n = iobounds1_r - ioptr_r;
  }
  if (n > 0) {
    memmove(ioptr_w, ioptr_r, n);
    *ptr_ioptr_w += n;
    *ptr_ioptr_r += n;
  }
  return n;
}

static inline uint64_t  //
wuffs_base__io_writer__copy_from_slice(uint8_t** ptr_ioptr_w,
                                       uint8_t* iobounds1_w,
                                       wuffs_base__slice_u8 src) {
  uint8_t* ioptr_w = *ptr_ioptr_w;
  size_t n = src.len;
  if (n > ((size_t)(iobounds1_w - ioptr_w))) {
    n = iobounds1_w - ioptr_w;
  }
  if (n > 0) {
    memmove(ioptr_w, src.ptr, n);
    *ptr_ioptr_w += n;
  }
  return n;
}

static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_slice(uint8_t** ptr_ioptr_w,
                                         uint8_t* iobounds1_w,
                                         uint32_t length,
                                         wuffs_base__slice_u8 src) {
  uint8_t* ioptr_w = *ptr_ioptr_w;
  size_t n = src.len;
  if (n > length) {
    n = length;
  }
  if (n > ((size_t)(iobounds1_w - ioptr_w))) {
    n = iobounds1_w - ioptr_w;
  }
  if (n > 0) {
    memmove(ioptr_w, src.ptr, n);
    *ptr_ioptr_w += n;
  }
  return n;
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_reader__set(wuffs_base__io_reader* o,
                           wuffs_base__io_buffer* b,
                           uint8_t** ptr_iop,
                           uint8_t** ptr_io1,
                           wuffs_base__slice_u8 data) {
  b->data = data;
  b->meta.wi = data.len;
  b->meta.ri = 0;
  b->meta.pos = 0;
  b->meta.closed = false;

  o->private_impl.buf = b;
  o->private_impl.mark = data.ptr;
  o->private_impl.limit = data.ptr + data.len;
  *ptr_iop = data.ptr;
  *ptr_io1 = data.ptr + data.len;
  return ((wuffs_base__empty_struct){});
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_reader__set_limit(wuffs_base__io_reader* o,
                                 uint8_t* ioptr_r,
                                 uint64_t limit) {
  if (o && (((size_t)(o->private_impl.limit - ioptr_r)) > limit)) {
    o->private_impl.limit = ioptr_r + limit;
  }
  return ((wuffs_base__empty_struct){});
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_reader__set_mark(wuffs_base__io_reader* o, uint8_t* mark) {
  o->private_impl.mark = mark;
  return ((wuffs_base__empty_struct){});
}

static inline wuffs_base__slice_u8  //
wuffs_base__io_reader__take(uint8_t** ptr_iop, uint8_t* io1, uint64_t n) {
  if (n <= ((size_t)(io1 - *ptr_iop))) {
    uint8_t* p = *ptr_iop;
    *ptr_iop += n;
    return ((wuffs_base__slice_u8){
        .ptr = p,
        .len = n,
    });
  }
  return ((wuffs_base__slice_u8){});
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_writer__set(wuffs_base__io_writer* o,
                           wuffs_base__io_buffer* b,
                           uint8_t** ioptr1_ptr,
                           uint8_t** ioptr2_ptr,
                           wuffs_base__slice_u8 data) {
  b->data = data;
  b->meta.wi = 0;
  b->meta.ri = 0;
  b->meta.pos = 0;
  b->meta.closed = false;

  o->private_impl.buf = b;
  o->private_impl.mark = data.ptr;
  o->private_impl.limit = data.ptr + data.len;
  *ioptr1_ptr = data.ptr;
  *ioptr2_ptr = data.ptr + data.len;
  return ((wuffs_base__empty_struct){});
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_writer__set_mark(wuffs_base__io_writer* o, uint8_t* mark) {
  o->private_impl.mark = mark;
  return ((wuffs_base__empty_struct){});
}
