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

// ---------------- Pixel Swizzler

static inline uint32_t  //
wuffs_base__swap_u32_argb_abgr(uint32_t u) {
  uint32_t o = u & 0xFF00FF00;
  uint32_t r = u & 0x00FF0000;
  uint32_t b = u & 0x000000FF;
  return o | (r >> 16) | (b << 16);
}

// --------

WUFFS_BASE__MAYBE_STATIC wuffs_base__color_u32_argb_premul  //
wuffs_base__pixel_buffer__color_u32_at(const wuffs_base__pixel_buffer* pb,
                                       uint32_t x,
                                       uint32_t y) {
  if (!pb || (x >= pb->pixcfg.private_impl.width) ||
      (y >= pb->pixcfg.private_impl.height)) {
    return 0;
  }

  if (wuffs_base__pixel_format__is_planar(&pb->pixcfg.private_impl.pixfmt)) {
    // TODO: support planar formats.
    return 0;
  }

  size_t stride = pb->private_impl.planes[0].stride;
  const uint8_t* row = pb->private_impl.planes[0].ptr + (stride * ((size_t)y));

  switch (pb->pixcfg.private_impl.pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
      return wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x)));

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY: {
      uint8_t* palette = pb->private_impl.planes[3].ptr;
      return wuffs_base__load_u32le__no_bounds_check(palette +
                                                     (4 * ((size_t)row[x])));
    }

      // Common formats above. Rarer formats below.

    case WUFFS_BASE__PIXEL_FORMAT__Y:
      return 0xFF000000 | (0x00010101 * ((uint32_t)(row[x])));

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL: {
      uint8_t* palette = pb->private_impl.planes[3].ptr;
      return wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
          wuffs_base__load_u32le__no_bounds_check(palette +
                                                  (4 * ((size_t)row[x]))));
    }

    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      return wuffs_base__color_u16_rgb_565__as__color_u32_argb_premul(
          wuffs_base__load_u16le__no_bounds_check(row + (2 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      return 0xFF000000 |
             wuffs_base__load_u24le__no_bounds_check(row + (3 * ((size_t)x)));
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      return wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      return 0xFF000000 |
             wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x)));

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      return wuffs_base__swap_u32_argb_abgr(
          0xFF000000 |
          wuffs_base__load_u24le__no_bounds_check(row + (3 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
      return wuffs_base__swap_u32_argb_abgr(
          wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
              wuffs_base__load_u32le__no_bounds_check(row +
                                                      (4 * ((size_t)x)))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
      return wuffs_base__swap_u32_argb_abgr(
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      return wuffs_base__swap_u32_argb_abgr(
          0xFF000000 |
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));

    default:
      // TODO: support more formats.
      break;
  }

  return 0;
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__status  //
wuffs_base__pixel_buffer__set_color_u32_at(
    wuffs_base__pixel_buffer* pb,
    uint32_t x,
    uint32_t y,
    wuffs_base__color_u32_argb_premul color) {
  if (!pb) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if ((x >= pb->pixcfg.private_impl.width) ||
      (y >= pb->pixcfg.private_impl.height)) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }

  if (wuffs_base__pixel_format__is_planar(&pb->pixcfg.private_impl.pixfmt)) {
    // TODO: support planar formats.
    return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }

  size_t stride = pb->private_impl.planes[0].stride;
  uint8_t* row = pb->private_impl.planes[0].ptr + (stride * ((size_t)y));

  switch (pb->pixcfg.private_impl.pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      wuffs_base__store_u32le__no_bounds_check(row + (4 * ((size_t)x)), color);
      break;

      // Common formats above. Rarer formats below.

    case WUFFS_BASE__PIXEL_FORMAT__Y:
      wuffs_base__store_u8__no_bounds_check(
          row + ((size_t)x),
          wuffs_base__color_u32_argb_premul__as__color_u8_gray(color));
      break;

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
      wuffs_base__store_u8__no_bounds_check(
          row + ((size_t)x), wuffs_base__pixel_palette__closest_element(
                                 wuffs_base__pixel_buffer__palette(pb),
                                 pb->pixcfg.private_impl.pixfmt, color));
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      wuffs_base__store_u16le__no_bounds_check(
          row + (2 * ((size_t)x)),
          wuffs_base__color_u32_argb_premul__as__color_u16_rgb_565(color));
      break;
    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      wuffs_base__store_u24le__no_bounds_check(row + (3 * ((size_t)x)), color);
      break;
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)),
          wuffs_base__color_u32_argb_premul__as__color_u32_argb_nonpremul(
              color));
      break;

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      wuffs_base__store_u24le__no_bounds_check(
          row + (3 * ((size_t)x)), wuffs_base__swap_u32_argb_abgr(color));
      break;
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)),
          wuffs_base__color_u32_argb_premul__as__color_u32_argb_nonpremul(
              wuffs_base__swap_u32_argb_abgr(color)));
      break;
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)), wuffs_base__swap_u32_argb_abgr(color));
      break;

    default:
      // TODO: support more formats.
      return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }

  return wuffs_base__make_status(NULL);
}

