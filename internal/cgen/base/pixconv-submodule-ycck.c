// Copyright 2023 The Wuffs Authors.
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

// --------

static inline uint32_t  //
wuffs_base__u32__max_of_4(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
  return wuffs_base__u32__max(     //
      wuffs_base__u32__max(a, b),  //
      wuffs_base__u32__max(c, d));
}

static inline uint32_t  //
wuffs_base__u32__min_of_5(uint32_t a,
                          uint32_t b,
                          uint32_t c,
                          uint32_t d,
                          uint32_t e) {
  return wuffs_base__u32__min(          //
      wuffs_base__u32__min(             //
          wuffs_base__u32__min(a, b),   //
          wuffs_base__u32__min(c, d)),  //
      e);
}

// Preconditions: see all the checks made in
// wuffs_base__pixel_swizzler__swizzle_ycck before calling this function. For
// example, (width > 0) is a precondition, but there are many more.
static void  //
wuffs_base__pixel_swizzler__swizzle_ycc__general__box_filter(
    const wuffs_base__pixel_swizzler* p,
    wuffs_base__pixel_buffer* dst,
    wuffs_base__slice_u8 dst_palette,
    uint32_t width,
    uint32_t height,
    const uint8_t* src_ptr0,
    const uint8_t* src_ptr1,
    const uint8_t* src_ptr2,
    uint32_t stride0,
    uint32_t stride1,
    uint32_t stride2,
    uint32_t h0_out_of_12,
    uint32_t h1_out_of_12,
    uint32_t h2_out_of_12,
    uint32_t v0_out_of_12,
    uint32_t v1_out_of_12,
    uint32_t v2_out_of_12) {
  uint32_t iy0 = 0;
  uint32_t iy1 = 0;
  uint32_t iy2 = 0;
  uint32_t y = 0;
  while (true) {
    const uint8_t* src_iter0 = src_ptr0;
    const uint8_t* src_iter1 = src_ptr1;
    const uint8_t* src_iter2 = src_ptr2;

    uint32_t ix0 = 0;
    uint32_t ix1 = 0;
    uint32_t ix2 = 0;
    uint32_t x = 0;
    while (true) {
      wuffs_base__pixel_buffer__set_color_u32_at(
          dst, x, y,
          wuffs_base__color_ycc__as__color_u32(*src_iter0, *src_iter1,
                                               *src_iter2));

      if ((x + 1) == width) {
        break;
      }
      x = x + 1;
      ix0 += h0_out_of_12;
      if (ix0 >= 12) {
        ix0 = 0;
        src_iter0++;
      }
      ix1 += h1_out_of_12;
      if (ix1 >= 12) {
        ix1 = 0;
        src_iter1++;
      }
      ix2 += h2_out_of_12;
      if (ix2 >= 12) {
        ix2 = 0;
        src_iter2++;
      }
    }

    if ((y + 1) == height) {
      break;
    }
    y = y + 1;
    iy0 += v0_out_of_12;
    if (iy0 >= 12) {
      iy0 = 0;
      src_ptr0 += stride0;
    }
    iy1 += v1_out_of_12;
    if (iy1 >= 12) {
      iy1 = 0;
      src_ptr1 += stride1;
    }
    iy2 += v2_out_of_12;
    if (iy2 >= 12) {
      iy2 = 0;
      src_ptr2 += stride2;
    }
  }
}

