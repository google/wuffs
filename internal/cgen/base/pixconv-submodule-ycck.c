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

#if defined(WUFFS_BASE__CPU_ARCH__X86_FAMILY)
WUFFS_BASE__MAYBE_ATTRIBUTE_TARGET("pclmul,popcnt,sse4.2,avx2")
static void  //
wuffs_base__pixel_swizzler__swizzle_ycc__convert_bgrx_x86_avx2(
    wuffs_base__pixel_buffer* dst,
    uint32_t x,
    uint32_t x_end,
    uint32_t y,
    const uint8_t* up0,
    const uint8_t* up1,
    const uint8_t* up2);

WUFFS_BASE__MAYBE_ATTRIBUTE_TARGET("pclmul,popcnt,sse4.2,avx2")
static void  //
wuffs_base__pixel_swizzler__swizzle_ycc__convert_rgbx_x86_avx2(
    wuffs_base__pixel_buffer* dst,
    uint32_t x,
    uint32_t x_end,
    uint32_t y,
    const uint8_t* up0,
    const uint8_t* up1,
    const uint8_t* up2);

WUFFS_BASE__MAYBE_ATTRIBUTE_TARGET("pclmul,popcnt,sse4.2,avx2")
static const uint8_t*  //
wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h2v2_triangle_x86_avx2(
    uint8_t* dst_ptr,
    const uint8_t* src_ptr_major,
    const uint8_t* src_ptr_minor,
    size_t src_len,
    uint32_t h1v2_bias_ignored,
    bool first_column,
    bool last_column);
#endif

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

// --------

typedef void (*wuffs_base__pixel_swizzler__swizzle_ycc__convert_func)(
    wuffs_base__pixel_buffer* dst,
    uint32_t x,
    uint32_t x_end,
    uint32_t y,
    const uint8_t* up0,
    const uint8_t* up1,
    const uint8_t* up2);

static void  //
wuffs_base__pixel_swizzler__swizzle_ycc__convert_general(
    wuffs_base__pixel_buffer* dst,
    uint32_t x,
    uint32_t x_end,
    uint32_t y,
    const uint8_t* up0,
    const uint8_t* up1,
    const uint8_t* up2) {
  for (; x < x_end; x++) {
    uint32_t color =                           //
        wuffs_base__color_ycc__as__color_u32(  //
            *up0++, *up1++, *up2++);
    wuffs_base__pixel_buffer__set_color_u32_at(dst, x, y, color);
  }
}

static void  //
wuffs_base__pixel_swizzler__swizzle_ycc__convert_bgrx(
    wuffs_base__pixel_buffer* dst,
    uint32_t x,
    uint32_t x_end,
    uint32_t y,
    const uint8_t* up0,
    const uint8_t* up1,
    const uint8_t* up2) {
  size_t dst_stride = dst->private_impl.planes[0].stride;
  uint8_t* dst_iter = dst->private_impl.planes[0].ptr +
                      (dst_stride * ((size_t)y)) + (4 * ((size_t)x));

  for (; x < x_end; x++) {
    uint32_t color =                           //
        wuffs_base__color_ycc__as__color_u32(  //
            *up0++, *up1++, *up2++);
    wuffs_base__poke_u32le__no_bounds_check(dst_iter, color);
    dst_iter += 4;
  }
}

static void  //
wuffs_base__pixel_swizzler__swizzle_ycc__convert_rgbx(
    wuffs_base__pixel_buffer* dst,
    uint32_t x,
    uint32_t x_end,
    uint32_t y,
    const uint8_t* up0,
    const uint8_t* up1,
    const uint8_t* up2) {
  size_t dst_stride = dst->private_impl.planes[0].stride;
  uint8_t* dst_iter = dst->private_impl.planes[0].ptr +
                      (dst_stride * ((size_t)y)) + (4 * ((size_t)x));

  for (; x < x_end; x++) {
    uint32_t color =                                //
        wuffs_base__color_ycc__as__color_u32_abgr(  //
            *up0++, *up1++, *up2++);
    wuffs_base__poke_u32le__no_bounds_check(dst_iter, color);
    dst_iter += 4;
  }
}

// --------

// wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func upsamples to a
// destination slice at least 480 (YCCK) or 672 (YCC) bytes long and whose
// src_len (multiplied by 1, 2, 3 or 4) is positive but no more than that. This
// 480 or 672 length is just under 1/4 or 1/3 of the scratch_buffer_2k slice
// length. Both (480 * 4) = 1920 and (672 * 3) = 2016 are less than 2048.
//
// 480 and 672 are nice round numbers because a JPEG MCU is 1, 2, 3 or 4 blocks
// wide and each block is 8 pixels wide. We have:
//   480 = 1 * 8 * 60,   672 = 1 * 8 * 84
//   480 = 2 * 8 * 30,   672 = 2 * 8 * 42
//   480 = 3 * 8 * 20,   672 = 3 * 8 * 28
//   480 = 4 * 8 * 15,   672 = 4 * 8 * 21
//
// Box filters are equivalent to nearest neighbor upsampling. These ignore the
// src_ptr_minor, h1v2_bias, first_column and last_column arguments.
//
// Triangle filters use a 3:1 ratio (in 1 dimension), or 9:3:3:1 (in 2
// dimensions), which is higher quality (less blocky) but also higher
// computational effort.
//
// In theory, we could use triangle filters for any (inv_h, inv_v) combination.
// In practice, matching libjpeg-turbo, we only implement it for the common
// chroma subsampling ratios (YCC420, YCC422 or YCC440), corresponding to an
// (inv_h, inv_v) pair of (2, 2), (2, 1) or (1, 2).
typedef const uint8_t* (
    *wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func)(
    uint8_t* dst_ptr,
    const uint8_t* src_ptr_major,  // Nearest row.
    const uint8_t* src_ptr_minor,  // Adjacent row, alternating above or below.
    size_t src_len,
    uint32_t h1v2_bias,
    bool first_column,
    bool last_column);

static const uint8_t*  //
wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h1vn_box(
    uint8_t* dst_ptr,
    const uint8_t* src_ptr_major,
    const uint8_t* src_ptr_minor_ignored,
    size_t src_len,
    uint32_t h1v2_bias_ignored,
    bool first_column_ignored,
    bool last_column_ignored) {
  return src_ptr_major;
}

static const uint8_t*  //
wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h2vn_box(
    uint8_t* dst_ptr,
    const uint8_t* src_ptr_major,
    const uint8_t* src_ptr_minor_ignored,
    size_t src_len,
    uint32_t h1v2_bias_ignored,
    bool first_column_ignored,
    bool last_column_ignored) {
  uint8_t* dp = dst_ptr;
  const uint8_t* sp = src_ptr_major;
  while (src_len--) {
    uint8_t sv = *sp++;
    *dp++ = sv;
    *dp++ = sv;
  }
  return dst_ptr;
}

static const uint8_t*  //
wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h3vn_box(
    uint8_t* dst_ptr,
    const uint8_t* src_ptr_major,
    const uint8_t* src_ptr_minor_ignored,
    size_t src_len,
    uint32_t h1v2_bias_ignored,
    bool first_column_ignored,
    bool last_column_ignored) {
  uint8_t* dp = dst_ptr;
  const uint8_t* sp = src_ptr_major;
  while (src_len--) {
    uint8_t sv = *sp++;
    *dp++ = sv;
    *dp++ = sv;
    *dp++ = sv;
  }
  return dst_ptr;
}

static const uint8_t*  //
wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h4vn_box(
    uint8_t* dst_ptr,
    const uint8_t* src_ptr_major,
    const uint8_t* src_ptr_minor_ignored,
    size_t src_len,
    uint32_t h1v2_bias_ignored,
    bool first_column_ignored,
    bool last_column_ignored) {
  uint8_t* dp = dst_ptr;
  const uint8_t* sp = src_ptr_major;
  while (src_len--) {
    uint8_t sv = *sp++;
    *dp++ = sv;
    *dp++ = sv;
    *dp++ = sv;
    *dp++ = sv;
  }
  return dst_ptr;
}

static const uint8_t*  //
wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h1v2_triangle(
    uint8_t* dst_ptr,
    const uint8_t* src_ptr_major,
    const uint8_t* src_ptr_minor,
    size_t src_len,
    uint32_t h1v2_bias,
    bool first_column,
    bool last_column) {
  uint8_t* dp = dst_ptr;
  const uint8_t* sp_major = src_ptr_major;
  const uint8_t* sp_minor = src_ptr_minor;
  while (src_len--) {
    *dp++ = (uint8_t)(((3u * ((uint32_t)(*sp_major++))) +  //
                       (1u * ((uint32_t)(*sp_minor++))) +  //
                       h1v2_bias) >>
                      2u);
  }
  return dst_ptr;
}

