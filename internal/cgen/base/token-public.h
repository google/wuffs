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

// wuffs_base__token is an element of a byte stream's tokenization.
//
// See https://github.com/google/wuffs/blob/master/doc/note/tokens.md
typedef struct {
  uint64_t repr;

#ifdef __cplusplus
  inline int64_t value() const;
  inline int64_t value_extension() const;
  inline int64_t value_major() const;
  inline int64_t value_base_category() const;
  inline uint64_t value_minor() const;
  inline uint64_t value_base_detail() const;
  inline bool continued() const;
  inline uint64_t length() const;
#endif  // __cplusplus

} wuffs_base__token;

static inline wuffs_base__token  //
wuffs_base__make_token(uint64_t repr) {
  wuffs_base__token ret;
  ret.repr = repr;
  return ret;
}

// --------

#define WUFFS_BASE__TOKEN__LENGTH__MAX_INCL 0xFFFF

#define WUFFS_BASE__TOKEN__VALUE__SHIFT 17
#define WUFFS_BASE__TOKEN__VALUE_EXTENSION__SHIFT 17
#define WUFFS_BASE__TOKEN__VALUE_MAJOR__SHIFT 42
#define WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT 17
#define WUFFS_BASE__TOKEN__VALUE_BASE_CATEGORY__SHIFT 38
#define WUFFS_BASE__TOKEN__VALUE_BASE_DETAIL__SHIFT 17
#define WUFFS_BASE__TOKEN__CONTINUED__SHIFT 16
#define WUFFS_BASE__TOKEN__LENGTH__SHIFT 0

// --------

#define WUFFS_BASE__TOKEN__VBC__FILLER 0
#define WUFFS_BASE__TOKEN__VBC__STRUCTURE 1
#define WUFFS_BASE__TOKEN__VBC__STRING 2
#define WUFFS_BASE__TOKEN__VBC__UNICODE_CODE_POINT 3
#define WUFFS_BASE__TOKEN__VBC__LITERAL 4
#define WUFFS_BASE__TOKEN__VBC__NUMBER 5

// --------

#define WUFFS_BASE__TOKEN__VBD__FILLER__COMMENT_LINE 0x00001
#define WUFFS_BASE__TOKEN__VBD__FILLER__COMMENT_BLOCK 0x00002

// --------

#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__PUSH 0x00001
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__POP 0x00002
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_NONE 0x00010
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_LIST 0x00020
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_DICT 0x00040
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_NONE 0x01000
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST 0x02000
#define WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_DICT 0x04000

// --------

// "DEFINITELY_FOO" means that the destination bytes (and also the source
// bytes, for 1_DST_1_SRC_COPY) are in the FOO format. Definitely means that
// the lack of the bit is conservative: it is valid for all-ASCII strings to
// have neither DEFINITELY_UTF_8 or DEFINITELY_ASCII bits set.
#define WUFFS_BASE__TOKEN__VBD__STRING__DEFINITELY_UTF_8 0x00001
#define WUFFS_BASE__TOKEN__VBD__STRING__DEFINITELY_ASCII 0x00002

// "CONVERT_D_DST_S_SRC" means that multiples of S source bytes (possibly
// padded) produces multiples of D destination bytes. For example,
// CONVERT_1_DST_4_SRC_BACKSLASH_X means a source like "\\x23\\x67\\xAB", where
// 12 src bytes encode 3 dst bytes.
//
// Post-processing may further transform those D destination bytes (e.g. treat
// "\\xFF" as the Unicode code point U+00FF instead of the byte 0xFF), but that
// is out of scope of this VBD's semantics.
//
// When src is the empty string, multiple conversion algorithms are applicable
// (so these bits are not necessarily mutually exclusive), all producing the
// same empty dst string.
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_0_DST_1_SRC_DROP 0x00010
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_1_DST_1_SRC_COPY 0x00020
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_1_DST_2_SRC_HEXADECIMAL 0x00040
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_1_DST_4_SRC_BACKSLASH_X 0x00080
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_3_DST_4_SRC_BASE_64_STD 0x00100
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_3_DST_4_SRC_BASE_64_URL 0x00200
#define WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_4_DST_5_SRC_ASCII_85 0x00400

// --------

#define WUFFS_BASE__TOKEN__VBD__LITERAL__UNDEFINED 0x00001
#define WUFFS_BASE__TOKEN__VBD__LITERAL__NULL 0x00002
#define WUFFS_BASE__TOKEN__VBD__LITERAL__FALSE 0x00004
#define WUFFS_BASE__TOKEN__VBD__LITERAL__TRUE 0x00008

// --------

// For a source string of "123" or "0x9A", it is valid for a tokenizer to
// return any one of:
//  - WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_FLOATING_POINT.
//  - WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_INTEGER_SIGNED.
//  - WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_INTEGER_UNSIGNED.
//
// For a source string of "+123" or "-0x9A", only the first two are valid.
//
// For a source string of "123.", only the first one is valid.
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_FLOATING_POINT 0x00001
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_INTEGER_SIGNED 0x00002
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_INTEGER_UNSIGNED 0x00004

