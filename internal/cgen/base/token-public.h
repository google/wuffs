// After editing this file, run "go generate" in the parent directory.

// Copyright 2020 The Wuffs Authors.
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

// ---------------- Tokens

typedef struct {
  // The repr is divided as:
  //  - Bits 63 .. 40 (24 bits) is the major value.
  //  - Bits 39 .. 16 (24 bits) is the minor value.
  //  - Bits 15 ..  0 (16 bits) is the length.
  //
  // The major value is a [Base38](doc/note/base38-and-fourcc.md) value. If
  // zero (special cased for Wuffs' built-in "base" package) then the minor
  // value is further sub-divided:
  //  - Bits 39 .. 37 ( 3 bits) is the value_base_category.
  //  - Bits 36 .. 16 (21 bits) is the value_base_detail.
  //
  // In particular, at 21 bits, the value_base_detail can hold every valid
  // Unicode code point.
  //
  // If the major value is non-zero then the minor value has whatever arbitrary
  // meaning the tokenizer's package assigns to it.
  uint64_t repr;

#ifdef __cplusplus
  inline uint64_t value() const;
  inline uint64_t value_major() const;
  inline uint64_t value_minor() const;
  inline uint64_t value_base_category() const;
  inline uint64_t value_base_detail() const;
  inline uint64_t length() const;
#endif  // __cplusplus

} wuffs_base__token;

static inline wuffs_base__token  //
wuffs_base__make_token(uint64_t repr) {
  wuffs_base__token ret;
  ret.repr = repr;
  return ret;
}

#define WUFFS_BASE__TOKEN__VALUE__MASK 0xFFFFFFFFFFFF
#define WUFFS_BASE__TOKEN__VALUE_MAJOR__MASK 0xFFFFFF
#define WUFFS_BASE__TOKEN__VALUE_MINOR__MASK 0xFFFFFF
#define WUFFS_BASE__TOKEN__VALUE_BASE_CATEGORY__MASK 0x7FFFFFF
#define WUFFS_BASE__TOKEN__VALUE_BASE_DETAIL__MASK 0x1FFFFF
#define WUFFS_BASE__TOKEN__LENGTH__MASK 0xFFFF

#define WUFFS_BASE__TOKEN__VALUE__SHIFT 16
#define WUFFS_BASE__TOKEN__VALUE_MAJOR__SHIFT 40
#define WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT 16
#define WUFFS_BASE__TOKEN__VALUE_BASE_CATEGORY__SHIFT 37
#define WUFFS_BASE__TOKEN__VALUE_BASE_DETAIL__SHIFT 16
#define WUFFS_BASE__TOKEN__LENGTH__SHIFT 0

#define WUFFS_BASE__TOKEN__VBC__FILLER 0
#define WUFFS_BASE__TOKEN__VBC__STRING 1
#define WUFFS_BASE__TOKEN__VBC__BYTES 2
#define WUFFS_BASE__TOKEN__VBC__STRUCTURE 3
#define WUFFS_BASE__TOKEN__VBC__NUMBER 4
#define WUFFS_BASE__TOKEN__VBC__UNICODE_CODE_POINT 5

// Bits 0x2, 0x4 and 0x8 are reserved for flags that are common between
// VBD_FILLER, VBD_STRING and VBD_BYTES.
#define WUFFS_BASE__TOKEN__VBD__FILLER__INCOMPLETE 0x00001
#define WUFFS_BASE__TOKEN__VBD__FILLER__END_OF_CONSECUTIVE_COMMENTS 0x00010
#define WUFFS_BASE__TOKEN__VBD__FILLER__COMMENT_LINE 0x00020
#define WUFFS_BASE__TOKEN__VBD__FILLER__COMMENT_BLOCK 0x00040

// Bits 0x2, 0x4 and 0x8 are reserved for flags that are common between
// VBD_FILLER, VBD_STRING and VBD_BYTES.
#define WUFFS_BASE__TOKEN__VBD__STRING__INCOMPLETE 0x00001
#define WUFFS_BASE__TOKEN__VBD__STRING__ALL_ASCII 0x00010