static const uint8_t*  //
wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h2v1_triangle(
    uint8_t* dst_ptr,
    const uint8_t* src_ptr_major,
    const uint8_t* src_ptr_minor,
    size_t src_len,
    uint32_t h1v2_bias_ignored,
    bool first_column,
    bool last_column) {
  uint8_t* dp = dst_ptr;
  const uint8_t* sp = src_ptr_major;

  if (first_column) {
    src_len--;
    if ((src_len <= 0u) && last_column) {
      uint8_t sv = *sp++;
      *dp++ = sv;
      *dp++ = sv;
      return dst_ptr;
    }
    uint32_t svp1 = sp[+1];
    uint8_t sv = *sp++;
    *dp++ = sv;
    *dp++ = (uint8_t)(((3u * (uint32_t)sv) + svp1 + 2u) >> 2u);
    if (src_len <= 0u) {
      return dst_ptr;
    }
  }

  if (last_column) {
    src_len--;
  }

  for (; src_len > 0u; src_len--) {
    uint32_t svm1 = sp[-1];
    uint32_t svp1 = sp[+1];
    uint32_t sv3 = 3u * (uint32_t)(*sp++);
    *dp++ = (uint8_t)((sv3 + svm1 + 1u) >> 2u);
    *dp++ = (uint8_t)((sv3 + svp1 + 2u) >> 2u);
  }

  if (last_column) {
    uint32_t svm1 = sp[-1];
    uint8_t sv = *sp++;
    *dp++ = (uint8_t)(((3u * (uint32_t)sv) + svm1 + 1u) >> 2u);
    *dp++ = sv;
  }

  return dst_ptr;
}

static const uint8_t*  //
wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h2v2_triangle(
    uint8_t* dst_ptr,
    const uint8_t* src_ptr_major,
    const uint8_t* src_ptr_minor,
    size_t src_len,
    uint32_t h1v2_bias_ignored,
    bool first_column,
    bool last_column) {
  uint8_t* dp = dst_ptr;
  const uint8_t* sp_major = src_ptr_major;
  const uint8_t* sp_minor = src_ptr_minor;

  if (first_column) {
    src_len--;
    if ((src_len <= 0u) && last_column) {
      uint32_t sv = (12u * ((uint32_t)(*sp_major++))) +  //
                    (4u * ((uint32_t)(*sp_minor++)));
      *dp++ = (uint8_t)((sv + 8u) >> 4u);
      *dp++ = (uint8_t)((sv + 7u) >> 4u);
      return dst_ptr;
    }

    uint32_t sv_major_m1 = sp_major[-0];  // Clamp offset to zero.
    uint32_t sv_minor_m1 = sp_minor[-0];  // Clamp offset to zero.
    uint32_t sv_major_p1 = sp_major[+1];
    uint32_t sv_minor_p1 = sp_minor[+1];

    uint32_t sv = (9u * ((uint32_t)(*sp_major++))) +  //
                  (3u * ((uint32_t)(*sp_minor++)));
    *dp++ = (uint8_t)((sv + (3u * sv_major_m1) + (sv_minor_m1) + 8u) >> 4u);
    *dp++ = (uint8_t)((sv + (3u * sv_major_p1) + (sv_minor_p1) + 7u) >> 4u);
    if (src_len <= 0u) {
      return dst_ptr;
    }
  }

  if (last_column) {
    src_len--;
  }

  for (; src_len > 0u; src_len--) {
    uint32_t sv_major_m1 = sp_major[-1];
    uint32_t sv_minor_m1 = sp_minor[-1];
    uint32_t sv_major_p1 = sp_major[+1];
    uint32_t sv_minor_p1 = sp_minor[+1];

    uint32_t sv = (9u * ((uint32_t)(*sp_major++))) +  //
                  (3u * ((uint32_t)(*sp_minor++)));
    *dp++ = (uint8_t)((sv + (3u * sv_major_m1) + (sv_minor_m1) + 8u) >> 4u);
    *dp++ = (uint8_t)((sv + (3u * sv_major_p1) + (sv_minor_p1) + 7u) >> 4u);
  }

  if (last_column) {
    uint32_t sv_major_m1 = sp_major[-1];
    uint32_t sv_minor_m1 = sp_minor[-1];
    uint32_t sv_major_p1 = sp_major[+0];  // Clamp offset to zero.
    uint32_t sv_minor_p1 = sp_minor[+0];  // Clamp offset to zero.

    uint32_t sv = (9u * ((uint32_t)(*sp_major++))) +  //
                  (3u * ((uint32_t)(*sp_minor++)));
    *dp++ = (uint8_t)((sv + (3u * sv_major_m1) + (sv_minor_m1) + 8u) >> 4u);
    *dp++ = (uint8_t)((sv + (3u * sv_major_p1) + (sv_minor_p1) + 7u) >> 4u);
  }

  return dst_ptr;
}

// wuffs_base__pixel_swizzler__swizzle_ycc__upsample_funcs is indexed by inv_h
// and then inv_v.
static const wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func
    wuffs_base__pixel_swizzler__swizzle_ycc__upsample_funcs[4][4] = {
        {
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h1vn_box,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h1v2_triangle,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h1vn_box,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h1vn_box,
        },
        {
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h2v1_triangle,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h2v2_triangle,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h2vn_box,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h2vn_box,
        },
        {
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h3vn_box,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h3vn_box,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h3vn_box,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h3vn_box,
        },
        {
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h4vn_box,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h4vn_box,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h4vn_box,
            wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h4vn_box,
        },
};