// --------

WUFFS_BASE__MAYBE_STATIC uint8_t  //
wuffs_base__pixel_palette__closest_element(
    wuffs_base__slice_u8 palette_slice,
    wuffs_base__pixel_format palette_format,
    wuffs_base__color_u32_argb_premul c) {
  size_t n = palette_slice.len / 4;
  if (n > 256) {
    n = 256;
  }
  size_t best_index = 0;
  uint64_t best_score = 0xFFFFFFFFFFFFFFFF;

  // Work in 16-bit color.
  uint32_t ca = 0x101 * (0xFF & (c >> 24));
  uint32_t cr = 0x101 * (0xFF & (c >> 16));
  uint32_t cg = 0x101 * (0xFF & (c >> 8));
  uint32_t cb = 0x101 * (0xFF & (c >> 0));

  switch (palette_format.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY: {
      bool nonpremul = palette_format.repr ==
                       WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL;

      size_t i;
      for (i = 0; i < n; i++) {
        // Work in 16-bit color.
        uint32_t pb = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 0]));
        uint32_t pg = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 1]));
        uint32_t pr = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 2]));
        uint32_t pa = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 3]));

        // Convert to premultiplied alpha.
        if (nonpremul && (pa != 0xFFFF)) {
          pb = (pb * pa) / 0xFFFF;
          pg = (pg * pa) / 0xFFFF;
          pr = (pr * pa) / 0xFFFF;
        }

        // These deltas are conceptually int32_t (signed) but after squaring,
        // it's equivalent to work in uint32_t (unsigned).
        pb -= cb;
        pg -= cg;
        pr -= cr;
        pa -= ca;
        uint64_t score = ((uint64_t)(pb * pb)) + ((uint64_t)(pg * pg)) +
                         ((uint64_t)(pr * pr)) + ((uint64_t)(pa * pa));
        if (best_score > score) {
          best_score = score;
          best_index = i;
        }
      }
      break;
    }
  }

  return (uint8_t)best_index;
}

// --------

static inline uint32_t  //
wuffs_base__composite_nonpremul_nonpremul_u32_axxx(uint32_t dst_nonpremul,
                                                   uint32_t src_nonpremul) {
  // Convert from 8-bit color to 16-bit color.
  uint32_t sa = 0x101 * (0xFF & (src_nonpremul >> 24));
  uint32_t sr = 0x101 * (0xFF & (src_nonpremul >> 16));
  uint32_t sg = 0x101 * (0xFF & (src_nonpremul >> 8));
  uint32_t sb = 0x101 * (0xFF & (src_nonpremul >> 0));
  uint32_t da = 0x101 * (0xFF & (dst_nonpremul >> 24));
  uint32_t dr = 0x101 * (0xFF & (dst_nonpremul >> 16));
  uint32_t dg = 0x101 * (0xFF & (dst_nonpremul >> 8));
  uint32_t db = 0x101 * (0xFF & (dst_nonpremul >> 0));

  // Convert dst from nonpremul to premul.
  dr = (dr * da) / 0xFFFF;
  dg = (dg * da) / 0xFFFF;
  db = (db * da) / 0xFFFF;

  // Calculate the inverse of the src-alpha: how much of the dst to keep.
  uint32_t ia = 0xFFFF - sa;

  // Composite src (nonpremul) over dst (premul).
  da = sa + ((da * ia) / 0xFFFF);
  dr = ((sr * sa) + (dr * ia)) / 0xFFFF;
  dg = ((sg * sa) + (dg * ia)) / 0xFFFF;
  db = ((sb * sa) + (db * ia)) / 0xFFFF;

  // Convert dst from premul to nonpremul.
  if (da != 0) {
    dr = (dr * 0xFFFF) / da;
    dg = (dg * 0xFFFF) / da;
    db = (db * 0xFFFF) / da;
  }

  // Convert from 16-bit color to 8-bit color and combine the components.
  da >>= 8;
  dr >>= 8;
  dg >>= 8;
  db >>= 8;
  return (db << 0) | (dg << 8) | (dr << 16) | (da << 24);
}

