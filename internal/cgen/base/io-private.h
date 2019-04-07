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
wuffs_base__io_writer__copy_n_from_history(uint8_t** ptr_iop_w,
                                           uint8_t* io0_w,
                                           uint8_t* io1_w,
                                           uint32_t length,
                                           uint32_t distance) {
  if (!distance) {
    return 0;
  }
  uint8_t* p = *ptr_iop_w;
  if ((size_t)(p - io0_w) < (size_t)(distance)) {
    return 0;
  }
  uint8_t* q = p - distance;
  size_t n = (size_t)(io1_w - p);
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
//  - distance <= (*ptr_iop_w - io0_w)
//  - length   <= (io1_w      - *ptr_iop_w)
static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_history_fast(uint8_t** ptr_iop_w,
                                                uint8_t* io0_w,
                                                uint8_t* io1_w,
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
                                          uint8_t* io1_w,
                                          uint32_t length,
                                          uint8_t** ptr_iop_r,
                                          uint8_t* io1_r) {
  uint8_t* iop_w = *ptr_iop_w;
  size_t n = length;
  if (n > ((size_t)(io1_w - iop_w))) {
    n = (size_t)(io1_w - iop_w);
  }
  uint8_t* iop_r = *ptr_iop_r;
  if (n > ((size_t)(io1_r - iop_r))) {
    n = (size_t)(io1_r - iop_r);
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
                                       uint8_t* io1_w,
                                       wuffs_base__slice_u8 src) {
  uint8_t* iop_w = *ptr_iop_w;
  size_t n = src.len;
  if (n > ((size_t)(io1_w - iop_w))) {
    n = (size_t)(io1_w - iop_w);
  }
  if (n > 0) {
    memmove(iop_w, src.ptr, n);
    *ptr_iop_w += n;
  }
  return (uint64_t)(n);
}

static inline uint32_t  //
wuffs_base__io_writer__copy_n_from_slice(uint8_t** ptr_iop_w,
                                         uint8_t* io1_w,
                                         uint32_t length,
                                         wuffs_base__slice_u8 src) {
  uint8_t* iop_w = *ptr_iop_w;
  size_t n = src.len;
  if (n > length) {
    n = length;
  }
  if (n > ((size_t)(io1_w - iop_w))) {
    n = (size_t)(io1_w - iop_w);
  }
  if (n > 0) {
    memmove(iop_w, src.ptr, n);
    *ptr_iop_w += n;
  }
  return (uint32_t)(n);
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_reader__set(wuffs_base__io_reader* o,
                           wuffs_base__io_buffer* b,
                           uint8_t** ptr_iop_r,
                           uint8_t** ptr_io1_r,
                           wuffs_base__slice_u8 data) {
  b->data = data;
  b->meta.wi = data.len;
  b->meta.ri = 0;
  b->meta.pos = 0;
  b->meta.closed = false;

  o->private_impl.buf = b;
  o->private_impl.mark = data.ptr;
  o->private_impl.limit = data.ptr + data.len;
  *ptr_iop_r = data.ptr;
  *ptr_io1_r = data.ptr + data.len;

  wuffs_base__empty_struct ret;
  ret.private_impl = 0;
  return ret;
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_reader__set_limit(wuffs_base__io_reader* o,
                                 uint8_t* iop_r,
                                 uint64_t limit) {
  if (o && (((size_t)(o->private_impl.limit - iop_r)) > limit)) {
    o->private_impl.limit = iop_r + limit;
  }

  wuffs_base__empty_struct ret;
  ret.private_impl = 0;
  return ret;
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_reader__set_mark(wuffs_base__io_reader* o, uint8_t* mark) {
  o->private_impl.mark = mark;

  wuffs_base__empty_struct ret;
  ret.private_impl = 0;
  return ret;
}

static inline wuffs_base__slice_u8  //
wuffs_base__io_reader__take(uint8_t** ptr_iop_r, uint8_t* io1_r, uint64_t n) {
  if (n <= ((size_t)(io1_r - *ptr_iop_r))) {
    uint8_t* p = *ptr_iop_r;
    *ptr_iop_r += n;
    return wuffs_base__make_slice_u8(p, n);
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_writer__set(wuffs_base__io_writer* o,
                           wuffs_base__io_buffer* b,
                           uint8_t** ptr_iop_w,
                           uint8_t** ptr_io1_w,
                           wuffs_base__slice_u8 data) {
  b->data = data;
  b->meta.wi = 0;
  b->meta.ri = 0;
  b->meta.pos = 0;
  b->meta.closed = false;

  o->private_impl.buf = b;
  o->private_impl.mark = data.ptr;
  o->private_impl.limit = data.ptr + data.len;
  *ptr_iop_w = data.ptr;
  *ptr_io1_w = data.ptr + data.len;

  wuffs_base__empty_struct ret;
  ret.private_impl = 0;
  return ret;
}

static inline wuffs_base__empty_struct  //
wuffs_base__io_writer__set_mark(wuffs_base__io_writer* o, uint8_t* mark) {
  o->private_impl.mark = mark;

  wuffs_base__empty_struct ret;
  ret.private_impl = 0;
  return ret;
}

// ---------------- I/O (Utility)

#define wuffs_base__utility__null_io_reader wuffs_base__null_io_reader
#define wuffs_base__utility__null_io_writer wuffs_base__null_io_writer
