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

// ---------------- Images

// wuffs_base__color_u32_argb_premul is an 8 bit per channel premultiplied
// Alpha, Red, Green, Blue color, as a uint32_t value. It is in word order, not
// byte order: its value is always 0xAARRGGBB, regardless of endianness.
typedef uint32_t wuffs_base__color_u32_argb_premul;

// --------

// wuffs_base__pixel_format encodes the format of the bytes that constitute an
// image frame's pixel data. Its bits:
//  - bit        31  is reserved.
//  - bits 30 .. 28 encodes color (and channel order, in terms of memory).
//  - bit        27  is reserved.
//  - bits 26 .. 24 encodes transparency.
//  - bits 23 .. 21 are reserved.
//  - bit        20 indicates big-endian/MSB-first (as opposed to little/LSB).
//  - bit        19 indicates floating point (as opposed to integer).
//  - bit        18 indicates palette-indexed. The number-of-planes (the next
//                  field) will be 0, as the format is considered interleaved,
//                  but the 8-bit N-BGRA color data is stored in plane 3.
//  - bits 17 .. 16 are the number of planes, minus 1. Zero means interleaved.
//  - bits 15 .. 12 encodes the number of bits (depth) in the 3rd channel.
//  - bits 11 ..  8 encodes the number of bits (depth) in the 2nd channel.
//  - bits  7 ..  4 encodes the number of bits (depth) in the 1st channel.
//  - bits  3 ..  0 encodes the number of bits (depth) in the 0th channel.
//
// The bit fields of a wuffs_base__pixel_format are not independent. For
// example, the number of planes should not be greater than the number of
// channels. Similarly, bits 15..4 are unused (and should be zero) if bits
// 31..24 (color and transparency) together imply only 1 channel (gray, no
// alpha) and floating point samples should mean a bit depth of 16, 32 or 64.
//
// Formats hold between 1 and 4 channels. For example: Y (1 channel: gray), YA
// (2 channels: gray and alpha), BGR (3 channels: blue, green, red) or CMYK (4
// channels: cyan, magenta, yellow, black).
//
// For direct formats with N > 1 channels, those channels can be laid out in
// either 1 (interleaved) or N (planar) planes. For example, RGBA data is
// usually interleaved, but YCbCr data is usually planar, due to chroma
// subsampling (for details, see the wuffs_base__pixel_subsampling type).
//
// For indexed formats, the palette (always 256 × 4 bytes) holds 8 bits per
// channel non-alpha-premultiplied BGRA color data. There is only 1 plane (for
// the index), as the format is considered interleaved. Plane 0 holds the
// per-pixel indices. Plane 3 is re-purposed to hold the per-index colors.
//
// The color field is encoded in 3 bits:
//  - 0 means                   A (Alpha).
//  - 1 means Y         or     YA (Gray, Alpha).
//  - 2 means YCbCr     or YCbCrA (Luma, Chroma-blue, Chroma-red, Alpha).
//  - 3 means YCoCg     or YCoCgA (Luma, Chroma-orange, Chroma-green, Alpha).
//  - 4 means BGR, BGRX or   BGRA (Blue, Green, Red, X-padding or Alpha).
//  - 5 means RGB, RGBX or   RGBA (Red, Green, Blue, X-padding or Alpha).
//  - 6 means CMY       or   CMYK (Cyan, Magenta, Yellow, Black).
//  - all other values are reserved.
//
// In Wuffs, channels are given in memory order (also known as byte order),
// regardless of endianness, since the C type for the pixel data is an array of
// bytes, not an array of uint32_t. For example, interleaved BGRA with 8 bits
// per channel means that the bytes in memory are always Blue, Green, Red then
// Alpha. On big-endian systems, that is the uint32_t 0xBBGGRRAA. On
// little-endian, 0xAARRGGBB.
//
// When the color field (3 bits) encodes multiple options, the transparency
// field (3 bits) distinguishes them:
//  - 0 means fully opaque, no extra channels
//  - 1 means fully opaque, one extra channel (X or K, padding or black).
//  - 5 means one extra alpha channel, other channels are non-premultiplied.
//  - 6 means one extra alpha channel, other channels are     premultiplied.
//  - 7 means one extra alpha channel, binary alpha.
//  - all other values are reserved.
//
// Binary alpha means that if a color is not completely opaque, it is
// completely transparent black. As a source pixel format, it can therefore be
// treated as either non-premultiplied or premultiplied.
//
// The zero wuffs_base__pixel_format value is an invalid pixel format, as it is
// invalid to combine the zero color (alpha only) with the zero transparency.
//
// Bit depth is encoded in 4 bits:
//  -  0 means the channel or index is unused.
//  -  x means a bit depth of  x, for x in the range 1..8.
//  -  9 means a bit depth of 10.
//  - 10 means a bit depth of 12.
//  - 11 means a bit depth of 16.
//  - 12 means a bit depth of 24.
//  - 13 means a bit depth of 32.
//  - 14 means a bit depth of 48.
//  - 15 means a bit depth of 64.
//
// For example, wuffs_base__pixel_format 0x5510BBBB is a natural format for
// decoding a PNG image - network byte order (also known as big-endian),
// interleaved, non-premultiplied alpha - that happens to be 16-bit-depth
// truecolor with alpha (RGBA). In memory order:
//
//  ptr+0  ptr+1  ptr+2  ptr+3  ptr+4  ptr+5  ptr+6  ptr+7
//  Rhi    Rlo    Ghi    Glo    Bhi    Blo    Ahi    Alo
//
// For example, the value wuffs_base__pixel_format 0x40000565 means BGR with no
// alpha or padding, 5/6/5 bits for blue/green/red, interleaved 2 bytes per
// pixel, laid out LSB-first in memory order:
//
//  ptr+0...........  ptr+1...........
//  MSB          LSB  MSB          LSB
//  G₂G₁G₀B₄B₃B₂B₁B₀  R₄R₃R₂R₁R₀G₅G₄G₃
//
// On little-endian systems (but not big-endian), this Wuffs pixel format value
// (0x40000565) corresponds to the Cairo library's CAIRO_FORMAT_RGB16_565, the
// SDL2 (Simple DirectMedia Layer 2) library's SDL_PIXELFORMAT_RGB565 and the
// Skia library's kRGB_565_SkColorType. Note BGR in Wuffs versus RGB in the
// other libraries.
//
// Regardless of endianness, this Wuffs pixel format value (0x40000565)
// corresponds to the V4L2 (Video For Linux 2) library's V4L2_PIX_FMT_RGB565
// and the Wayland-DRM library's WL_DRM_FORMAT_RGB565.
//
// Different software libraries name their pixel formats (and especially their
// channel order) either according to memory layout or as bits of a native
// integer type like uint32_t. The two conventions differ because of a system's
// endianness. As mentioned earlier, Wuffs pixel formats are always in memory
// order. More detail of other software libraries' naming conventions is in the
// Pixel Format Guide at https://afrantzis.github.io/pixel-format-guide/
//
// Do not manipulate these bits directly; they are private implementation
// details. Use methods such as wuffs_base__pixel_format__num_planes instead.
typedef uint32_t wuffs_base__pixel_format;