#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_NEG_INF 0x00010
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_POS_INF 0x00020
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_NEG_NAN 0x00040
#define WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_POS_NAN 0x00080

// The number 300 might be represented as "\x01\x2C", "\x2C\x01\x00\x00" or
// "300", which are big-endian, little-endian or text. For binary formats, the
// token length discriminates e.g. u16 little-endian vs u32 little-endian.
#define WUFFS_BASE__TOKEN__VBD__NUMBER__FORMAT_BINARY_BIG_ENDIAN 0x00100
#define WUFFS_BASE__TOKEN__VBD__NUMBER__FORMAT_BINARY_LITTLE_ENDIAN 0x00200
#define WUFFS_BASE__TOKEN__VBD__NUMBER__FORMAT_TEXT 0x00400

// --------

// wuffs_base__token__value returns the token's high 46 bits, sign-extended. A
// negative value means an extended token, non-negative means a simple token.
static inline int64_t  //
wuffs_base__token__value(const wuffs_base__token* t) {
  return ((int64_t)(t->repr)) >> WUFFS_BASE__TOKEN__VALUE__SHIFT;
}

// wuffs_base__token__value_extension returns a negative value if the token was
// not an extended token.
static inline int64_t  //
wuffs_base__token__value_extension(const wuffs_base__token* t) {
  return (~(int64_t)(t->repr)) >> WUFFS_BASE__TOKEN__VALUE_EXTENSION__SHIFT;
}

// wuffs_base__token__value_major returns a negative value if the token was not
// a simple token.
static inline int64_t  //
wuffs_base__token__value_major(const wuffs_base__token* t) {
  return ((int64_t)(t->repr)) >> WUFFS_BASE__TOKEN__VALUE_MAJOR__SHIFT;
}

// wuffs_base__token__value_base_category returns a negative value if the token
// was not a simple token.
static inline int64_t  //
wuffs_base__token__value_base_category(const wuffs_base__token* t) {
  return ((int64_t)(t->repr)) >> WUFFS_BASE__TOKEN__VALUE_BASE_CATEGORY__SHIFT;
}

static inline uint64_t  //
wuffs_base__token__value_minor(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__VALUE_MINOR__SHIFT) & 0x1FFFFFF;
}

static inline uint64_t  //
wuffs_base__token__value_base_detail(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__VALUE_BASE_DETAIL__SHIFT) & 0x1FFFFF;
}

static inline bool  //
wuffs_base__token__continued(const wuffs_base__token* t) {
  return t->repr & 0x10000;
}

static inline uint64_t  //
wuffs_base__token__length(const wuffs_base__token* t) {
  return (t->repr >> WUFFS_BASE__TOKEN__LENGTH__SHIFT) & 0xFFFF;
}

#ifdef __cplusplus

inline int64_t  //
wuffs_base__token::value() const {
  return wuffs_base__token__value(this);
}

inline int64_t  //
wuffs_base__token::value_extension() const {
  return wuffs_base__token__value_extension(this);
}

inline int64_t  //
wuffs_base__token::value_major() const {
  return wuffs_base__token__value_major(this);
}

inline int64_t  //
wuffs_base__token::value_base_category() const {
  return wuffs_base__token__value_base_category(this);
}

inline uint64_t  //
wuffs_base__token::value_minor() const {
  return wuffs_base__token__value_minor(this);
}

inline uint64_t  //
wuffs_base__token::value_base_detail() const {
  return wuffs_base__token__value_base_detail(this);
}

inline bool  //
wuffs_base__token::continued() const {
  return wuffs_base__token__continued(this);
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
  inline bool is_valid() const;
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
wuffs_base__slice_token__reader(wuffs_base__slice_token s, bool closed) {
  wuffs_base__token_buffer ret;
  ret.data.ptr = s.ptr;
  ret.data.len = s.len;
  ret.meta.wi = s.len;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = closed;
  return ret;
}

static inline wuffs_base__token_buffer  //
wuffs_base__slice_token__writer(wuffs_base__slice_token s) {
  wuffs_base__token_buffer ret;
  ret.data.ptr = s.ptr;
  ret.data.len = s.len;
  ret.meta.wi = 0;
  ret.meta.ri = 0;
  ret.meta.pos = 0;
  ret.meta.closed = false;
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

static inline bool  //
wuffs_base__token_buffer__is_valid(const wuffs_base__token_buffer* buf) {
  if (buf) {
    if (buf->data.ptr) {
      return (buf->meta.ri <= buf->meta.wi) && (buf->meta.wi <= buf->data.len);
    } else {
      return (buf->meta.ri == 0) && (buf->meta.wi == 0) && (buf->data.len == 0);
    }
  }
  return false;
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

inline bool  //
wuffs_base__token_buffer::is_valid() const {
  return wuffs_base__token_buffer__is_valid(this);
}

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
