// After editing this file, run "go generate" in the ../data directory.

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
//
// See (/doc/note/io-input-output.md).

// wuffs_base__io_buffer_meta is the metadata for a wuffs_base__io_buffer's
// data.
typedef struct {
  size_t wi;     // Write index. Invariant: wi <= len.
  size_t ri;     // Read  index. Invariant: ri <= wi.
  uint64_t pos;  // Buffer position (relative to the start of stream).
  bool closed;   // No further writes are expected.
} wuffs_base__io_buffer_meta;

// wuffs_base__io_buffer is a 1-dimensional buffer (a pointer and length) plus
// additional metadata.
//
// A value with all fields zero is a valid, empty buffer.
typedef struct {
  wuffs_base__slice_u8 data;
  wuffs_base__io_buffer_meta meta;

#ifdef __cplusplus
  inline bool is_valid() const;
  inline void compact();
  inline size_t reader_length() const;
  inline uint8_t* reader_pointer() const;
  inline uint64_t reader_position() const;
  inline wuffs_base__slice_u8 reader_slice() const;
  inline size_t writer_length() const;
  inline uint8_t* writer_pointer() const;
  inline uint64_t writer_position() const;
  inline wuffs_base__slice_u8 writer_slice() const;

  // Deprecated: use reader_position.
  inline uint64_t reader_io_position() const;
  // Deprecated: use writer_position.
  inline uint64_t writer_io_position() const;
#endif  // __cplusplus

} wuffs_base__io_buffer;

static inline wuffs_base__io_buffer  //
wuffs_base__make_io_buffer(wuffs_base__slice_u8 data,
                           wuffs_base__io_buffer_meta meta) {
  wuffs_base__io_buffer ret;
  ret.data = data;
  ret.meta = meta;
  return ret;
}

static inline wuffs_base__io_buffer_meta  //
wuffs_base__make_io_buffer_meta(size_t wi,
                                size_t ri,
                                uint64_t pos,
                                bool closed) {
  wuffs_base__io_buffer_meta ret;
  ret.wi = wi;
  ret.ri = ri;
  ret.pos = pos;
  ret.closed = closed;
  return ret;
}

static inline wuffs_base__io_buffer  //
wuffs_base__ptr_u8__reader(uint8_t* ptr, size_t len, bool closed) {
  wuffs_base__io_buffer ret;
  ret.data.ptr = ptr;
  ret.data.len = len;
  ret.meta.wi = len;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = closed;
  return ret;
}

static inline wuffs_base__io_buffer  //
wuffs_base__ptr_u8__writer(uint8_t* ptr, size_t len) {
  wuffs_base__io_buffer ret;
  ret.data.ptr = ptr;
  ret.data.len = len;
  ret.meta.wi = 0;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = false;
  return ret;
}

static inline wuffs_base__io_buffer  //
wuffs_base__slice_u8__reader(wuffs_base__slice_u8 s, bool closed) {
  wuffs_base__io_buffer ret;
  ret.data.ptr = s.ptr;
  ret.data.len = s.len;
  ret.meta.wi = s.len;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = closed;
  return ret;
}

static inline wuffs_base__io_buffer  //
wuffs_base__slice_u8__writer(wuffs_base__slice_u8 s) {
  wuffs_base__io_buffer ret;
  ret.data.ptr = s.ptr;
  ret.data.len = s.len;
  ret.meta.wi = 0;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = false;
  return ret;
}

static inline wuffs_base__io_buffer  //
wuffs_base__empty_io_buffer() {
  wuffs_base__io_buffer ret;
  ret.data.ptr = NULL;
  ret.data.len = 0;
  ret.meta.wi = 0;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = false;
  return ret;
}

static inline wuffs_base__io_buffer_meta  //
wuffs_base__empty_io_buffer_meta() {
  wuffs_base__io_buffer_meta ret;
  ret.wi = 0;
  ret.ri = 0;
  ret.pos = 0;
  ret.closed = false;
  return ret;
}

static inline bool  //
wuffs_base__io_buffer__is_valid(const wuffs_base__io_buffer* buf) {
  if (buf) {
    if (buf->data.ptr) {
      return (buf->meta.ri <= buf->meta.wi) && (buf->meta.wi <= buf->data.len);
    } else {
      return (buf->meta.ri == 0) && (buf->meta.wi == 0) && (buf->data.len == 0);
    }
  }
  return false;
}