static inline uint32_t  //
wuffs_base__composite_nonpremul_premul_u32_axxx(uint32_t dst_nonpremul,
                                                uint32_t src_premul) {
  // Convert from 8-bit color to 16-bit color.
  uint32_t sa = 0x101 * (0xFF & (src_premul >> 24));
  uint32_t sr = 0x101 * (0xFF & (src_premul >> 16));
  uint32_t sg = 0x101 * (0xFF & (src_premul >> 8));
  uint32_t sb = 0x101 * (0xFF & (src_premul >> 0));
  uint32_t da = 0x101 * (0xFF & (dst_nonpremul >> 24));
  uint32_t dr = 0x101 * (0xFF & (dst_nonpremul >> 16));
  uint32_t dg = 0x101 * (0xFF & (dst_nonpremul >> 8));
  uint32_t db = 0x101 * (0xFF & (dst_nonpremul >> 0));

  // Convert dst from nonpremul to premul.
  dr = (dr * da) / 0xFFFF;
  dg = (dg * da) / 0xFFFF;
  db = (db * da) / 0xFFFF;

  // Calculate the inverse of the src-alpha: how much of the dst to keep.
  uint32_t ia = 0xFFFF - sa;

  // Composite src (premul) over dst (premul).
  da = sa + ((da * ia) / 0xFFFF);
  dr = sr + ((dr * ia) / 0xFFFF);
  dg = sg + ((dg * ia) / 0xFFFF);
  db = sb + ((db * ia) / 0xFFFF);

  // Convert dst from premul to nonpremul.
  if (da != 0) {
    dr = (dr * 0xFFFF) / da;
    dg = (dg * 0xFFFF) / da;
    db = (db * 0xFFFF) / da;
  }

  // Convert from 16-bit color to 8-bit color and combine the components.
  da >>= 8;
  dr >>= 8;
  dg >>= 8;
  db >>= 8;
  return (db << 0) | (dg << 8) | (dr << 16) | (da << 24);
}

static inline uint32_t  //
wuffs_base__composite_premul_nonpremul_u32_axxx(uint32_t dst_premul,
                                                uint32_t src_nonpremul) {
  // Convert from 8-bit color to 16-bit color.
  uint32_t sa = 0x101 * (0xFF & (src_nonpremul >> 24));
  uint32_t sr = 0x101 * (0xFF & (src_nonpremul >> 16));
  uint32_t sg = 0x101 * (0xFF & (src_nonpremul >> 8));
  uint32_t sb = 0x101 * (0xFF & (src_nonpremul >> 0));
  uint32_t da = 0x101 * (0xFF & (dst_premul >> 24));
  uint32_t dr = 0x101 * (0xFF & (dst_premul >> 16));
  uint32_t dg = 0x101 * (0xFF & (dst_premul >> 8));
  uint32_t db = 0x101 * (0xFF & (dst_premul >> 0));

  // Calculate the inverse of the src-alpha: how much of the dst to keep.
  uint32_t ia = 0xFFFF - sa;

  // Composite src (nonpremul) over dst (premul).
  da = sa + ((da * ia) / 0xFFFF);
  dr = ((sr * sa) + (dr * ia)) / 0xFFFF;
  dg = ((sg * sa) + (dg * ia)) / 0xFFFF;
  db = ((sb * sa) + (db * ia)) / 0xFFFF;

  // Convert from 16-bit color to 8-bit color and combine the components.
  da >>= 8;
  dr >>= 8;
  dg >>= 8;
  db >>= 8;
  return (db << 0) | (dg << 8) | (dr << 16) | (da << 24);
}