// Common 8-bit-depth pixel formats. This list is not exhaustive; not all valid
// wuffs_base__pixel_format values are present.

#define WUFFS_BASE__PIXEL_FORMAT__INVALID ((wuffs_base__pixel_format)0x00000000)

#define WUFFS_BASE__PIXEL_FORMAT__A ((wuffs_base__pixel_format)0x02000008)

#define WUFFS_BASE__PIXEL_FORMAT__Y ((wuffs_base__pixel_format)0x10000008)
#define WUFFS_BASE__PIXEL_FORMAT__YA_NONPREMUL \
  ((wuffs_base__pixel_format)0x15000008)
#define WUFFS_BASE__PIXEL_FORMAT__YA_PREMUL \
  ((wuffs_base__pixel_format)0x16000008)

#define WUFFS_BASE__PIXEL_FORMAT__YCBCR ((wuffs_base__pixel_format)0x20020888)
#define WUFFS_BASE__PIXEL_FORMAT__YCBCRK ((wuffs_base__pixel_format)0x21038888)
#define WUFFS_BASE__PIXEL_FORMAT__YCBCRA_NONPREMUL \
  ((wuffs_base__pixel_format)0x25038888)

#define WUFFS_BASE__PIXEL_FORMAT__YCOCG ((wuffs_base__pixel_format)0x30020888)
#define WUFFS_BASE__PIXEL_FORMAT__YCOCGK ((wuffs_base__pixel_format)0x31038888)
#define WUFFS_BASE__PIXEL_FORMAT__YCOCGA_NONPREMUL \
  ((wuffs_base__pixel_format)0x35038888)

#define WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL \
  ((wuffs_base__pixel_format)0x45040008)
#define WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL \
  ((wuffs_base__pixel_format)0x46040008)
#define WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY \
  ((wuffs_base__pixel_format)0x47040008)

#define WUFFS_BASE__PIXEL_FORMAT__BGR ((wuffs_base__pixel_format)0x40000888)
#define WUFFS_BASE__PIXEL_FORMAT__BGRX ((wuffs_base__pixel_format)0x41008888)
#define WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL \
  ((wuffs_base__pixel_format)0x45008888)
#define WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL \
  ((wuffs_base__pixel_format)0x46008888)
#define WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY \
  ((wuffs_base__pixel_format)0x47008888)

#define WUFFS_BASE__PIXEL_FORMAT__RGB ((wuffs_base__pixel_format)0x50000888)
#define WUFFS_BASE__PIXEL_FORMAT__RGBX ((wuffs_base__pixel_format)0x51008888)
#define WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL \
  ((wuffs_base__pixel_format)0x55008888)
#define WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL \
  ((wuffs_base__pixel_format)0x56008888)
#define WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY \
  ((wuffs_base__pixel_format)0x57008888)

#define WUFFS_BASE__PIXEL_FORMAT__CMY ((wuffs_base__pixel_format)0x60020888)
#define WUFFS_BASE__PIXEL_FORMAT__CMYK ((wuffs_base__pixel_format)0x61038888)

extern const uint32_t wuffs_base__pixel_format__bits_per_channel[16];

static inline bool  //
wuffs_base__pixel_format__is_valid(wuffs_base__pixel_format f) {
  return f != 0;
}

// wuffs_base__pixel_format__bits_per_pixel returns the number of bits per
// pixel for interleaved pixel formats, and returns 0 for planar pixel formats.
static inline uint32_t  //
wuffs_base__pixel_format__bits_per_pixel(wuffs_base__pixel_format f) {
  if (((f >> 16) & 0x03) != 0) {
    return 0;
  }
  return wuffs_base__pixel_format__bits_per_channel[0x0F & (f >> 0)] +
         wuffs_base__pixel_format__bits_per_channel[0x0F & (f >> 4)] +
         wuffs_base__pixel_format__bits_per_channel[0x0F & (f >> 8)] +
         wuffs_base__pixel_format__bits_per_channel[0x0F & (f >> 12)];
}

static inline bool  //
wuffs_base__pixel_format__is_indexed(wuffs_base__pixel_format f) {
  return (f >> 18) & 0x01;
}

static inline bool  //
wuffs_base__pixel_format__is_interleaved(wuffs_base__pixel_format f) {
  return ((f >> 16) & 0x03) == 0;
}