// wuffs_base__io_buffer__compact moves any written but unread bytes to the
// start of the buffer.
static inline void  //
wuffs_base__io_buffer__compact(wuffs_base__io_buffer* buf) {
  if (!buf || (buf->meta.ri == 0)) {
    return;
  }
  buf->meta.pos = wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.ri);
  size_t n = buf->meta.wi - buf->meta.ri;
  if (n != 0) {
    memmove(buf->data.ptr, buf->data.ptr + buf->meta.ri, n);
  }
  buf->meta.wi = n;
  buf->meta.ri = 0;
}

// Deprecated. Use wuffs_base__io_buffer__reader_position.
static inline uint64_t  //
wuffs_base__io_buffer__reader_io_position(const wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.ri) : 0;
}

static inline size_t  //
wuffs_base__io_buffer__reader_length(const wuffs_base__io_buffer* buf) {
  return buf ? buf->meta.wi - buf->meta.ri : 0;
}

static inline uint8_t*  //
wuffs_base__io_buffer__reader_pointer(const wuffs_base__io_buffer* buf) {
  return buf ? (buf->data.ptr + buf->meta.ri) : NULL;
}

static inline uint64_t  //
wuffs_base__io_buffer__reader_position(const wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.ri) : 0;
}

static inline wuffs_base__slice_u8  //
wuffs_base__io_buffer__reader_slice(const wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__make_slice_u8(buf->data.ptr + buf->meta.ri,
                                         buf->meta.wi - buf->meta.ri)
             : wuffs_base__empty_slice_u8();
}

// Deprecated. Use wuffs_base__io_buffer__writer_position.
static inline uint64_t  //
wuffs_base__io_buffer__writer_io_position(const wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.wi) : 0;
}

static inline size_t  //
wuffs_base__io_buffer__writer_length(const wuffs_base__io_buffer* buf) {
  return buf ? buf->data.len - buf->meta.wi : 0;
}

static inline uint8_t*  //
wuffs_base__io_buffer__writer_pointer(const wuffs_base__io_buffer* buf) {
  return buf ? (buf->data.ptr + buf->meta.wi) : NULL;
}

static inline uint64_t  //
wuffs_base__io_buffer__writer_position(const wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.wi) : 0;
}

static inline wuffs_base__slice_u8  //
wuffs_base__io_buffer__writer_slice(const wuffs_base__io_buffer* buf) {
  return buf ? wuffs_base__make_slice_u8(buf->data.ptr + buf->meta.wi,
                                         buf->data.len - buf->meta.wi)
             : wuffs_base__empty_slice_u8();
}

#ifdef __cplusplus

inline bool  //
wuffs_base__io_buffer::is_valid() const {
  return wuffs_base__io_buffer__is_valid(this);
}

inline void  //
wuffs_base__io_buffer::compact() {
  wuffs_base__io_buffer__compact(this);
}

inline uint64_t  //
wuffs_base__io_buffer::reader_io_position() const {
  return wuffs_base__io_buffer__reader_io_position(this);
}

inline size_t  //
wuffs_base__io_buffer::reader_length() const {
  return wuffs_base__io_buffer__reader_length(this);
}

inline uint8_t*  //
wuffs_base__io_buffer::reader_pointer() const {
  return wuffs_base__io_buffer__reader_pointer(this);
}

inline uint64_t  //
wuffs_base__io_buffer::reader_position() const {
  return wuffs_base__io_buffer__reader_position(this);
}

inline wuffs_base__slice_u8  //
wuffs_base__io_buffer::reader_slice() const {
  return wuffs_base__io_buffer__reader_slice(this);
}

inline uint64_t  //
wuffs_base__io_buffer::writer_io_position() const {
  return wuffs_base__io_buffer__writer_io_position(this);
}

inline size_t  //
wuffs_base__io_buffer::writer_length() const {
  return wuffs_base__io_buffer__writer_length(this);
}

inline uint8_t*  //
wuffs_base__io_buffer::writer_pointer() const {
  return wuffs_base__io_buffer__writer_pointer(this);
}

inline uint64_t  //
wuffs_base__io_buffer::writer_position() const {
  return wuffs_base__io_buffer__writer_position(this);
}

inline wuffs_base__slice_u8  //
wuffs_base__io_buffer::writer_slice() const {
  return wuffs_base__io_buffer__writer_slice(this);
}

#endif  // __cplusplus