static inline uint32_t  //
wuffs_base__composite_premul_premul_u32_axxx(uint32_t dst_premul,
                                             uint32_t src_premul) {
  // Convert from 8-bit color to 16-bit color.
  uint32_t sa = 0x101 * (0xFF & (src_premul >> 24));
  uint32_t sr = 0x101 * (0xFF & (src_premul >> 16));
  uint32_t sg = 0x101 * (0xFF & (src_premul >> 8));
  uint32_t sb = 0x101 * (0xFF & (src_premul >> 0));
  uint32_t da = 0x101 * (0xFF & (dst_premul >> 24));
  uint32_t dr = 0x101 * (0xFF & (dst_premul >> 16));
  uint32_t dg = 0x101 * (0xFF & (dst_premul >> 8));
  uint32_t db = 0x101 * (0xFF & (dst_premul >> 0));

  // Calculate the inverse of the src-alpha: how much of the dst to keep.
  uint32_t ia = 0xFFFF - sa;

  // Composite src (premul) over dst (premul).
  da = sa + ((da * ia) / 0xFFFF);
  dr = sr + ((dr * ia) / 0xFFFF);
  dg = sg + ((dg * ia) / 0xFFFF);
  db = sb + ((db * ia) / 0xFFFF);

  // Convert from 16-bit color to 8-bit color and combine the components.
  da >>= 8;
  dr >>= 8;
  dg >>= 8;
  db >>= 8;
  return (db << 0) | (dg << 8) | (dr << 16) | (da << 24);
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__squash_bgr_565_888(wuffs_base__slice_u8 dst,
                                               wuffs_base__slice_u8 src) {
  size_t len4 = (dst.len < src.len ? dst.len : src.len) / 4;
  uint8_t* d = dst.ptr;
  const uint8_t* s = src.ptr;

  size_t n = len4;
  while (n--) {
    uint32_t argb = wuffs_base__load_u32le__no_bounds_check(s);
    uint32_t b5 = 0x1F & (argb >> (8 - 5));
    uint32_t g6 = 0x3F & (argb >> (16 - 6));
    uint32_t r5 = 0x1F & (argb >> (24 - 5));
    uint32_t alpha = argb & 0xFF000000;
    wuffs_base__store_u32le__no_bounds_check(
        d, alpha | (r5 << 11) | (g6 << 5) | (b5 << 0));
    s += 4;
    d += 4;
  }
  return len4 * 4;
}

static uint64_t  //
wuffs_base__pixel_swizzler__swap_rgbx_bgrx(wuffs_base__slice_u8 dst,
                                           wuffs_base__slice_u8 src) {
  size_t len4 = (dst.len < src.len ? dst.len : src.len) / 4;
  uint8_t* d = dst.ptr;
  const uint8_t* s = src.ptr;

  size_t n = len4;
  while (n--) {
    uint8_t b0 = s[0];
    uint8_t b1 = s[1];
    uint8_t b2 = s[2];
    uint8_t b3 = s[3];
    d[0] = b2;
    d[1] = b1;
    d[2] = b0;
    d[3] = b3;
    s += 4;
    d += 4;
  }
  return len4 * 4;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__copy_1_1(uint8_t* dst_ptr,
                                     size_t dst_len,
                                     uint8_t* dst_palette_ptr,
                                     size_t dst_palette_len,
                                     const uint8_t* src_ptr,
                                     size_t src_len) {
  size_t len = (dst_len < src_len) ? dst_len : src_len;
  if (len > 0) {
    memmove(dst_ptr, src_ptr, len);
  }
  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__copy_3_3(uint8_t* dst_ptr,
                                     size_t dst_len,
                                     uint8_t* dst_palette_ptr,
                                     size_t dst_palette_len,
                                     const uint8_t* src_ptr,
                                     size_t src_len) {
  size_t dst_len3 = dst_len / 3;
  size_t src_len3 = src_len / 3;
  size_t len = (dst_len3 < src_len3) ? dst_len3 : src_len3;
  if (len > 0) {
    memmove(dst_ptr, src_ptr, len * 3);
  }
  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__copy_4_4(uint8_t* dst_ptr,
                                     size_t dst_len,
                                     uint8_t* dst_palette_ptr,
                                     size_t dst_palette_len,
                                     const uint8_t* src_ptr,
                                     size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len4 < src_len4) ? dst_len4 : src_len4;
  if (len > 0) {
    memmove(dst_ptr, src_ptr, len * 4);
  }
  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__bgr(uint8_t* dst_ptr,
                                         size_t dst_len,
                                         uint8_t* dst_palette_ptr,
                                         size_t dst_palette_len,
                                         const uint8_t* src_ptr,
                                         size_t src_len) {
  size_t dst_len2 = dst_len / 2;
  size_t src_len3 = src_len / 3;
  size_t len = (dst_len2 < src_len3) ? dst_len2 : src_len3;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t b5 = s[0] >> 3;
    uint32_t g6 = s[1] >> 2;
    uint32_t r5 = s[2] >> 3;
    uint32_t rgb_565 = (r5 << 11) | (g6 << 5) | (b5 << 0);
    wuffs_base__store_u16le__no_bounds_check(d + (0 * 2), (uint16_t)rgb_565);

    s += 1 * 3;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__bgra_nonpremul__src(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len2 = dst_len / 2;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len2 < src_len4) ? dst_len2 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    wuffs_base__store_u16le__no_bounds_check(
        d + (0 * 2),
        wuffs_base__color_u32_argb_premul__as__color_u16_rgb_565(
            wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
                wuffs_base__load_u32le__no_bounds_check(s + (0 * 4)))));

    s += 1 * 4;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__bgra_nonpremul__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len2 = dst_len / 2;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len2 < src_len4) ? dst_len2 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    // Convert from 8-bit color to 16-bit color.
    uint32_t sa = 0x101 * ((uint32_t)s[3]);
    uint32_t sr = 0x101 * ((uint32_t)s[2]);
    uint32_t sg = 0x101 * ((uint32_t)s[1]);
    uint32_t sb = 0x101 * ((uint32_t)s[0]);

    // Convert from 565 color to 16-bit color.
    uint32_t old_rgb_565 = wuffs_base__load_u16le__no_bounds_check(d + (0 * 2));
    uint32_t old_r5 = 0x1F & (old_rgb_565 >> 11);
    uint32_t dr = (0x8421 * old_r5) >> 4;
    uint32_t old_g6 = 0x3F & (old_rgb_565 >> 5);
    uint32_t dg = (0x1041 * old_g6) >> 2;
    uint32_t old_b5 = 0x1F & (old_rgb_565 >> 0);
    uint32_t db = (0x8421 * old_b5) >> 4;

    // Calculate the inverse of the src-alpha: how much of the dst to keep.
    uint32_t ia = 0xFFFF - sa;

    // Composite src (nonpremul) over dst (premul).
    dr = ((sr * sa) + (dr * ia)) / 0xFFFF;
    dg = ((sg * sa) + (dg * ia)) / 0xFFFF;
    db = ((sb * sa) + (db * ia)) / 0xFFFF;

    // Convert from 16-bit color to 565 color and combine the components.
    uint32_t new_r5 = 0x1F & (dr >> 11);
    uint32_t new_g6 = 0x3F & (dg >> 10);
    uint32_t new_b5 = 0x1F & (db >> 11);
    uint32_t new_rgb_565 = (new_r5 << 11) | (new_g6 << 5) | (new_b5 << 0);
    wuffs_base__store_u16le__no_bounds_check(d + (0 * 2),
                                             (uint16_t)new_rgb_565);

    s += 1 * 4;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__y(uint8_t* dst_ptr,
                                       size_t dst_len,
                                       uint8_t* dst_palette_ptr,
                                       size_t dst_palette_len,
                                       const uint8_t* src_ptr,
                                       size_t src_len) {
  size_t dst_len2 = dst_len / 2;
  size_t len = (dst_len2 < src_len) ? dst_len2 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t y5 = s[0] >> 3;
    uint32_t y6 = s[0] >> 2;
    uint32_t rgb_565 = (y5 << 11) | (y6 << 5) | (y5 << 0);
    wuffs_base__store_u16le__no_bounds_check(d + (0 * 2), (uint16_t)rgb_565);

    s += 1 * 1;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__index__src(uint8_t* dst_ptr,
                                                size_t dst_len,
                                                uint8_t* dst_palette_ptr,
                                                size_t dst_palette_len,
                                                const uint8_t* src_ptr,
                                                size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len2 = dst_len / 2;
  size_t len = (dst_len2 < src_len) ? dst_len2 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    wuffs_base__store_u16le__no_bounds_check(
        d + (0 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (1 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (2 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (3 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 2;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    wuffs_base__store_u16le__no_bounds_check(
        d + (0 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[0] * 4)));

    s += 1 * 1;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__index_binary_alpha__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len2 = dst_len / 2;
  size_t len = (dst_len2 < src_len) ? dst_len2 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u16le__no_bounds_check(d + (0 * 2), (uint16_t)s0);
    }

    s += 1 * 1;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__bgr__bgra_nonpremul__src(uint8_t* dst_ptr,
                                                     size_t dst_len,
                                                     uint8_t* dst_palette_ptr,
                                                     size_t dst_palette_len,
                                                     const uint8_t* src_ptr,
                                                     size_t src_len) {
  size_t dst_len3 = dst_len / 3;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len3 < src_len4) ? dst_len3 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t s0 =
        wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
            wuffs_base__load_u32le__no_bounds_check(s + (0 * 4)));
    wuffs_base__store_u24le__no_bounds_check(d + (0 * 3), s0);

    s += 1 * 4;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr__bgra_nonpremul__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len3 = dst_len / 3;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len3 < src_len4) ? dst_len3 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    // Convert from 8-bit color to 16-bit color.
    uint32_t sa = 0x101 * ((uint32_t)s[3]);
    uint32_t sr = 0x101 * ((uint32_t)s[2]);
    uint32_t sg = 0x101 * ((uint32_t)s[1]);
    uint32_t sb = 0x101 * ((uint32_t)s[0]);
    uint32_t dr = 0x101 * ((uint32_t)d[2]);
    uint32_t dg = 0x101 * ((uint32_t)d[1]);
    uint32_t db = 0x101 * ((uint32_t)d[0]);

    // Calculate the inverse of the src-alpha: how much of the dst to keep.
    uint32_t ia = 0xFFFF - sa;

    // Composite src (nonpremul) over dst (premul).
    dr = ((sr * sa) + (dr * ia)) / 0xFFFF;
    dg = ((sg * sa) + (dg * ia)) / 0xFFFF;
    db = ((sb * sa) + (db * ia)) / 0xFFFF;

    // Convert from 16-bit color to 8-bit color.
    d[0] = (uint8_t)(db >> 8);
    d[1] = (uint8_t)(dg >> 8);
    d[2] = (uint8_t)(dr >> 8);

    s += 1 * 4;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__bgra_nonpremul__bgra_nonpremul__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len4 < src_len4) ? dst_len4 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t d0 = wuffs_base__load_u32le__no_bounds_check(d + (0 * 4));
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(s + (0 * 4));
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4),
        wuffs_base__composite_nonpremul_nonpremul_u32_axxx(d0, s0));

    s += 1 * 4;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len4 < src_len4) ? dst_len4 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(s + (0 * 4));
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4),
        wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(s0));

    s += 1 * 4;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t src_len4 = src_len / 4;
  size_t len = (dst_len4 < src_len4) ? dst_len4 : src_len4;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t d0 = wuffs_base__load_u32le__no_bounds_check(d + (0 * 4));
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(s + (0 * 4));
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), wuffs_base__composite_premul_nonpremul_u32_axxx(d0, s0));

    s += 1 * 4;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__xxx__index__src(uint8_t* dst_ptr,
                                            size_t dst_len,
                                            uint8_t* dst_palette_ptr,
                                            size_t dst_palette_len,
                                            const uint8_t* src_ptr,
                                            size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len3 = dst_len / 3;
  size_t len = (dst_len3 < src_len) ? dst_len3 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  // The comparison in the while condition is ">", not ">=", because with
  // ">=", the last 4-byte store could write past the end of the dst slice.
  //
  // Each 4-byte store writes one too many bytes, but a subsequent store
  // will overwrite that with the correct byte. There is always another
  // store, whether a 4-byte store in this loop or a 1-byte store in the
  // next loop.
  while (n > loop_unroll_count) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (1 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (2 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (3 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 3;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    wuffs_base__store_u24le__no_bounds_check(d + (0 * 3), s0);

    s += 1 * 1;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxx__index_binary_alpha__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len3 = dst_len / 3;
  size_t len = (dst_len3 < src_len) ? dst_len3 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u24le__no_bounds_check(d + (0 * 3), s0);
    }
    uint32_t s1 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[1] * 4));
    if (s1) {
      wuffs_base__store_u24le__no_bounds_check(d + (1 * 3), s1);
    }
    uint32_t s2 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[2] * 4));
    if (s2) {
      wuffs_base__store_u24le__no_bounds_check(d + (2 * 3), s2);
    }
    uint32_t s3 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[3] * 4));
    if (s3) {
      wuffs_base__store_u24le__no_bounds_check(d + (3 * 3), s3);
    }

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 3;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u24le__no_bounds_check(d + (0 * 3), s0);
    }

    s += 1 * 1;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxx__y(uint8_t* dst_ptr,
                                   size_t dst_len,
                                   uint8_t* dst_palette_ptr,
                                   size_t dst_palette_len,
                                   const uint8_t* src_ptr,
                                   size_t src_len) {
  size_t dst_len3 = dst_len / 3;
  size_t len = (dst_len3 < src_len) ? dst_len3 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint8_t s0 = s[0];
    d[0] = s0;
    d[1] = s0;
    d[2] = s0;

    s += 1 * 1;
    d += 1 * 3;
    n -= 1;
  }

  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__index__src(uint8_t* dst_ptr,
                                             size_t dst_len,
                                             uint8_t* dst_palette_ptr,
                                             size_t dst_palette_len,
                                             const uint8_t* src_ptr,
                                             size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len4 = dst_len / 4;
  size_t len = (dst_len4 < src_len) ? dst_len4 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (1 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (2 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (3 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 4;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette_ptr + ((size_t)s[0] * 4)));

    s += 1 * 1;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__index_binary_alpha__src_over(
    uint8_t* dst_ptr,
    size_t dst_len,
    uint8_t* dst_palette_ptr,
    size_t dst_palette_len,
    const uint8_t* src_ptr,
    size_t src_len) {
  if (dst_palette_len != 1024) {
    return 0;
  }
  size_t dst_len4 = dst_len / 4;
  size_t len = (dst_len4 < src_len) ? dst_len4 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u32le__no_bounds_check(d + (0 * 4), s0);
    }
    uint32_t s1 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[1] * 4));
    if (s1) {
      wuffs_base__store_u32le__no_bounds_check(d + (1 * 4), s1);
    }
    uint32_t s2 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[2] * 4));
    if (s2) {
      wuffs_base__store_u32le__no_bounds_check(d + (2 * 4), s2);
    }
    uint32_t s3 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[3] * 4));
    if (s3) {
      wuffs_base__store_u32le__no_bounds_check(d + (3 * 4), s3);
    }

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 4;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette_ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u32le__no_bounds_check(d + (0 * 4), s0);
    }

    s += 1 * 1;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__xxx(uint8_t* dst_ptr,
                                      size_t dst_len,
                                      uint8_t* dst_palette_ptr,
                                      size_t dst_palette_len,
                                      const uint8_t* src_ptr,
                                      size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t src_len3 = src_len / 3;
  size_t len = (dst_len4 < src_len3) ? dst_len4 : src_len3;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4),
        0xFF000000 | wuffs_base__load_u24le__no_bounds_check(s + (0 * 3)));

    s += 1 * 3;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__y(uint8_t* dst_ptr,
                                    size_t dst_len,
                                    uint8_t* dst_palette_ptr,
                                    size_t dst_palette_len,
                                    const uint8_t* src_ptr,
                                    size_t src_len) {
  size_t dst_len4 = dst_len / 4;
  size_t len = (dst_len4 < src_len) ? dst_len4 : src_len;
  uint8_t* d = dst_ptr;
  const uint8_t* s = src_ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), 0xFF000000 | (0x010101 * (uint32_t)s[0]));

    s += 1 * 1;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