static inline bool  //
wuffs_base__pixel_format__is_planar(wuffs_base__pixel_format f) {
  return ((f >> 16) & 0x03) != 0;
}

static inline uint32_t  //
wuffs_base__pixel_format__num_planes(wuffs_base__pixel_format f) {
  return ((f >> 16) & 0x03) + 1;
}

#define WUFFS_BASE__PIXEL_FORMAT__NUM_PLANES_MAX 4

#define WUFFS_BASE__PIXEL_FORMAT__INDEXED__INDEX_PLANE 0
#define WUFFS_BASE__PIXEL_FORMAT__INDEXED__COLOR_PLANE 3

// --------

// wuffs_base__pixel_subsampling encodes the mapping of pixel space coordinates
// (x, y) to pixel buffer indices (i, j). That mapping can differ for each
// plane p. For a depth of 8 bits (1 byte), the p'th plane's sample starts at
// (planes[p].ptr + (j * planes[p].stride) + i).
//
// For interleaved pixel formats, the mapping is trivial: i = x and j = y. For
// planar pixel formats, the mapping can differ due to chroma subsampling. For
// example, consider a three plane YCbCr pixel format with 4:2:2 subsampling.
// For the luma (Y) channel, there is one sample for every pixel, but for the
// chroma (Cb, Cr) channels, there is one sample for every two pixels: pairs of
// horizontally adjacent pixels form one macropixel, i = x / 2 and j == y. In
// general, for a given p:
//  - i = (x + bias_x) >> shift_x.
//  - j = (y + bias_y) >> shift_y.
// where biases and shifts are in the range 0..3 and 0..2 respectively.
//
// In general, the biases will be zero after decoding an image. However, making
// a sub-image may change the bias, since the (x, y) coordinates are relative
// to the sub-image's top-left origin, but the backing pixel buffers were
// created relative to the original image's origin.
//
// For each plane p, each of those four numbers (biases and shifts) are encoded
// in two bits, which combine to form an 8 bit unsigned integer:
//
//  e_p = (bias_x << 6) | (shift_x << 4) | (bias_y << 2) | (shift_y << 0)
//
// Those e_p values (e_0 for the first plane, e_1 for the second plane, etc)
// combine to form a wuffs_base__pixel_subsampling value:
//
//  pixsub = (e_3 << 24) | (e_2 << 16) | (e_1 << 8) | (e_0 << 0)
//
// Do not manipulate these bits directly; they are private implementation
// details. Use methods such as wuffs_base__pixel_subsampling__bias_x instead.
typedef uint32_t wuffs_base__pixel_subsampling;

#define WUFFS_BASE__PIXEL_SUBSAMPLING__NONE ((wuffs_base__pixel_subsampling)0)

#define WUFFS_BASE__PIXEL_SUBSAMPLING__444 \
  ((wuffs_base__pixel_subsampling)0x000000)
#define WUFFS_BASE__PIXEL_SUBSAMPLING__440 \
  ((wuffs_base__pixel_subsampling)0x010100)
#define WUFFS_BASE__PIXEL_SUBSAMPLING__422 \
  ((wuffs_base__pixel_subsampling)0x101000)
#define WUFFS_BASE__PIXEL_SUBSAMPLING__420 \
  ((wuffs_base__pixel_subsampling)0x111100)
#define WUFFS_BASE__PIXEL_SUBSAMPLING__411 \
  ((wuffs_base__pixel_subsampling)0x202000)
#define WUFFS_BASE__PIXEL_SUBSAMPLING__410 \
  ((wuffs_base__pixel_subsampling)0x212100)

static inline uint32_t  //
wuffs_base__pixel_subsampling__bias_x(wuffs_base__pixel_subsampling s,
                                      uint32_t plane) {
  uint32_t shift = ((plane & 0x03) * 8) + 6;
  return (s >> shift) & 0x03;
}

static inline uint32_t  //
wuffs_base__pixel_subsampling__shift_x(wuffs_base__pixel_subsampling s,
                                       uint32_t plane) {
  uint32_t shift = ((plane & 0x03) * 8) + 4;
  return (s >> shift) & 0x03;
}

static inline uint32_t  //
wuffs_base__pixel_subsampling__bias_y(wuffs_base__pixel_subsampling s,
                                      uint32_t plane) {
  uint32_t shift = ((plane & 0x03) * 8) + 2;
  return (s >> shift) & 0x03;
}

static inline uint32_t  //
wuffs_base__pixel_subsampling__shift_y(wuffs_base__pixel_subsampling s,
                                       uint32_t plane) {
  uint32_t shift = ((plane & 0x03) * 8) + 0;
  return (s >> shift) & 0x03;
}

// --------

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    wuffs_base__pixel_format pixfmt;
    wuffs_base__pixel_subsampling pixsub;
    uint32_t width;
    uint32_t height;
  } private_impl;

#ifdef __cplusplus
  inline void set(wuffs_base__pixel_format pixfmt,
                  wuffs_base__pixel_subsampling pixsub,
                  uint32_t width,
                  uint32_t height);
  inline void invalidate();
  inline bool is_valid() const;
  inline wuffs_base__pixel_format pixel_format() const;
  inline wuffs_base__pixel_subsampling pixel_subsampling() const;
  inline wuffs_base__rect_ie_u32 bounds() const;
  inline uint32_t width() const;
  inline uint32_t height() const;
  inline uint64_t pixbuf_len() const;
#endif  // __cplusplus

} wuffs_base__pixel_config;

static inline wuffs_base__pixel_config  //
wuffs_base__null_pixel_config() {
  wuffs_base__pixel_config ret;
  ret.private_impl.pixfmt = 0;
  ret.private_impl.pixsub = 0;
  ret.private_impl.width = 0;
  ret.private_impl.height = 0;
  return ret;
}