// wuffs_base__pixel_swizzler__flattened_length is like
// wuffs_base__table__flattened_length but returns uint64_t (not size_t) and
// also accounts for subsampling.
static uint64_t  //
wuffs_base__pixel_swizzler__flattened_length(uint32_t width,
                                             uint32_t height,
                                             uint32_t stride,
                                             uint32_t inv_h,
                                             uint32_t inv_v) {
  uint64_t scaled_width = (((uint64_t)width) + (inv_h - 1)) / inv_h;
  uint64_t scaled_height = (((uint64_t)height) + (inv_v - 1)) / inv_v;
  if (scaled_height <= 0) {
    return 0;
  }
  return ((scaled_height - 1) * stride) + scaled_width;
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__status  //
wuffs_base__pixel_swizzler__swizzle_ycck(const wuffs_base__pixel_swizzler* p,
                                         wuffs_base__pixel_buffer* dst,
                                         wuffs_base__slice_u8 dst_palette,
                                         uint32_t width,
                                         uint32_t height,
                                         wuffs_base__slice_u8 src0,
                                         wuffs_base__slice_u8 src1,
                                         wuffs_base__slice_u8 src2,
                                         wuffs_base__slice_u8 src3,
                                         uint32_t width0,
                                         uint32_t width1,
                                         uint32_t width2,
                                         uint32_t width3,
                                         uint32_t height0,
                                         uint32_t height1,
                                         uint32_t height2,
                                         uint32_t height3,
                                         uint32_t stride0,
                                         uint32_t stride1,
                                         uint32_t stride2,
                                         uint32_t stride3,
                                         uint8_t h0,
                                         uint8_t h1,
                                         uint8_t h2,
                                         uint8_t h3,
                                         uint8_t v0,
                                         uint8_t v1,
                                         uint8_t v2,
                                         uint8_t v3,
                                         bool triangle_filter_for_2to1) {
  if (!p) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  } else if ((h3 != 0) || (v3 != 0) || triangle_filter_for_2to1) {
    // TODO: support the K in YCCK and support triangle_filter_for_2to1.
    return wuffs_base__make_status(
        wuffs_base__error__unsupported_pixel_swizzler_option);
  } else if (!dst || (width > 0xFFFF) || (height > 0xFFFF) ||  //
             (4 <= (h0 - 1)) || (4 <= (v0 - 1)) ||             //
             (4 <= (h1 - 1)) || (4 <= (v1 - 1)) ||             //
             (4 <= (h2 - 1)) || (4 <= (v2 - 1))) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }

  uint32_t max_incl_h = wuffs_base__u32__max_of_4(h0, h1, h2, h3);
  uint32_t max_incl_v = wuffs_base__u32__max_of_4(v0, v1, v2, v3);
  uint32_t inv_h0 = max_incl_h / h0;
  uint32_t inv_h1 = max_incl_h / h1;
  uint32_t inv_h2 = max_incl_h / h2;
  uint32_t inv_v0 = max_incl_v / v0;
  uint32_t inv_v1 = max_incl_v / v1;
  uint32_t inv_v2 = max_incl_v / v2;
  width = wuffs_base__u32__min_of_5(  //
      width,                          //
      width0 * inv_h0,                //
      width1 * inv_h1,                //
      width2 * inv_h2,                //
      wuffs_base__pixel_config__width(&dst->pixcfg));
  height = wuffs_base__u32__min_of_5(  //
      height,                          //
      height0 * inv_v0,                //
      height1 * inv_v1,                //
      height2 * inv_v2,                //
      wuffs_base__pixel_config__height(&dst->pixcfg));

  if (((h0 * inv_h0) != max_incl_h) ||  //
      ((h1 * inv_h1) != max_incl_h) ||  //
      ((h2 * inv_h2) != max_incl_h) ||  //
      ((v0 * inv_v0) != max_incl_v) ||  //
      ((v1 * inv_v1) != max_incl_v) ||  //
      ((v2 * inv_v2) != max_incl_v) ||  //
      (src0.len < wuffs_base__pixel_swizzler__flattened_length(
                      width, height, stride0, inv_h0, inv_v0)) ||
      (src1.len < wuffs_base__pixel_swizzler__flattened_length(
                      width, height, stride1, inv_h1, inv_v1)) ||
      (src2.len < wuffs_base__pixel_swizzler__flattened_length(
                      width, height, stride2, inv_h2, inv_v2))) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }

  if (wuffs_base__pixel_format__is_planar(&dst->pixcfg.private_impl.pixfmt)) {
    // TODO: see wuffs_base__pixel_buffer__set_color_u32_at's TODO.
    return wuffs_base__make_status(
        wuffs_base__error__unsupported_pixel_swizzler_option);
  }

  switch (dst->pixcfg.private_impl.pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__Y:
    case WUFFS_BASE__PIXEL_FORMAT__Y_16LE:
    case WUFFS_BASE__PIXEL_FORMAT__Y_16BE:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
    case WUFFS_BASE__PIXEL_FORMAT__BGR:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL_4X16LE:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
    case WUFFS_BASE__PIXEL_FORMAT__RGB:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      break;

    default:
      // TODO: see wuffs_base__pixel_buffer__set_color_u32_at's TODO.
      return wuffs_base__make_status(
          wuffs_base__error__unsupported_pixel_swizzler_option);
  }

  if ((width <= 0) || (height <= 0)) {
    return wuffs_base__make_status(NULL);
  }

  wuffs_base__pixel_swizzler__swizzle_ycc__general__box_filter(
      p, dst, dst_palette, width, height,  //
      src0.ptr, src1.ptr, src2.ptr,        //
      stride0, stride1, stride2,           //
      (h0 * 12) / max_incl_h,              //
      (h1 * 12) / max_incl_h,              //
      (h2 * 12) / max_incl_h,              //
      (v0 * 12) / max_incl_v,              //
      (v1 * 12) / max_incl_v,              //
      (v2 * 12) / max_incl_v);
  return wuffs_base__make_status(NULL);
}