static inline uint32_t  //
wuffs_base__pixel_swizzler__has_triangle_upsampler(uint32_t inv_h,
                                                   uint32_t inv_v) {
  if (inv_h == 1u) {
    return inv_v == 2u;
  } else if (inv_h == 2u) {
    return (inv_v == 1u) || (inv_v == 2u);
  }
  return false;
}

// --------

// All of the wuffs_base__pixel_swizzler__swizzle_ycc__etc functions have
// preconditions. See all of the checks made in
// wuffs_base__pixel_swizzler__swizzle_ycck before calling these functions. For
// example, (width > 0) is a precondition, but there are many more.

static void  //
wuffs_base__pixel_swizzler__swizzle_ycc__general__triangle_filter_edge_row(
    wuffs_base__pixel_buffer* dst,
    uint32_t width,
    uint32_t y,
    const uint8_t* src_ptr0,
    const uint8_t* src_ptr1,
    const uint8_t* src_ptr2,
    uint32_t stride0,
    uint32_t stride1,
    uint32_t stride2,
    uint32_t inv_h0,
    uint32_t inv_h1,
    uint32_t inv_h2,
    uint32_t inv_v0,
    uint32_t inv_v1,
    uint32_t inv_v2,
    uint32_t half_width_for_2to1,
    uint32_t h1v2_bias,
    uint8_t* scratch_buffer_2k_ptr,
    wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func upfunc0,
    wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func upfunc1,
    wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func upfunc2,
    wuffs_base__pixel_swizzler__swizzle_ycc__convert_func convfunc) {
  const uint8_t* src0 = src_ptr0 + ((y / inv_v0) * (size_t)stride0);
  const uint8_t* src1 = src_ptr1 + ((y / inv_v1) * (size_t)stride1);
  const uint8_t* src2 = src_ptr2 + ((y / inv_v2) * (size_t)stride2);
  uint32_t total_src_len0 = 0u;
  uint32_t total_src_len1 = 0u;
  uint32_t total_src_len2 = 0u;

  uint32_t x = 0u;
  while (x < width) {
    bool first_column = x == 0u;
    uint32_t end = x + 672u;
    if (end > width) {
      end = width;
    }

    uint32_t src_len0 = ((end - x) + inv_h0 - 1u) / inv_h0;
    uint32_t src_len1 = ((end - x) + inv_h1 - 1u) / inv_h1;
    uint32_t src_len2 = ((end - x) + inv_h2 - 1u) / inv_h2;
    total_src_len0 += src_len0;
    total_src_len1 += src_len1;
    total_src_len2 += src_len2;

    const uint8_t* src_ptr_x0 = src0 + (x / inv_h0);
    const uint8_t* up0 = (*upfunc0)(          //
        scratch_buffer_2k_ptr + (0u * 672u),  //
        src_ptr_x0,                           //
        src_ptr_x0,                           //
        src_len0,                             //
        h1v2_bias,                            //
        first_column,                         //
        (total_src_len0 >= half_width_for_2to1));

    const uint8_t* src_ptr_x1 = src1 + (x / inv_h1);
    const uint8_t* up1 = (*upfunc1)(          //
        scratch_buffer_2k_ptr + (1u * 672u),  //
        src_ptr_x1,                           //
        src_ptr_x1,                           //
        src_len1,                             //
        h1v2_bias,                            //
        first_column,                         //
        (total_src_len1 >= half_width_for_2to1));

    const uint8_t* src_ptr_x2 = src2 + (x / inv_h2);
    const uint8_t* up2 = (*upfunc2)(          //
        scratch_buffer_2k_ptr + (2u * 672u),  //
        src_ptr_x2,                           //
        src_ptr_x2,                           //
        src_len2,                             //
        h1v2_bias,                            //
        first_column,                         //
        (total_src_len2 >= half_width_for_2to1));

    (*convfunc)(dst, x, end, y, up0, up1, up2);
    x = end;
  }
}