// TODO: Should this function return bool? An error type?
static inline void  //
wuffs_base__pixel_config__set(wuffs_base__pixel_config* c,
                              wuffs_base__pixel_format pixfmt,
                              wuffs_base__pixel_subsampling pixsub,
                              uint32_t width,
                              uint32_t height) {
  if (!c) {
    return;
  }
  if (pixfmt) {
    uint64_t wh = ((uint64_t)width) * ((uint64_t)height);
    // TODO: handle things other than 1 byte per pixel.
    if (wh <= ((uint64_t)SIZE_MAX)) {
      c->private_impl.pixfmt = pixfmt;
      c->private_impl.pixsub = pixsub;
      c->private_impl.width = width;
      c->private_impl.height = height;
      return;
    }
  }

  c->private_impl.pixfmt = 0;
  c->private_impl.pixsub = 0;
  c->private_impl.width = 0;
  c->private_impl.height = 0;
}

static inline void  //
wuffs_base__pixel_config__invalidate(wuffs_base__pixel_config* c) {
  if (c) {
    c->private_impl.pixfmt = 0;
    c->private_impl.pixsub = 0;
    c->private_impl.width = 0;
    c->private_impl.height = 0;
  }
}

static inline bool  //
wuffs_base__pixel_config__is_valid(const wuffs_base__pixel_config* c) {
  return c && c->private_impl.pixfmt;
}

static inline wuffs_base__pixel_format  //
wuffs_base__pixel_config__pixel_format(const wuffs_base__pixel_config* c) {
  return c ? c->private_impl.pixfmt : 0;
}

static inline wuffs_base__pixel_subsampling  //
wuffs_base__pixel_config__pixel_subsampling(const wuffs_base__pixel_config* c) {
  return c ? c->private_impl.pixsub : 0;
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__pixel_config__bounds(const wuffs_base__pixel_config* c) {
  if (c) {
    wuffs_base__rect_ie_u32 ret;
    ret.min_incl_x = 0;
    ret.min_incl_y = 0;
    ret.max_excl_x = c->private_impl.width;
    ret.max_excl_y = c->private_impl.height;
    return ret;
  }

  wuffs_base__rect_ie_u32 ret;
  ret.min_incl_x = 0;
  ret.min_incl_y = 0;
  ret.max_excl_x = 0;
  ret.max_excl_y = 0;
  return ret;
}

static inline uint32_t  //
wuffs_base__pixel_config__width(const wuffs_base__pixel_config* c) {
  return c ? c->private_impl.width : 0;
}

static inline uint32_t  //
wuffs_base__pixel_config__height(const wuffs_base__pixel_config* c) {
  return c ? c->private_impl.height : 0;
}

// TODO: this is the right API for planar (not interleaved) pixbufs? Should it
// allow decoding into a color model different from the format's intrinsic one?
// For example, decoding a JPEG image straight to RGBA instead of to YCbCr?
static inline uint64_t  //
wuffs_base__pixel_config__pixbuf_len(const wuffs_base__pixel_config* c) {
  if (!c) {
    return 0;
  }
  if (wuffs_base__pixel_format__is_planar(c->private_impl.pixfmt)) {
    // TODO: support planar pixel formats, concious of pixel subsampling.
    return 0;
  }
  uint32_t bits_per_pixel =
      wuffs_base__pixel_format__bits_per_pixel(c->private_impl.pixfmt);
  if ((bits_per_pixel == 0) || ((bits_per_pixel % 8) != 0)) {
    // TODO: support fraction-of-byte pixels, e.g. 1 bit per pixel?
    return 0;
  }
  uint64_t bytes_per_pixel = bits_per_pixel / 8;

  uint64_t n =
      ((uint64_t)c->private_impl.width) * ((uint64_t)c->private_impl.height);
  if (n > (UINT64_MAX / bytes_per_pixel)) {
    return 0;
  }
  n *= bytes_per_pixel;

  if (wuffs_base__pixel_format__is_indexed(c->private_impl.pixfmt)) {
    if (n > (UINT64_MAX - 1024)) {
      return 0;
    }
    n += 1024;
  }

  return n;
}

#ifdef __cplusplus

inline void  //
wuffs_base__pixel_config::set(wuffs_base__pixel_format pixfmt,
                              wuffs_base__pixel_subsampling pixsub,
                              uint32_t width,
                              uint32_t height) {
  wuffs_base__pixel_config__set(this, pixfmt, pixsub, width, height);
}

inline void  //
wuffs_base__pixel_config::invalidate() {
  wuffs_base__pixel_config__invalidate(this);
}

inline bool  //
wuffs_base__pixel_config::is_valid() const {
  return wuffs_base__pixel_config__is_valid(this);
}

inline wuffs_base__pixel_format  //
wuffs_base__pixel_config::pixel_format() const {
  return wuffs_base__pixel_config__pixel_format(this);
}

inline wuffs_base__pixel_subsampling  //
wuffs_base__pixel_config::pixel_subsampling() const {
  return wuffs_base__pixel_config__pixel_subsampling(this);
}

inline wuffs_base__rect_ie_u32  //
wuffs_base__pixel_config::bounds() const {
  return wuffs_base__pixel_config__bounds(this);
}

inline uint32_t  //
wuffs_base__pixel_config::width() const {
  return wuffs_base__pixel_config__width(this);
}

inline uint32_t  //
wuffs_base__pixel_config::height() const {
  return wuffs_base__pixel_config__height(this);
}

inline uint64_t  //
wuffs_base__pixel_config::pixbuf_len() const {
  return wuffs_base__pixel_config__pixbuf_len(this);
}

#endif  // __cplusplus

// --------

typedef struct {
  wuffs_base__pixel_config pixcfg;

  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    uint64_t first_frame_io_position;
    bool first_frame_is_opaque;
  } private_impl;

#ifdef __cplusplus
  inline void set(wuffs_base__pixel_format pixfmt,
                  wuffs_base__pixel_subsampling pixsub,
                  uint32_t width,
                  uint32_t height,
                  uint64_t first_frame_io_position,
                  bool first_frame_is_opaque);
  inline void invalidate();
  inline bool is_valid() const;
  inline uint64_t first_frame_io_position() const;
  inline bool first_frame_is_opaque() const;
#endif  // __cplusplus

} wuffs_base__image_config;