// --------

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__y(wuffs_base__pixel_swizzler* p,
                                       wuffs_base__pixel_format dst_pixfmt,
                                       wuffs_base__slice_u8 dst_palette,
                                       wuffs_base__slice_u8 src_palette,
                                       wuffs_base__pixel_blend blend) {
  switch (dst_pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      return wuffs_base__pixel_swizzler__bgr_565__y;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      return wuffs_base__pixel_swizzler__xxx__y;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      return wuffs_base__pixel_swizzler__xxxx__y;
  }
  return NULL;
}

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__indexed__bgra_binary(
    wuffs_base__pixel_swizzler* p,
    wuffs_base__pixel_format dst_pixfmt,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src_palette,
    wuffs_base__pixel_blend blend) {
  switch (dst_pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
      if (wuffs_base__slice_u8__copy_from_slice(dst_palette, src_palette) !=
          1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__copy_1_1;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      if (wuffs_base__pixel_swizzler__squash_bgr_565_888(dst_palette,
                                                         src_palette) != 1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__bgr_565__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__bgr_565__index_binary_alpha__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      if (wuffs_base__slice_u8__copy_from_slice(dst_palette, src_palette) !=
          1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxx__index_binary_alpha__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
      if (wuffs_base__slice_u8__copy_from_slice(dst_palette, src_palette) !=
          1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxxx__index_binary_alpha__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      if (wuffs_base__pixel_swizzler__swap_rgbx_bgrx(dst_palette,
                                                     src_palette) != 1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxx__index_binary_alpha__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
      if (wuffs_base__pixel_swizzler__swap_rgbx_bgrx(dst_palette,
                                                     src_palette) != 1024) {
        return NULL;
      }
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__xxxx__index__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__xxxx__index_binary_alpha__src_over;
      }
      return NULL;
  }
  return NULL;
}

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__bgr(wuffs_base__pixel_swizzler* p,
                                         wuffs_base__pixel_format dst_pixfmt,
                                         wuffs_base__slice_u8 dst_palette,
                                         wuffs_base__slice_u8 src_palette,
                                         wuffs_base__pixel_blend blend) {
  switch (dst_pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      return wuffs_base__pixel_swizzler__bgr_565__bgr;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      return wuffs_base__pixel_swizzler__copy_3_3;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      return wuffs_base__pixel_swizzler__xxxx__xxx;

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      // TODO.
      break;
  }
  return NULL;
}

static wuffs_base__pixel_swizzler__func  //
wuffs_base__pixel_swizzler__prepare__bgra_nonpremul(
    wuffs_base__pixel_swizzler* p,
    wuffs_base__pixel_format dst_pixfmt,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src_palette,
    wuffs_base__pixel_blend blend) {
  switch (dst_pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__bgr_565__bgra_nonpremul__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__bgr_565__bgra_nonpremul__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__bgr__bgra_nonpremul__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__bgr__bgra_nonpremul__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__copy_4_4;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__bgra_nonpremul__bgra_nonpremul__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
      switch (blend) {
        case WUFFS_BASE__PIXEL_BLEND__SRC:
          return wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src;
        case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
          return wuffs_base__pixel_swizzler__bgra_premul__bgra_nonpremul__src_over;
      }
      return NULL;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      // TODO.
      break;

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      // TODO.
      break;
  }
  return NULL;
}

// --------

WUFFS_BASE__MAYBE_STATIC wuffs_base__status  //
wuffs_base__pixel_swizzler__prepare(wuffs_base__pixel_swizzler* p,
                                    wuffs_base__pixel_format dst_pixfmt,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__pixel_format src_pixfmt,
                                    wuffs_base__slice_u8 src_palette,
                                    wuffs_base__pixel_blend blend) {
  if (!p) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  p->private_impl.func = NULL;
  p->private_impl.src_pixfmt_bytes_per_pixel = 0;

  wuffs_base__pixel_swizzler__func func = NULL;
  uint32_t src_pixfmt_bits_per_pixel =
      wuffs_base__pixel_format__bits_per_pixel(&src_pixfmt);
  if ((src_pixfmt_bits_per_pixel == 0) ||
      ((src_pixfmt_bits_per_pixel & 7) != 0)) {
    return wuffs_base__make_status(
        wuffs_base__error__unsupported_pixel_swizzler_option);
  }

  // TODO: support many more formats.

  switch (src_pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__Y:
      func = wuffs_base__pixel_swizzler__prepare__y(p, dst_pixfmt, dst_palette,
                                                    src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
      func = wuffs_base__pixel_swizzler__prepare__indexed__bgra_binary(
          p, dst_pixfmt, dst_palette, src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      func = wuffs_base__pixel_swizzler__prepare__bgr(
          p, dst_pixfmt, dst_palette, src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      func = wuffs_base__pixel_swizzler__prepare__bgra_nonpremul(
          p, dst_pixfmt, dst_palette, src_palette, blend);
      break;
  }

  p->private_impl.func = func;
  p->private_impl.src_pixfmt_bytes_per_pixel = src_pixfmt_bits_per_pixel / 8;
  return wuffs_base__make_status(
      func ? NULL : wuffs_base__error__unsupported_pixel_swizzler_option);
}

WUFFS_BASE__MAYBE_STATIC uint64_t  //
wuffs_base__pixel_swizzler__swizzle_interleaved_from_reader(
    const wuffs_base__pixel_swizzler* p,
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    const uint8_t** ptr_iop_r,
    const uint8_t* io2_r) {
  if (p && p->private_impl.func) {
    const uint8_t* iop_r = *ptr_iop_r;
    uint64_t n = (*p->private_impl.func)(dst.ptr, dst.len, dst_palette.ptr,
                                         dst_palette.len, iop_r,
                                         (size_t)(io2_r - iop_r));
    *ptr_iop_r += n * p->private_impl.src_pixfmt_bytes_per_pixel;
    return n;
  }
  return 0;
}

WUFFS_BASE__MAYBE_STATIC uint64_t  //
wuffs_base__pixel_swizzler__swizzle_interleaved_from_slice(
    const wuffs_base__pixel_swizzler* p,
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  if (p && p->private_impl.func) {
    return (*p->private_impl.func)(dst.ptr, dst.len, dst_palette.ptr,
                                   dst_palette.len, src.ptr, src.len);
  }
  return 0;
}