static void  //
wuffs_base__pixel_swizzler__swizzle_ycc__general__triangle_filter(
    wuffs_base__pixel_buffer* dst,
    uint32_t width,
    uint32_t height,
    const uint8_t* src_ptr0,
    const uint8_t* src_ptr1,
    const uint8_t* src_ptr2,
    uint32_t stride0,
    uint32_t stride1,
    uint32_t stride2,
    uint32_t inv_h0,
    uint32_t inv_h1,
    uint32_t inv_h2,
    uint32_t inv_v0,
    uint32_t inv_v1,
    uint32_t inv_v2,
    uint32_t half_width_for_2to1,
    uint32_t half_height_for_2to1,
    uint8_t* scratch_buffer_2k_ptr,
    const wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func (
        *upfuncs)[4][4],
    wuffs_base__pixel_swizzler__swizzle_ycc__convert_func convfunc) {
  wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func upfunc0 =
      (*upfuncs)[(inv_h0 - 1u) & 3u][(inv_v0 - 1u) & 3u];
  wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func upfunc1 =
      (*upfuncs)[(inv_h1 - 1u) & 3u][(inv_v1 - 1u) & 3u];
  wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func upfunc2 =
      (*upfuncs)[(inv_h2 - 1u) & 3u][(inv_v2 - 1u) & 3u];

  // First row.
  uint32_t h1v2_bias = 1u;
  wuffs_base__pixel_swizzler__swizzle_ycc__general__triangle_filter_edge_row(
      dst, width, 0u,                //
      src_ptr0, src_ptr1, src_ptr2,  //
      stride0, stride1, stride2,     //
      inv_h0, inv_h1, inv_h2,        //
      inv_v0, inv_v1, inv_v2,        //
      half_width_for_2to1,           //
      h1v2_bias,                     //
      scratch_buffer_2k_ptr,         //
      upfunc0, upfunc1, upfunc2, convfunc);
  h1v2_bias = 2u;

  // Middle rows.
  bool last_row = height == 2u * half_height_for_2to1;
  uint32_t y_max_excl = last_row ? (height - 1u) : height;
  uint32_t y;
  for (y = 1u; y < y_max_excl; y++) {
    const uint8_t* src0_major = src_ptr0 + ((y / inv_v0) * (size_t)stride0);
    const uint8_t* src0_minor =
        (inv_v0 != 2u)
            ? src0_major
            : ((y & 1u) ? (src0_major + stride0) : (src0_major - stride0));
    const uint8_t* src1_major = src_ptr1 + ((y / inv_v1) * (size_t)stride1);
    const uint8_t* src1_minor =
        (inv_v1 != 2u)
            ? src1_major
            : ((y & 1u) ? (src1_major + stride1) : (src1_major - stride1));
    const uint8_t* src2_major = src_ptr2 + ((y / inv_v2) * (size_t)stride2);
    const uint8_t* src2_minor =
        (inv_v2 != 2u)
            ? src2_major
            : ((y & 1u) ? (src2_major + stride2) : (src2_major - stride2));
    uint32_t total_src_len0 = 0u;
    uint32_t total_src_len1 = 0u;
    uint32_t total_src_len2 = 0u;

    uint32_t x = 0u;
    while (x < width) {
      bool first_column = x == 0u;
      uint32_t end = x + 672u;
      if (end > width) {
        end = width;
      }

      uint32_t src_len0 = ((end - x) + inv_h0 - 1u) / inv_h0;
      uint32_t src_len1 = ((end - x) + inv_h1 - 1u) / inv_h1;
      uint32_t src_len2 = ((end - x) + inv_h2 - 1u) / inv_h2;
      total_src_len0 += src_len0;
      total_src_len1 += src_len1;
      total_src_len2 += src_len2;

      const uint8_t* up0 = (*upfunc0)(          //
          scratch_buffer_2k_ptr + (0u * 672u),  //
          src0_major + (x / inv_h0),            //
          src0_minor + (x / inv_h0),            //
          src_len0,                             //
          h1v2_bias,                            //
          first_column,                         //
          (total_src_len0 >= half_width_for_2to1));

      const uint8_t* up1 = (*upfunc1)(          //
          scratch_buffer_2k_ptr + (1u * 672u),  //
          src1_major + (x / inv_h1),            //
          src1_minor + (x / inv_h1),            //
          src_len1,                             //
          h1v2_bias,                            //
          first_column,                         //
          (total_src_len1 >= half_width_for_2to1));

      const uint8_t* up2 = (*upfunc2)(          //
          scratch_buffer_2k_ptr + (2u * 672u),  //
          src2_major + (x / inv_h2),            //
          src2_minor + (x / inv_h2),            //
          src_len2,                             //
          h1v2_bias,                            //
          first_column,                         //
          (total_src_len2 >= half_width_for_2to1));

      (*convfunc)(dst, x, end, y, up0, up1, up2);
      x = end;
    }

    h1v2_bias ^= 3u;
  }

  // Last row.
  if (y_max_excl != height) {
    wuffs_base__pixel_swizzler__swizzle_ycc__general__triangle_filter_edge_row(
        dst, width, height - 1u,       //
        src_ptr0, src_ptr1, src_ptr2,  //
        stride0, stride1, stride2,     //
        inv_h0, inv_h1, inv_h2,        //
        inv_v0, inv_v1, inv_v2,        //
        half_width_for_2to1,           //
        h1v2_bias,                     //
        scratch_buffer_2k_ptr,         //
        upfunc0, upfunc1, upfunc2, convfunc);
  }
}