static inline wuffs_base__image_config  //
wuffs_base__null_image_config() {
  wuffs_base__image_config ret;
  ret.pixcfg = wuffs_base__null_pixel_config();
  ret.private_impl.first_frame_io_position = 0;
  ret.private_impl.first_frame_is_opaque = false;
  return ret;
}

// TODO: Should this function return bool? An error type?
static inline void  //
wuffs_base__image_config__set(wuffs_base__image_config* c,
                              wuffs_base__pixel_format pixfmt,
                              wuffs_base__pixel_subsampling pixsub,
                              uint32_t width,
                              uint32_t height,
                              uint64_t first_frame_io_position,
                              bool first_frame_is_opaque) {
  if (!c) {
    return;
  }
  if (wuffs_base__pixel_format__is_valid(pixfmt)) {
    c->pixcfg.private_impl.pixfmt = pixfmt;
    c->pixcfg.private_impl.pixsub = pixsub;
    c->pixcfg.private_impl.width = width;
    c->pixcfg.private_impl.height = height;
    c->private_impl.first_frame_io_position = first_frame_io_position;
    c->private_impl.first_frame_is_opaque = first_frame_is_opaque;
    return;
  }

  c->pixcfg.private_impl.pixfmt = 0;
  c->pixcfg.private_impl.pixsub = 0;
  c->pixcfg.private_impl.width = 0;
  c->pixcfg.private_impl.height = 0;
  c->private_impl.first_frame_io_position = 0;
  c->private_impl.first_frame_is_opaque = 0;
}

static inline void  //
wuffs_base__image_config__invalidate(wuffs_base__image_config* c) {
  if (c) {
    c->pixcfg.private_impl.pixfmt = 0;
    c->pixcfg.private_impl.pixsub = 0;
    c->pixcfg.private_impl.width = 0;
    c->pixcfg.private_impl.height = 0;
    c->private_impl.first_frame_io_position = 0;
    c->private_impl.first_frame_is_opaque = 0;
  }
}

static inline bool  //
wuffs_base__image_config__is_valid(const wuffs_base__image_config* c) {
  return c && wuffs_base__pixel_config__is_valid(&(c->pixcfg));
}

static inline uint64_t  //
wuffs_base__image_config__first_frame_io_position(
    const wuffs_base__image_config* c) {
  return c ? c->private_impl.first_frame_io_position : 0;
}

static inline bool  //
wuffs_base__image_config__first_frame_is_opaque(
    const wuffs_base__image_config* c) {
  return c ? c->private_impl.first_frame_is_opaque : false;
}

#ifdef __cplusplus

inline void  //
wuffs_base__image_config::set(wuffs_base__pixel_format pixfmt,
                              wuffs_base__pixel_subsampling pixsub,
                              uint32_t width,
                              uint32_t height,
                              uint64_t first_frame_io_position,
                              bool first_frame_is_opaque) {
  wuffs_base__image_config__set(this, pixfmt, pixsub, width, height,
                                first_frame_io_position, first_frame_is_opaque);
}

inline void  //
wuffs_base__image_config::invalidate() {
  wuffs_base__image_config__invalidate(this);
}

inline bool  //
wuffs_base__image_config::is_valid() const {
  return wuffs_base__image_config__is_valid(this);
}

inline uint64_t  //
wuffs_base__image_config::first_frame_io_position() const {
  return wuffs_base__image_config__first_frame_io_position(this);
}

inline bool  //
wuffs_base__image_config::first_frame_is_opaque() const {
  return wuffs_base__image_config__first_frame_is_opaque(this);
}

#endif  // __cplusplus

// --------

// wuffs_base__animation_blend encodes, for an animated image, how to blend the
// transparent pixels of this frame with the existing canvas. In Porter-Duff
// compositing operator terminology:
//  - 0 means the frame may be transparent, and should be blended "src over
//    dst", also known as just "over".
//  - 1 means the frame may be transparent, and should be blended "src".
//  - 2 means the frame is completely opaque, so that "src over dst" and "src"
//    are equivalent.
//
// These semantics are conservative. It is valid for a completely opaque frame
// to have a blend value other than 2.
typedef uint8_t wuffs_base__animation_blend;

#define WUFFS_BASE__ANIMATION_BLEND__SRC_OVER_DST \
  ((wuffs_base__animation_blend)0)
#define WUFFS_BASE__ANIMATION_BLEND__SRC ((wuffs_base__animation_blend)1)
#define WUFFS_BASE__ANIMATION_BLEND__OPAQUE ((wuffs_base__animation_blend)2)

// --------

// wuffs_base__animation_disposal encodes, for an animated image, how to
// dispose of a frame after displaying it:
//  - None means to draw the next frame on top of this one.
//  - Restore Background means to clear the frame's dirty rectangle to "the
//    background color" (in practice, this means transparent black) before
//    drawing the next frame.
//  - Restore Previous means to undo the current frame, so that the next frame
//    is drawn on top of the previous one.
typedef uint8_t wuffs_base__animation_disposal;

#define WUFFS_BASE__ANIMATION_DISPOSAL__NONE ((wuffs_base__animation_disposal)0)
#define WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_BACKGROUND \
  ((wuffs_base__animation_disposal)1)