// Bits 0x2, 0x4 and 0x8 are reserved for flags that are common between
// VBD_FILLER, VBD_STRING and VBD_BYTES.
#define WUFFS_BASE__TOKEN__VBD__BYTES__INCOMPLETE 0x00001
// "D_DST_S_SRC" means that it takes S source bytes (possibly padded) to
// produce D destination bytes. For example,
// WUFFS_BASE__TOKEN__VBD__BYTES__1_DST_4_SRC_BACKSLASH_X means that the source
// looks like "\\x23\\x67\\xAB", where 12 src bytes encode 3 dst bytes.
#define WUFFS_BASE__TOKEN__VBD__BYTES__1_DST_1_SRC_RAW 0x00010
#define WUFFS_BASE__TOKEN__VBD__BYTES__1_DST_2_SRC_HEX 0x00020
#define WUFFS_BASE__TOKEN__VBD__BYTES__1_DST_4_SRC_BACKSLASH_X 0x00040
#define WUFFS_BASE__TOKEN__VBD__BYTES__1_DST_6_SRC_BACKSLASH_U 0x00080
#define WUFFS_BASE__TOKEN__VBD__BYTES__3_DST_4_SRC_BASE_64_STD 0x00100
#define WUFFS_BASE__TOKEN__VBD__BYTES__3_DST_4_SRC_BASE_64_URL 0x00200
#define WUFFS_BASE__TOKEN__VBD__BYTES__4_DST_5_SRC_ASCII_85 0x00400

#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__PUSH 0x00001
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__POP 0x00002
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_NONE 0x00010
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_LIST 0x00020
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_DICT 0x00040
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_NONE 0x01000
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST 0x02000
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_DICT 0x04000

#define WUFFS_BASE__TOKEN__VBD__NUMBER__LITERAL 0x00001
#define WUFFS_BASE__TOKEN__VBD__NUMBER__LITERAL__UNDEFINED 0x00101
#define WUFFS_BASE__TOKEN__VBD__NUMBER__LITERAL__NULL 0x00201
#define WUFFS_BASE__TOKEN__VBD__NUMBER__LITERAL__FALSE 0x00401
#define WUFFS_BASE__TOKEN__VBD__NUMBER__LITERAL__TRUE 0x00801
// For a source string of "123" or "0x9A", it is valid for a tokenizer to
// return any one of:
//  - WUFFS_BASE__TOKEN__VBD__NUMBER__FLOATING_POINT.
//  - WUFFS_BASE__TOKEN__VBD__NUMBER__INTEGER_SIGNED.
//  - WUFFS_BASE__TOKEN__VBD__NUMBER__INTEGER_UNSIGNED.
//
// For a source string of "+123" or "-0x9A", only the first two are valid.
//
// For a source string of "123.", only the first one is valid.
#define WUFFS_BASE__TOKEN__VBD__NUMBER__FLOATING_POINT 0x00002
#define WUFFS_BASE__TOKEN__VBD__NUMBER__INTEGER_SIGNED 0x00004
#define WUFFS_BASE__TOKEN__VBD__NUMBER__INTEGER_UNSIGNED 0x00008

#define WUFFS_BASE__TOKEN__VBD__UNICODE_CODE_POINT__MAX_INCL 0x10FFFF

static inline uint64_t  //
wuffs_base__token__value(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__VALUE__SHIFT) &
         WUFFS_BASE__TOKEN__VALUE__MASK;
}

static inline uint64_t  //
wuffs_base__token__value_major(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__VALUE_MAJOR__SHIFT) &
         WUFFS_BASE__TOKEN__VALUE_MAJOR__MASK;
}

static inline uint64_t  //
wuffs_base__token__value_minor(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) &
         WUFFS_BASE__TOKEN__VALUE_MINOR__MASK;
}

static inline uint64_t  //
wuffs_base__token__value_base_category(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__VALUE_BASE_CATEGORY__SHIFT) &
         WUFFS_BASE__TOKEN__VALUE_BASE_CATEGORY__MASK;
}

static inline uint64_t  //
wuffs_base__token__value_base_detail(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__VALUE_BASE_DETAIL__SHIFT) &
         WUFFS_BASE__TOKEN__VALUE_BASE_DETAIL__MASK;
}

static inline uint64_t  //
wuffs_base__token__length(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__LENGTH__SHIFT) &
         WUFFS_BASE__TOKEN__LENGTH__MASK;
}

#ifdef __cplusplus

inline uint64_t  //
wuffs_base__token::value() const {
  return wuffs_base__token__value(this);
}

inline uint64_t  //
wuffs_base__token::value_major() const {
  return wuffs_base__token__value_major(this);
}

inline uint64_t  //
wuffs_base__token::value_minor() const {
  return wuffs_base__token__value_minor(this);
}

inline uint64_t  //
wuffs_base__token::value_base_category() const {
  return wuffs_base__token__value_base_category(this);
}

inline uint64_t  //
wuffs_base__token::value_base_detail() const {
  return wuffs_base__token__value_base_detail(this);
}

inline uint64_t  //
wuffs_base__token::length() const {
  return wuffs_base__token__length(this);
}

#endif  // __cplusplus

// --------