static void  //
wuffs_base__pixel_swizzler__swizzle_ycc__general__box_filter(
    wuffs_base__pixel_buffer* dst,
    uint32_t width,
    uint32_t height,
    const uint8_t* src_ptr0,
    const uint8_t* src_ptr1,
    const uint8_t* src_ptr2,
    uint32_t stride0,
    uint32_t stride1,
    uint32_t stride2,
    uint32_t inv_h0,
    uint32_t inv_h1,
    uint32_t inv_h2,
    uint32_t inv_v0,
    uint32_t inv_v1,
    uint32_t inv_v2,
    uint32_t half_width_for_2to1,
    uint32_t half_height_for_2to1,
    uint8_t* scratch_buffer_2k_ptr,
    const wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func (
        *upfuncs)[4][4],
    wuffs_base__pixel_swizzler__swizzle_ycc__convert_func convfunc) {
  // Convert an inv_h or inv_v value from {1, 2, 3, 4} to {12, 6, 4, 3}.
  uint32_t h0_out_of_12 = 12u / inv_h0;
  uint32_t h1_out_of_12 = 12u / inv_h1;
  uint32_t h2_out_of_12 = 12u / inv_h2;
  uint32_t v0_out_of_12 = 12u / inv_v0;
  uint32_t v1_out_of_12 = 12u / inv_v1;
  uint32_t v2_out_of_12 = 12u / inv_v2;

  uint32_t iy0 = 0u;
  uint32_t iy1 = 0u;
  uint32_t iy2 = 0u;
  uint32_t y = 0u;
  while (true) {
    const uint8_t* src_iter0 = src_ptr0;
    const uint8_t* src_iter1 = src_ptr1;
    const uint8_t* src_iter2 = src_ptr2;

    // TODO: call convfunc instead.
    // ¡ dst_iter = etc

    uint32_t ix0 = 0u;
    uint32_t ix1 = 0u;
    uint32_t ix2 = 0u;
    uint32_t x = 0u;
    while (true) {
      uint32_t color =                           //
          wuffs_base__color_ycc__as__color_u32(  //
              *src_iter0, *src_iter1, *src_iter2);
      wuffs_base__pixel_buffer__set_color_u32_at(dst, x, y, color);
      // ¡ dst_iter += 4

      if ((x + 1u) == width) {
        break;
      }
      x = x + 1u;
      ix0 += h0_out_of_12;
      if (ix0 >= 12u) {
        ix0 = 0u;
        src_iter0++;
      }
      ix1 += h1_out_of_12;
      if (ix1 >= 12u) {
        ix1 = 0u;
        src_iter1++;
      }
      ix2 += h2_out_of_12;
      if (ix2 >= 12u) {
        ix2 = 0u;
        src_iter2++;
      }
    }

    if ((y + 1u) == height) {
      break;
    }
    y = y + 1u;
    iy0 += v0_out_of_12;
    if (iy0 >= 12u) {
      iy0 = 0u;
      src_ptr0 += stride0;
    }
    iy1 += v1_out_of_12;
    if (iy1 >= 12u) {
      iy1 = 0u;
      src_ptr1 += stride1;
    }
    iy2 += v2_out_of_12;
    if (iy2 >= 12u) {
      iy2 = 0u;
      src_ptr2 += stride2;
    }
  }
}

// --------

// Specialized forms of:
//   - wuffs_base__pixel_swizzler__swizzle_ycc__general__triangle_filter
//   - wuffs_base__pixel_swizzler__swizzle_ycc__general__box_filter