#define WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS \
  ((wuffs_base__animation_disposal)2)

// --------

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    wuffs_base__rect_ie_u32 bounds;
    wuffs_base__flicks duration;
    uint64_t index;
    uint64_t io_position;
    wuffs_base__animation_blend blend;
    wuffs_base__animation_disposal disposal;
    wuffs_base__color_u32_argb_premul background_color;
  } private_impl;

#ifdef __cplusplus
  inline void update(wuffs_base__rect_ie_u32 bounds,
                     wuffs_base__flicks duration,
                     uint64_t index,
                     uint64_t io_position,
                     wuffs_base__animation_blend blend,
                     wuffs_base__animation_disposal disposal,
                     wuffs_base__color_u32_argb_premul background_color);
  inline wuffs_base__rect_ie_u32 bounds() const;
  inline uint32_t width() const;
  inline uint32_t height() const;
  inline wuffs_base__flicks duration() const;
  inline uint64_t index() const;
  inline uint64_t io_position() const;
  inline wuffs_base__animation_blend blend() const;
  inline wuffs_base__animation_disposal disposal() const;
  inline wuffs_base__color_u32_argb_premul background_color() const;
#endif  // __cplusplus

} wuffs_base__frame_config;

static inline wuffs_base__frame_config  //
wuffs_base__null_frame_config() {
  wuffs_base__frame_config ret;
  ret.private_impl.bounds = wuffs_base__make_rect_ie_u32(0, 0, 0, 0);
  ret.private_impl.duration = 0;
  ret.private_impl.index = 0;
  ret.private_impl.io_position = 0;
  ret.private_impl.blend = 0;
  ret.private_impl.disposal = 0;
  return ret;
}

static inline void  //
wuffs_base__frame_config__update(
    wuffs_base__frame_config* c,
    wuffs_base__rect_ie_u32 bounds,
    wuffs_base__flicks duration,
    uint64_t index,
    uint64_t io_position,
    wuffs_base__animation_blend blend,
    wuffs_base__animation_disposal disposal,
    wuffs_base__color_u32_argb_premul background_color) {
  if (!c) {
    return;
  }

  c->private_impl.bounds = bounds;
  c->private_impl.duration = duration;
  c->private_impl.index = index;
  c->private_impl.io_position = io_position;
  c->private_impl.blend = blend;
  c->private_impl.disposal = disposal;
  c->private_impl.background_color = background_color;
}

static inline wuffs_base__rect_ie_u32  //
wuffs_base__frame_config__bounds(const wuffs_base__frame_config* c) {
  if (c) {
    return c->private_impl.bounds;
  }

  wuffs_base__rect_ie_u32 ret;
  ret.min_incl_x = 0;
  ret.min_incl_y = 0;
  ret.max_excl_x = 0;
  ret.max_excl_y = 0;
  return ret;
}

static inline uint32_t  //
wuffs_base__frame_config__width(const wuffs_base__frame_config* c) {
  return c ? wuffs_base__rect_ie_u32__width(&c->private_impl.bounds) : 0;
}

static inline uint32_t  //
wuffs_base__frame_config__height(const wuffs_base__frame_config* c) {
  return c ? wuffs_base__rect_ie_u32__height(&c->private_impl.bounds) : 0;
}

// wuffs_base__frame_config__duration returns the amount of time to display
// this frame. Zero means to display forever - a still (non-animated) image.
static inline wuffs_base__flicks  //
wuffs_base__frame_config__duration(const wuffs_base__frame_config* c) {
  return c ? c->private_impl.duration : 0;
}

// wuffs_base__frame_config__index returns the index of this frame. The first
// frame in an image has index 0, the second frame has index 1, and so on.
static inline uint64_t  //
wuffs_base__frame_config__index(const wuffs_base__frame_config* c) {
  return c ? c->private_impl.index : 0;
}

// wuffs_base__frame_config__io_position returns the I/O stream position before
// the frame config.
static inline uint64_t  //
wuffs_base__frame_config__io_position(const wuffs_base__frame_config* c) {
  return c ? c->private_impl.io_position : 0;
}

// wuffs_base__frame_config__blend returns, for an animated image, how to blend
// the transparent pixels of this frame with the existing canvas.
static inline wuffs_base__animation_blend  //
wuffs_base__frame_config__blend(const wuffs_base__frame_config* c) {
  return c ? c->private_impl.blend : 0;
}

// wuffs_base__frame_config__disposal returns, for an animated image, how to
// dispose of this frame after displaying it.
static inline wuffs_base__animation_disposal  //
wuffs_base__frame_config__disposal(const wuffs_base__frame_config* c) {
  return c ? c->private_impl.disposal : 0;
}

static inline wuffs_base__color_u32_argb_premul  //
wuffs_base__frame_config__background_color(const wuffs_base__frame_config* c) {
  return c ? c->private_impl.background_color : 0;
}

#ifdef __cplusplus

inline void  //
wuffs_base__frame_config::update(
    wuffs_base__rect_ie_u32 bounds,
    wuffs_base__flicks duration,
    uint64_t index,
    uint64_t io_position,
    wuffs_base__animation_blend blend,
    wuffs_base__animation_disposal disposal,
    wuffs_base__color_u32_argb_premul background_color) {
  wuffs_base__frame_config__update(this, bounds, duration, index, io_position,
                                   blend, disposal, background_color);
}

inline wuffs_base__rect_ie_u32  //
wuffs_base__frame_config::bounds() const {
  return wuffs_base__frame_config__bounds(this);
}

inline uint32_t  //
wuffs_base__frame_config::width() const {
  return wuffs_base__frame_config__width(this);
}