typedef WUFFS_BASE__SLICE(wuffs_base__token) wuffs_base__slice_token;

static inline wuffs_base__slice_token  //
wuffs_base__make_slice_token(wuffs_base__token* ptr, size_t len) {
  wuffs_base__slice_token ret;
  ret.ptr = ptr;
  ret.len = len;
  return ret;
}

// --------

// wuffs_base__token_buffer_meta is the metadata for a
// wuffs_base__token_buffer's data.
typedef struct {
  size_t wi;     // Write index. Invariant: wi <= len.
  size_t ri;     // Read  index. Invariant: ri <= wi.
  uint64_t pos;  // Position of the buffer start relative to the stream start.
  bool closed;   // No further writes are expected.
} wuffs_base__token_buffer_meta;

// wuffs_base__token_buffer is a 1-dimensional buffer (a pointer and length)
// plus additional metadata.
//
// A value with all fields zero is a valid, empty buffer.
typedef struct {
  wuffs_base__slice_token data;
  wuffs_base__token_buffer_meta meta;

#ifdef __cplusplus
  inline void compact();
  inline uint64_t reader_available() const;
  inline uint64_t reader_token_position() const;
  inline uint64_t writer_available() const;
  inline uint64_t writer_token_position() const;
#endif  // __cplusplus

} wuffs_base__token_buffer;

static inline wuffs_base__token_buffer  //
wuffs_base__make_token_buffer(wuffs_base__slice_token data,
                              wuffs_base__token_buffer_meta meta) {
  wuffs_base__token_buffer ret;
  ret.data = data;
  ret.meta = meta;
  return ret;
}

static inline wuffs_base__token_buffer_meta  //
wuffs_base__make_token_buffer_meta(size_t wi,
                                   size_t ri,
                                   uint64_t pos,
                                   bool closed) {
  wuffs_base__token_buffer_meta ret;
  ret.wi = wi;
  ret.ri = ri;
  ret.pos = pos;
  ret.closed = closed;
  return ret;
}

static inline wuffs_base__token_buffer  //
wuffs_base__empty_token_buffer() {
  wuffs_base__token_buffer ret;
  ret.data.ptr = NULL;
  ret.data.len = 0;
  ret.meta.wi = 0;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = false;
  return ret;
}

static inline wuffs_base__token_buffer_meta  //
wuffs_base__empty_token_buffer_meta() {
  wuffs_base__token_buffer_meta ret;
  ret.wi = 0;
  ret.ri = 0;
  ret.pos = 0;
  ret.closed = false;
  return ret;
}

// wuffs_base__token_buffer__compact moves any written but unread tokens to the
// start of the buffer.
static inline void  //
wuffs_base__token_buffer__compact(wuffs_base__token_buffer* buf) {
  if (!buf || (buf->meta.ri == 0)) {
    return;
  }
  buf->meta.pos = wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.ri);
  size_t n = buf->meta.wi - buf->meta.ri;
  if (n != 0) {
    memmove(buf->data.ptr, buf->data.ptr + buf->meta.ri,
            n * sizeof(wuffs_base__token));
  }
  buf->meta.wi = n;
  buf->meta.ri = 0;
}

static inline uint64_t  //
wuffs_base__token_buffer__reader_available(
    const wuffs_base__token_buffer* buf) {
  return buf ? buf->meta.wi - buf->meta.ri : 0;
}

static inline uint64_t  //
wuffs_base__token_buffer__reader_token_position(
    const wuffs_base__token_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.ri) : 0;
}

static inline uint64_t  //
wuffs_base__token_buffer__writer_available(
    const wuffs_base__token_buffer* buf) {
  return buf ? buf->data.len - buf->meta.wi : 0;
}

static inline uint64_t  //
wuffs_base__token_buffer__writer_token_position(
    const wuffs_base__token_buffer* buf) {
  return buf ? wuffs_base__u64__sat_add(buf->meta.pos, buf->meta.wi) : 0;
}

#ifdef __cplusplus

inline void  //
wuffs_base__token_buffer::compact() {
  wuffs_base__token_buffer__compact(this);
}

inline uint64_t  //
wuffs_base__token_buffer::reader_available() const {
  return wuffs_base__token_buffer__reader_available(this);
}

inline uint64_t  //
wuffs_base__token_buffer::reader_token_position() const {
  return wuffs_base__token_buffer__reader_token_position(this);
}

inline uint64_t  //
wuffs_base__token_buffer::writer_available() const {
  return wuffs_base__token_buffer__writer_available(this);
}

inline uint64_t  //
wuffs_base__token_buffer::writer_token_position() const {
  return wuffs_base__token_buffer__writer_token_position(this);
}

#endif  // __cplusplus