static void  //
wuffs_base__pixel_swizzler__swizzle_ycc__bgrx__hv11(
    wuffs_base__pixel_buffer* dst,
    uint32_t width,
    uint32_t height,
    const uint8_t* src_ptr0,
    const uint8_t* src_ptr1,
    const uint8_t* src_ptr2,
    uint32_t stride0,
    uint32_t stride1,
    uint32_t stride2,
    uint32_t inv_h0,
    uint32_t inv_h1,
    uint32_t inv_h2,
    uint32_t inv_v0,
    uint32_t inv_v1,
    uint32_t inv_v2,
    uint32_t half_width_for_2to1,
    uint32_t half_height_for_2to1,
    uint8_t* scratch_buffer_2k_ptr,
    const wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func (
        *upfuncs)[4][4],
    wuffs_base__pixel_swizzler__swizzle_ycc__convert_func convfunc) {
  uint32_t y = 0u;
  for (; y < height; y++) {
    const uint8_t* src_iter0 = src_ptr0;
    const uint8_t* src_iter1 = src_ptr1;
    const uint8_t* src_iter2 = src_ptr2;

    // TODO: call convfunc instead.
    size_t dst_stride = dst->private_impl.planes[0].stride;
    uint8_t* dst_iter =
        dst->private_impl.planes[0].ptr + (dst_stride * ((size_t)y));

    uint32_t x = 0u;

    for (; (x + 4u) <= width; x += 4u) {
      wuffs_base__poke_u32le__no_bounds_check(   //
          dst_iter + 0x00,                       //
          wuffs_base__color_ycc__as__color_u32(  //
              src_iter0[0], src_iter1[0], src_iter2[0]));
      wuffs_base__poke_u32le__no_bounds_check(   //
          dst_iter + 0x04,                       //
          wuffs_base__color_ycc__as__color_u32(  //
              src_iter0[1], src_iter1[1], src_iter2[1]));
      wuffs_base__poke_u32le__no_bounds_check(   //
          dst_iter + 0x08,                       //
          wuffs_base__color_ycc__as__color_u32(  //
              src_iter0[2], src_iter1[2], src_iter2[2]));
      wuffs_base__poke_u32le__no_bounds_check(   //
          dst_iter + 0x0C,                       //
          wuffs_base__color_ycc__as__color_u32(  //
              src_iter0[3], src_iter1[3], src_iter2[3]));
      dst_iter += 16;

      src_iter0 += 4u;
      src_iter1 += 4u;
      src_iter2 += 4u;
    }

    for (; x < width; x++) {
      wuffs_base__poke_u32le__no_bounds_check(   //
          dst_iter + 0x00,                       //
          wuffs_base__color_ycc__as__color_u32(  //
              src_iter0[0], src_iter1[0], src_iter2[0]));
      dst_iter += 4;

      src_iter0++;
      src_iter1++;
      src_iter2++;
    }

    src_ptr0 += stride0;
    src_ptr1 += stride1;
    src_ptr2 += stride2;
  }
}

// ¡ INSERT patch_pixconv

// --------