inline uint32_t  //
wuffs_base__frame_config::height() const {
  return wuffs_base__frame_config__height(this);
}

inline wuffs_base__flicks  //
wuffs_base__frame_config::duration() const {
  return wuffs_base__frame_config__duration(this);
}

inline uint64_t  //
wuffs_base__frame_config::index() const {
  return wuffs_base__frame_config__index(this);
}

inline uint64_t  //
wuffs_base__frame_config::io_position() const {
  return wuffs_base__frame_config__io_position(this);
}

inline wuffs_base__animation_blend  //
wuffs_base__frame_config::blend() const {
  return wuffs_base__frame_config__blend(this);
}

inline wuffs_base__animation_disposal  //
wuffs_base__frame_config::disposal() const {
  return wuffs_base__frame_config__disposal(this);
}

inline wuffs_base__color_u32_argb_premul  //
wuffs_base__frame_config::background_color() const {
  return wuffs_base__frame_config__background_color(this);
}

#endif  // __cplusplus

// --------

typedef struct {
  wuffs_base__pixel_config pixcfg;

  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    wuffs_base__table_u8 planes[WUFFS_BASE__PIXEL_FORMAT__NUM_PLANES_MAX];
    // TODO: color spaces.
  } private_impl;

#ifdef __cplusplus
  inline wuffs_base__status set_from_slice(wuffs_base__pixel_config* pixcfg,
                                           wuffs_base__slice_u8 pixbuf_memory);
  inline wuffs_base__status set_from_table(wuffs_base__pixel_config* pixcfg,
                                           wuffs_base__table_u8 pixbuf_memory);
  inline wuffs_base__slice_u8 palette();
  inline wuffs_base__pixel_format pixel_format() const;
  inline wuffs_base__table_u8 plane(uint32_t p);
#endif  // __cplusplus

} wuffs_base__pixel_buffer;

static inline wuffs_base__pixel_buffer  //
wuffs_base__null_pixel_buffer() {
  wuffs_base__pixel_buffer ret;
  ret.pixcfg = wuffs_base__null_pixel_config();
  ret.private_impl.planes[0] = wuffs_base__empty_table_u8();
  ret.private_impl.planes[1] = wuffs_base__empty_table_u8();
  ret.private_impl.planes[2] = wuffs_base__empty_table_u8();
  ret.private_impl.planes[3] = wuffs_base__empty_table_u8();
  return ret;
}

static inline wuffs_base__status  //
wuffs_base__pixel_buffer__set_from_slice(wuffs_base__pixel_buffer* b,
                                         wuffs_base__pixel_config* pixcfg,
                                         wuffs_base__slice_u8 pixbuf_memory) {
  if (!b) {
    return wuffs_base__error__bad_receiver;
  }
  memset(b, 0, sizeof(*b));
  if (!pixcfg) {
    return wuffs_base__error__bad_argument;
  }
  if (wuffs_base__pixel_format__is_planar(pixcfg->private_impl.pixfmt)) {
    // TODO: support planar pixel formats, concious of pixel subsampling.
    return wuffs_base__error__unsupported_option;
  }
  uint32_t bits_per_pixel =
      wuffs_base__pixel_format__bits_per_pixel(pixcfg->private_impl.pixfmt);
  if ((bits_per_pixel == 0) || ((bits_per_pixel % 8) != 0)) {
    // TODO: support fraction-of-byte pixels, e.g. 1 bit per pixel?
    return wuffs_base__error__unsupported_option;
  }
  uint64_t bytes_per_pixel = bits_per_pixel / 8;

  uint8_t* ptr = pixbuf_memory.ptr;
  uint64_t len = pixbuf_memory.len;
  if (wuffs_base__pixel_format__is_indexed(pixcfg->private_impl.pixfmt)) {
    // Split a 1024 byte chunk (256 palette entries × 4 bytes per entry) from
    // the start of pixbuf_memory. We split from the start, not the end, so
    // that the both chunks' pointers have the same alignment as the original
    // pointer, up to an alignment of 1024.
    if (len < 1024) {
      return wuffs_base__error__bad_argument_length_too_short;
    }
    wuffs_base__table_u8* tab =
        &b->private_impl.planes[WUFFS_BASE__PIXEL_FORMAT__INDEXED__COLOR_PLANE];
    tab->ptr = ptr;
    tab->width = 1024;
    tab->height = 1;
    tab->stride = 1024;
    ptr += 1024;
    len -= 1024;
  }

  uint64_t wh = ((uint64_t)pixcfg->private_impl.width) *
                ((uint64_t)pixcfg->private_impl.height);
  size_t width = (size_t)(pixcfg->private_impl.width);
  if ((wh > (UINT64_MAX / bytes_per_pixel)) ||
      (width > (SIZE_MAX / bytes_per_pixel))) {
    return wuffs_base__error__bad_argument;
  }
  wh *= bytes_per_pixel;
  width *= bytes_per_pixel;
  if (wh > len) {
    return wuffs_base__error__bad_argument_length_too_short;
  }

  b->pixcfg = *pixcfg;
  wuffs_base__table_u8* tab = &b->private_impl.planes[0];
  tab->ptr = ptr;
  tab->width = width;
  tab->height = pixcfg->private_impl.height;
  tab->stride = width;
  return NULL;
}

