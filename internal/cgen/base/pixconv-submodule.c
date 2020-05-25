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
  uint8_t* s = src.ptr;

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
  uint8_t* s = src.ptr;

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
wuffs_base__pixel_swizzler__copy_1_1(wuffs_base__slice_u8 dst,
                                     wuffs_base__slice_u8 dst_palette,
                                     wuffs_base__slice_u8 src) {
  return wuffs_base__slice_u8__copy_from_slice(dst, src);
}

static uint64_t  //
wuffs_base__pixel_swizzler__copy_3_3(wuffs_base__slice_u8 dst,
                                     wuffs_base__slice_u8 dst_palette,
                                     wuffs_base__slice_u8 src) {
  size_t dst_len3 = dst.len / 3;
  size_t src_len3 = src.len / 3;
  size_t len = dst_len3 < src_len3 ? dst_len3 : src_len3;
  if (len > 0) {
    memmove(dst.ptr, src.ptr, len * 3);
  }
  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__copy_4_4(wuffs_base__slice_u8 dst,
                                     wuffs_base__slice_u8 dst_palette,
                                     wuffs_base__slice_u8 src) {
  size_t dst_len4 = dst.len / 4;
  size_t src_len4 = src.len / 4;
  size_t len = dst_len4 < src_len4 ? dst_len4 : src_len4;
  if (len > 0) {
    memmove(dst.ptr, src.ptr, len * 4);
  }
  return len;
}

// --------

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__bgr(wuffs_base__slice_u8 dst,
                                         wuffs_base__slice_u8 dst_palette,
                                         wuffs_base__slice_u8 src) {
  size_t dst_len2 = dst.len / 2;
  size_t src_len3 = src.len / 3;
  size_t len = dst_len2 < src_len3 ? dst_len2 : src_len3;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  size_t dst_len2 = dst.len / 2;
  size_t src_len4 = src.len / 4;
  size_t len = dst_len2 < src_len4 ? dst_len2 : src_len4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  size_t dst_len2 = dst.len / 2;
  size_t src_len4 = src.len / 4;
  size_t len = dst_len2 < src_len4 ? dst_len2 : src_len4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
wuffs_base__pixel_swizzler__bgr_565__y(wuffs_base__slice_u8 dst,
                                       wuffs_base__slice_u8 dst_palette,
                                       wuffs_base__slice_u8 src) {
  size_t dst_len2 = dst.len / 2;
  size_t len = dst_len2 < src.len ? dst_len2 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
wuffs_base__pixel_swizzler__bgr_565__index__src(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len2 = dst.len / 2;
  size_t len = dst_len2 < src.len ? dst_len2 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    wuffs_base__store_u16le__no_bounds_check(
        d + (0 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (1 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (2 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u16le__no_bounds_check(
        d + (3 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 2;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    wuffs_base__store_u16le__no_bounds_check(
        d + (0 * 2), wuffs_base__load_u16le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[0] * 4)));

    s += 1 * 1;
    d += 1 * 2;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__bgr_565__index_binary_alpha__src_over(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len2 = dst.len / 2;
  size_t len = dst_len2 < src.len ? dst_len2 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  // TODO: unroll.

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
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
wuffs_base__pixel_swizzler__bgr__bgra_nonpremul__src(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  size_t dst_len3 = dst.len / 3;
  size_t src_len4 = src.len / 4;
  size_t len = dst_len3 < src_len4 ? dst_len3 : src_len4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  size_t dst_len3 = dst.len / 3;
  size_t src_len4 = src.len / 4;
  size_t len = dst_len3 < src_len4 ? dst_len3 : src_len4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  size_t dst_len4 = dst.len / 4;
  size_t src_len4 = src.len / 4;
  size_t len = dst_len4 < src_len4 ? dst_len4 : src_len4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  size_t dst_len4 = dst.len / 4;
  size_t src_len4 = src.len / 4;
  size_t len = dst_len4 < src_len4 ? dst_len4 : src_len4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  size_t dst_len4 = dst.len / 4;
  size_t src_len4 = src.len / 4;
  size_t len = dst_len4 < src_len4 ? dst_len4 : src_len4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
wuffs_base__pixel_swizzler__xxx__index__src(wuffs_base__slice_u8 dst,
                                            wuffs_base__slice_u8 dst_palette,
                                            wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len3 = dst.len / 3;
  size_t len = dst_len3 < src.len ? dst_len3 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
                         dst_palette.ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (1 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (2 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (3 * 3), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 3;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
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
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len3 = dst.len / 3;
  size_t len = dst_len3 < src.len ? dst_len3 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u24le__no_bounds_check(d + (0 * 3), s0);
    }
    uint32_t s1 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[1] * 4));
    if (s1) {
      wuffs_base__store_u24le__no_bounds_check(d + (1 * 3), s1);
    }
    uint32_t s2 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[2] * 4));
    if (s2) {
      wuffs_base__store_u24le__no_bounds_check(d + (2 * 3), s2);
    }
    uint32_t s3 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[3] * 4));
    if (s3) {
      wuffs_base__store_u24le__no_bounds_check(d + (3 * 3), s3);
    }

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 3;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
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
wuffs_base__pixel_swizzler__xxx__y(wuffs_base__slice_u8 dst,
                                   wuffs_base__slice_u8 dst_palette,
                                   wuffs_base__slice_u8 src) {
  size_t dst_len3 = dst.len / 3;
  size_t len = dst_len3 < src.len ? dst_len3 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
wuffs_base__pixel_swizzler__xxxx__index__src(wuffs_base__slice_u8 dst,
                                             wuffs_base__slice_u8 dst_palette,
                                             wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len4 = dst.len / 4;
  size_t len = dst_len4 < src.len ? dst_len4 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[0] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (1 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[1] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (2 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[2] * 4)));
    wuffs_base__store_u32le__no_bounds_check(
        d + (3 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[3] * 4)));

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 4;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    wuffs_base__store_u32le__no_bounds_check(
        d + (0 * 4), wuffs_base__load_u32le__no_bounds_check(
                         dst_palette.ptr + ((size_t)s[0] * 4)));

    s += 1 * 1;
    d += 1 * 4;
    n -= 1;
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__xxxx__index_binary_alpha__src_over(
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len4 = dst.len / 4;
  size_t len = dst_len4 < src.len ? dst_len4 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  const size_t loop_unroll_count = 4;

  while (n >= loop_unroll_count) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[0] * 4));
    if (s0) {
      wuffs_base__store_u32le__no_bounds_check(d + (0 * 4), s0);
    }
    uint32_t s1 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[1] * 4));
    if (s1) {
      wuffs_base__store_u32le__no_bounds_check(d + (1 * 4), s1);
    }
    uint32_t s2 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[2] * 4));
    if (s2) {
      wuffs_base__store_u32le__no_bounds_check(d + (2 * 4), s2);
    }
    uint32_t s3 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
                                                          ((size_t)s[3] * 4));
    if (s3) {
      wuffs_base__store_u32le__no_bounds_check(d + (3 * 4), s3);
    }

    s += loop_unroll_count * 1;
    d += loop_unroll_count * 4;
    n -= loop_unroll_count;
  }

  while (n >= 1) {
    uint32_t s0 = wuffs_base__load_u32le__no_bounds_check(dst_palette.ptr +
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
wuffs_base__pixel_swizzler__xxxx__xxx(wuffs_base__slice_u8 dst,
                                      wuffs_base__slice_u8 dst_palette,
                                      wuffs_base__slice_u8 src) {
  size_t dst_len4 = dst.len / 4;
  size_t src_len3 = src.len / 3;
  size_t len = dst_len4 < src_len3 ? dst_len4 : src_len3;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
wuffs_base__pixel_swizzler__xxxx__y(wuffs_base__slice_u8 dst,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__slice_u8 src) {
  size_t dst_len4 = dst.len / 4;
  size_t len = dst_len4 < src.len ? dst_len4 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
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
                                       wuffs_base__pixel_format dst_format,
                                       wuffs_base__slice_u8 dst_palette,
                                       wuffs_base__slice_u8 src_palette,
                                       wuffs_base__pixel_blend blend) {
  switch (dst_format.repr) {
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
    wuffs_base__pixel_format dst_format,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src_palette,
    wuffs_base__pixel_blend blend) {
  switch (dst_format.repr) {
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
                                         wuffs_base__pixel_format dst_format,
                                         wuffs_base__slice_u8 dst_palette,
                                         wuffs_base__slice_u8 src_palette,
                                         wuffs_base__pixel_blend blend) {
  switch (dst_format.repr) {
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
    wuffs_base__pixel_format dst_format,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src_palette,
    wuffs_base__pixel_blend blend) {
  switch (dst_format.repr) {
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

wuffs_base__status  //
wuffs_base__pixel_swizzler__prepare(wuffs_base__pixel_swizzler* p,
                                    wuffs_base__pixel_format dst_format,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__pixel_format src_format,
                                    wuffs_base__slice_u8 src_palette,
                                    wuffs_base__pixel_blend blend) {
  if (!p) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }

  // TODO: support many more formats.

  wuffs_base__pixel_swizzler__func func = NULL;

  switch (src_format.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__Y:
      func = wuffs_base__pixel_swizzler__prepare__y(p, dst_format, dst_palette,
                                                    src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
      func = wuffs_base__pixel_swizzler__prepare__indexed__bgra_binary(
          p, dst_format, dst_palette, src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      func = wuffs_base__pixel_swizzler__prepare__bgr(
          p, dst_format, dst_palette, src_palette, blend);
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      func = wuffs_base__pixel_swizzler__prepare__bgra_nonpremul(
          p, dst_format, dst_palette, src_palette, blend);
      break;
  }

  p->private_impl.func = func;
  return wuffs_base__make_status(
      func ? NULL : wuffs_base__error__unsupported_pixel_swizzler_option);
}

uint64_t  //
wuffs_base__pixel_swizzler__swizzle_interleaved(
    const wuffs_base__pixel_swizzler* p,
    wuffs_base__slice_u8 dst,
    wuffs_base__slice_u8 dst_palette,
    wuffs_base__slice_u8 src) {
  if (p && p->private_impl.func) {
    return (*p->private_impl.func)(dst, dst_palette, src);
  }
  return 0;
}