// wuffs_base__pixel_swizzler__flattened_length is like
// wuffs_base__table__flattened_length but returns uint64_t (not size_t) and
// also accounts for subsampling.
static uint64_t  //
wuffs_base__pixel_swizzler__flattened_length(uint32_t width,
                                             uint32_t height,
                                             uint32_t stride,
                                             uint32_t inv_h,
                                             uint32_t inv_v) {
  uint64_t scaled_width = (((uint64_t)width) + (inv_h - 1u)) / inv_h;
  uint64_t scaled_height = (((uint64_t)height) + (inv_v - 1u)) / inv_v;
  if (scaled_height <= 0u) {
    return 0u;
  }
  return ((scaled_height - 1u) * stride) + scaled_width;
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__status  //
wuffs_base__pixel_swizzler__swizzle_ycck(
    const wuffs_base__pixel_swizzler* p,
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
    bool triangle_filter_for_2to1,
    wuffs_base__slice_u8 scratch_buffer_2k) {
  if (!p) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  } else if ((h3 != 0u) || (v3 != 0u)) {
    // TODO: support the K in YCCK.
    return wuffs_base__make_status(
        wuffs_base__error__unsupported_pixel_swizzler_option);
  } else if (!dst || (width > 0xFFFFu) || (height > 0xFFFFu) ||  //
             (4u <= ((unsigned int)h0 - 1u)) ||                  //
             (4u <= ((unsigned int)h1 - 1u)) ||                  //
             (4u <= ((unsigned int)h2 - 1u)) ||                  //
             (4u <= ((unsigned int)v0 - 1u)) ||                  //
             (4u <= ((unsigned int)v1 - 1u)) ||                  //
             (4u <= ((unsigned int)v2 - 1u)) ||                  //
             (scratch_buffer_2k.len < 2048u)) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }

  uint32_t max_incl_h = wuffs_base__u32__max_of_4(h0, h1, h2, h3);
  uint32_t max_incl_v = wuffs_base__u32__max_of_4(v0, v1, v2, v3);

  // Calculate the inverse h and v ratios.
  //
  // It also canonicalizes (h=2 and max_incl_h=4) as equivalent to (h=1 and
  // max_incl_h=2). In both cases, the inv_h value is 2.
  uint32_t inv_h0 = max_incl_h / h0;
  uint32_t inv_h1 = max_incl_h / h1;
  uint32_t inv_h2 = max_incl_h / h2;
  uint32_t inv_v0 = max_incl_v / v0;
  uint32_t inv_v1 = max_incl_v / v1;
  uint32_t inv_v2 = max_incl_v / v2;

  uint32_t half_width_for_2to1 = (width + 1u) / 2u;
  uint32_t half_height_for_2to1 = (height + 1u) / 2u;

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

  if ((width <= 0u) || (height <= 0u)) {
    return wuffs_base__make_status(NULL);
  }

  wuffs_base__pixel_swizzler__swizzle_ycc__convert_func convfunc = NULL;

  switch (dst->pixcfg.private_impl.pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
#if defined(WUFFS_BASE__CPU_ARCH__X86_FAMILY)
      if (wuffs_base__cpu_arch__have_x86_sse42()) {
        convfunc =
            &wuffs_base__pixel_swizzler__swizzle_ycc__convert_bgrx_x86_avx2;
        break;
      }
#endif
      convfunc = &wuffs_base__pixel_swizzler__swizzle_ycc__convert_bgrx;
      break;
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
#if defined(WUFFS_BASE__CPU_ARCH__X86_FAMILY)
      if (wuffs_base__cpu_arch__have_x86_sse42()) {
        convfunc =
            &wuffs_base__pixel_swizzler__swizzle_ycc__convert_rgbx_x86_avx2;
        break;
      }
#endif
      convfunc = &wuffs_base__pixel_swizzler__swizzle_ycc__convert_rgbx;
      break;
    default:
      convfunc = &wuffs_base__pixel_swizzler__swizzle_ycc__convert_general;
      break;
  }

  void (*func)(wuffs_base__pixel_buffer * dst,  //
               uint32_t width,                  //
               uint32_t height,                 //
               const uint8_t* src_ptr0,         //
               const uint8_t* src_ptr1,         //
               const uint8_t* src_ptr2,         //
               uint32_t stride0,                //
               uint32_t stride1,                //
               uint32_t stride2,                //
               uint32_t inv_h0,                 //
               uint32_t inv_h1,                 //
               uint32_t inv_h2,                 //
               uint32_t inv_v0,                 //
               uint32_t inv_v1,                 //
               uint32_t inv_v2,                 //
               uint32_t half_width_for_2to1,    //
               uint32_t half_height_for_2to1,   //
               uint8_t* scratch_buffer_2k_ptr,  //
               const wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func(
                   *upfuncs)[4][4],
               wuffs_base__pixel_swizzler__swizzle_ycc__convert_func convfunc) =
      &wuffs_base__pixel_swizzler__swizzle_ycc__general__box_filter;

  wuffs_base__pixel_swizzler__swizzle_ycc__upsample_func upfuncs[4][4];
  memcpy(&upfuncs, &wuffs_base__pixel_swizzler__swizzle_ycc__upsample_funcs,
         sizeof upfuncs);

  if (triangle_filter_for_2to1 &&
      (wuffs_base__pixel_swizzler__has_triangle_upsampler(inv_h0, inv_v0) ||
       wuffs_base__pixel_swizzler__has_triangle_upsampler(inv_h1, inv_v1) ||
       wuffs_base__pixel_swizzler__has_triangle_upsampler(inv_h2, inv_v2))) {
    func = &wuffs_base__pixel_swizzler__swizzle_ycc__general__triangle_filter;
#if defined(WUFFS_BASE__CPU_ARCH__X86_FAMILY)
    if (wuffs_base__cpu_arch__have_x86_sse42()) {
      upfuncs[1][1] =
          wuffs_base__pixel_swizzler__swizzle_ycc__upsample_inv_h2v2_triangle_x86_avx2;
    }
#endif

  } else {
    switch (dst->pixcfg.private_impl.pixfmt.repr) {
      case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
      case WUFFS_BASE__PIXEL_FORMAT__BGRX:
        if ((max_incl_h | max_incl_v) == 1) {
          func = &wuffs_base__pixel_swizzler__swizzle_ycc__bgrx__hv11;
        } else {
          func = &wuffs_base__pixel_swizzler__swizzle_ycc__bgrx__box_filter;
        }
        break;
      case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
      case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
      case WUFFS_BASE__PIXEL_FORMAT__RGBX:
        if ((max_incl_h | max_incl_v) == 1) {
          func = &wuffs_base__pixel_swizzler__swizzle_ycc__rgbx__hv11;
        } else {
          func = &wuffs_base__pixel_swizzler__swizzle_ycc__rgbx__box_filter;
        }
        break;
    }
  }

  (*func)(dst, width, height,                         //
          src0.ptr, src1.ptr, src2.ptr,               //
          stride0, stride1, stride2,                  //
          inv_h0, inv_h1, inv_h2,                     //
          inv_v0, inv_v1, inv_v2,                     //
          half_width_for_2to1, half_height_for_2to1,  //
          scratch_buffer_2k.ptr, &upfuncs, convfunc);

  return wuffs_base__make_status(NULL);
}