static inline wuffs_base__status  //
wuffs_base__pixel_buffer__set_from_table(wuffs_base__pixel_buffer* b,
                                         wuffs_base__pixel_config* pixcfg,
                                         wuffs_base__table_u8 pixbuf_memory) {
  if (!b) {
    return wuffs_base__error__bad_receiver;
  }
  memset(b, 0, sizeof(*b));
  if (!pixcfg ||
      wuffs_base__pixel_format__is_planar(pixcfg->private_impl.pixfmt)) {
    return wuffs_base__error__bad_argument;
  }
  uint32_t bits_per_pixel =
      wuffs_base__pixel_format__bits_per_pixel(pixcfg->private_impl.pixfmt);
  if ((bits_per_pixel == 0) || ((bits_per_pixel % 8) != 0)) {
    // TODO: support fraction-of-byte pixels, e.g. 1 bit per pixel?
    return wuffs_base__error__unsupported_option;
  }
  uint64_t bytes_per_pixel = bits_per_pixel / 8;

  uint64_t width_in_bytes =
      ((uint64_t)pixcfg->private_impl.width) * bytes_per_pixel;
  if ((width_in_bytes > pixbuf_memory.width) ||
      (pixcfg->private_impl.height > pixbuf_memory.height)) {
    return wuffs_base__error__bad_argument;
  }

  b->pixcfg = *pixcfg;
  b->private_impl.planes[0] = pixbuf_memory;
  return NULL;
}

// wuffs_base__pixel_buffer__palette returns the palette color data. If
// non-empty, it will have length 1024.
static inline wuffs_base__slice_u8  //
wuffs_base__pixel_buffer__palette(wuffs_base__pixel_buffer* b) {
  if (b &&
      wuffs_base__pixel_format__is_indexed(b->pixcfg.private_impl.pixfmt)) {
    wuffs_base__table_u8* tab =
        &b->private_impl.planes[WUFFS_BASE__PIXEL_FORMAT__INDEXED__COLOR_PLANE];
    if ((tab->width == 1024) && (tab->height == 1)) {
      return wuffs_base__make_slice_u8(tab->ptr, 1024);
    }
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

static inline wuffs_base__pixel_format  //
wuffs_base__pixel_buffer__pixel_format(const wuffs_base__pixel_buffer* b) {
  if (b) {
    return b->pixcfg.private_impl.pixfmt;
  }
  return WUFFS_BASE__PIXEL_FORMAT__INVALID;
}

static inline wuffs_base__table_u8  //
wuffs_base__pixel_buffer__plane(wuffs_base__pixel_buffer* b, uint32_t p) {
  if (b && (p < WUFFS_BASE__PIXEL_FORMAT__NUM_PLANES_MAX)) {
    return b->private_impl.planes[p];
  }

  wuffs_base__table_u8 ret;
  ret.ptr = NULL;
  ret.width = 0;
  ret.height = 0;
  ret.stride = 0;
  return ret;
}

#ifdef __cplusplus

inline wuffs_base__status  //
wuffs_base__pixel_buffer::set_from_slice(wuffs_base__pixel_config* pixcfg,
                                         wuffs_base__slice_u8 pixbuf_memory) {
  return wuffs_base__pixel_buffer__set_from_slice(this, pixcfg, pixbuf_memory);
}

inline wuffs_base__status  //
wuffs_base__pixel_buffer::set_from_table(wuffs_base__pixel_config* pixcfg,
                                         wuffs_base__table_u8 pixbuf_memory) {
  return wuffs_base__pixel_buffer__set_from_table(this, pixcfg, pixbuf_memory);
}

inline wuffs_base__slice_u8  //
wuffs_base__pixel_buffer::palette() {
  return wuffs_base__pixel_buffer__palette(this);
}

inline wuffs_base__pixel_format  //
wuffs_base__pixel_buffer::pixel_format() const {
  return wuffs_base__pixel_buffer__pixel_format(this);
}

inline wuffs_base__table_u8  //
wuffs_base__pixel_buffer::plane(uint32_t p) {
  return wuffs_base__pixel_buffer__plane(this, p);
}

#endif  // __cplusplus

// --------

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    uint8_t TODO;
  } private_impl;

#ifdef __cplusplus
#endif  // __cplusplus

} wuffs_base__decode_frame_options;

#ifdef __cplusplus

#endif  // __cplusplus

// --------

typedef struct {
  // Do not access the private_impl's fields directly. There is no API/ABI
  // compatibility or safety guarantee if you do so.
  struct {
    // TODO: should the func type take restrict pointers?
    uint64_t (*func)(wuffs_base__slice_u8 dst,
                     wuffs_base__slice_u8 dst_palette,
                     wuffs_base__slice_u8 src);
  } private_impl;

#ifdef __cplusplus
  inline wuffs_base__status prepare(wuffs_base__pixel_format dst_format,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__pixel_format src_format,
                                    wuffs_base__slice_u8 src_palette);
  inline uint64_t swizzle_interleaved(wuffs_base__slice_u8 dst,
                                      wuffs_base__slice_u8 dst_palette,
                                      wuffs_base__slice_u8 src) const;
#endif  // __cplusplus

} wuffs_base__pixel_swizzler;

wuffs_base__status  //
wuffs_base__pixel_swizzler__prepare(wuffs_base__pixel_swizzler* p,
                                    wuffs_base__pixel_format dst_format,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__pixel_format src_format,
                                    wuffs_base__slice_u8 src_palette);

uint64_t  //
wuffs_base__pixel_swizzler__swizzle_interleaved(
    const wuffs_base__pixel_swizzler* p,
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src);

#ifdef __cplusplus

inline wuffs_base__status  //
wuffs_base__pixel_swizzler::prepare(wuffs_base__pixel_format dst_format,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__pixel_format src_format,
                                    wuffs_base__slice_u8 src_palette) {
  return wuffs_base__pixel_swizzler__prepare(this, dst_format, dst_palette,
                                             src_format, src_palette);
}

uint64_t  //
wuffs_base__pixel_swizzler::swizzle_interleaved(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) const {
  return wuffs_base__pixel_swizzler__swizzle_interleaved(this, dst, dst_palette,
                                                         src);
}

#endif  // __cplusplus
