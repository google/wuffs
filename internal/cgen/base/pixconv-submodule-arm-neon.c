// Copyright 2024 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

// --------

// ‼ WUFFS MULTI-FILE SECTION +arm_neon
#if defined(WUFFS_PRIVATE_IMPL__CPU_ARCH__ARM_NEON)

static void  //
wuffs_private_impl__swizzle_ycc__convert_3_bgrx_arm_neon(
    wuffs_base__pixel_buffer* dst,
    uint32_t x,
    uint32_t x_end,
    uint32_t y,
    const uint8_t* up0,
    const uint8_t* up1,
    const uint8_t* up2) {
  size_t dst_stride = dst->private_impl.planes[0].stride;
  uint8_t* dst_iter = dst->private_impl.planes[0].ptr +
                      (dst_stride * ((size_t)y)) + (4u * ((size_t)x));

  // Per wuffs_base__color_ycc__as__color_u32, the formulae:
  //
  //  R = Y                + 1.40200 * Cr
  //  G = Y - 0.34414 * Cb - 0.71414 * Cr
  //  B = Y + 1.77200 * Cb
  //
  // When scaled by 1<<16:
  //
  //  0.34414 becomes 0x0581A =  22554.
  //  0.71414 becomes 0x0B6D2 =  46802.
  //  1.40200 becomes 0x166E9 =  91881.
  //  1.77200 becomes 0x1C5A2 = 116130.
  //
  // Separate the integer and fractional parts, since we work with signed
  // 16-bit SIMD lanes (int16x4_t for vmull_n_s16).
  //
  //  -0x3A5E = -0x20000 + 0x1C5A2     The B:Cb factor.
  //  +0x66E9 = -0x10000 + 0x166E9     The R:Cr factor.
  //  -0x581A = +0x00000 - 0x0581A     The G:Cb factor.
  //  +0x492E = +0x10000 - 0x0B6D2     The G:Cr factor.
  //
  //  B-Y = frac_B * Cb / 65536 + 2 * Cb
  //  R-Y = frac_R * Cr / 65536 + 1 * Cr
  //  G-Y = (frac_Gcb * Cb + frac_Gcr * Cr) / 65536 - 1 * Cr

  const int16_t k_frac_b_cb = -0x3A5E;  // -14942
  const int16_t k_frac_r_cr = +0x66E9;  // +26345
  const int16_t k_frac_g_cb = -0x581A;  // -22554
  const int16_t k_frac_g_cr = +0x492E;  // +18734

  const int16x8_t bias = vdupq_n_s16(128);
  const uint8x8_t alpha = vdup_n_u8(0xFF);

  while ((x + 8u) <= x_end) {
    // Load 8 pixels of Y, Cb, Cr.
    uint8x8_t y_u8 = vld1_u8(up0);
    uint8x8_t cb_u8 = vld1_u8(up1);
    uint8x8_t cr_u8 = vld1_u8(up2);

    // Widen to int16 and center chroma around zero.
    int16x8_t yy = vreinterpretq_s16_u16(vmovl_u8(y_u8));
    int16x8_t cb = vsubq_s16(vreinterpretq_s16_u16(vmovl_u8(cb_u8)), bias);
    int16x8_t cr = vsubq_s16(vreinterpretq_s16_u16(vmovl_u8(cr_u8)), bias);

    // Split into lo/hi halves for 32-bit precision multiplies.
    int16x4_t cb_lo = vget_low_s16(cb);
    int16x4_t cb_hi = vget_high_s16(cb);
    int16x4_t cr_lo = vget_low_s16(cr);
    int16x4_t cr_hi = vget_high_s16(cr);

    // R-Y = round(frac_R * Cr / 65536) + Cr
    int16x8_t ry = vcombine_s16(
        vrshrn_n_s32(vmull_n_s16(cr_lo, k_frac_r_cr), 16),
        vrshrn_n_s32(vmull_n_s16(cr_hi, k_frac_r_cr), 16));
    ry = vaddq_s16(ry, cr);

    // B-Y = round(frac_B * Cb / 65536) + 2 * Cb
    int16x8_t by = vcombine_s16(
        vrshrn_n_s32(vmull_n_s16(cb_lo, k_frac_b_cb), 16),
        vrshrn_n_s32(vmull_n_s16(cb_hi, k_frac_b_cb), 16));
    by = vaddq_s16(by, vaddq_s16(cb, cb));

    // G-Y = round((frac_Gcb * Cb + frac_Gcr * Cr) / 65536) - Cr
    int32x4_t gy32_lo = vmull_n_s16(cb_lo, k_frac_g_cb);
    gy32_lo = vmlal_n_s16(gy32_lo, cr_lo, k_frac_g_cr);
    int32x4_t gy32_hi = vmull_n_s16(cb_hi, k_frac_g_cb);
    gy32_hi = vmlal_n_s16(gy32_hi, cr_hi, k_frac_g_cr);
    int16x8_t gy = vcombine_s16(
        vrshrn_n_s32(gy32_lo, 16),
        vrshrn_n_s32(gy32_hi, 16));
    gy = vsubq_s16(gy, cr);

    // Add Y and clamp to [0, 255] via saturating unsigned narrow.
    uint8x8_t r = vqmovun_s16(vaddq_s16(yy, ry));
    uint8x8_t g = vqmovun_s16(vaddq_s16(yy, gy));
    uint8x8_t b = vqmovun_s16(vaddq_s16(yy, by));

    // Interleave to BGRX and store 8 pixels (32 bytes).
    uint8x8x4_t bgrx;
    bgrx.val[0] = b;
    bgrx.val[1] = g;
    bgrx.val[2] = r;
    bgrx.val[3] = alpha;
    vst4_u8(dst_iter, bgrx);

    dst_iter += 32u;
    up0 += 8u;
    up1 += 8u;
    up2 += 8u;
    x += 8u;
  }

  // Scalar tail.
  for (; x < x_end; x++) {
    uint32_t color =                           //
        wuffs_base__color_ycc__as__color_u32(  //
            *up0++, *up1++, *up2++);
    wuffs_base__poke_u32le__no_bounds_check(dst_iter, color);
    dst_iter += 4u;
  }
}

// The rgbx flavor is exactly the same as the bgrx flavor except that the
// interleave order is {r, g, b, alpha} instead of {b, g, r, alpha}.
static void  //
wuffs_private_impl__swizzle_ycc__convert_3_rgbx_arm_neon(
    wuffs_base__pixel_buffer* dst,
    uint32_t x,
    uint32_t x_end,
    uint32_t y,
    const uint8_t* up0,
    const uint8_t* up1,
    const uint8_t* up2) {
  size_t dst_stride = dst->private_impl.planes[0].stride;
  uint8_t* dst_iter = dst->private_impl.planes[0].ptr +
                      (dst_stride * ((size_t)y)) + (4u * ((size_t)x));

  const int16_t k_frac_b_cb = -0x3A5E;
  const int16_t k_frac_r_cr = +0x66E9;
  const int16_t k_frac_g_cb = -0x581A;
  const int16_t k_frac_g_cr = +0x492E;

  const int16x8_t bias = vdupq_n_s16(128);
  const uint8x8_t alpha = vdup_n_u8(0xFF);

  while ((x + 8u) <= x_end) {
    uint8x8_t y_u8 = vld1_u8(up0);
    uint8x8_t cb_u8 = vld1_u8(up1);
    uint8x8_t cr_u8 = vld1_u8(up2);

    int16x8_t yy = vreinterpretq_s16_u16(vmovl_u8(y_u8));
    int16x8_t cb = vsubq_s16(vreinterpretq_s16_u16(vmovl_u8(cb_u8)), bias);
    int16x8_t cr = vsubq_s16(vreinterpretq_s16_u16(vmovl_u8(cr_u8)), bias);

    int16x4_t cb_lo = vget_low_s16(cb);
    int16x4_t cb_hi = vget_high_s16(cb);
    int16x4_t cr_lo = vget_low_s16(cr);
    int16x4_t cr_hi = vget_high_s16(cr);

    int16x8_t ry = vcombine_s16(
        vrshrn_n_s32(vmull_n_s16(cr_lo, k_frac_r_cr), 16),
        vrshrn_n_s32(vmull_n_s16(cr_hi, k_frac_r_cr), 16));
    ry = vaddq_s16(ry, cr);

    int16x8_t by = vcombine_s16(
        vrshrn_n_s32(vmull_n_s16(cb_lo, k_frac_b_cb), 16),
        vrshrn_n_s32(vmull_n_s16(cb_hi, k_frac_b_cb), 16));
    by = vaddq_s16(by, vaddq_s16(cb, cb));

    int32x4_t gy32_lo = vmull_n_s16(cb_lo, k_frac_g_cb);
    gy32_lo = vmlal_n_s16(gy32_lo, cr_lo, k_frac_g_cr);
    int32x4_t gy32_hi = vmull_n_s16(cb_hi, k_frac_g_cb);
    gy32_hi = vmlal_n_s16(gy32_hi, cr_hi, k_frac_g_cr);
    int16x8_t gy = vcombine_s16(
        vrshrn_n_s32(gy32_lo, 16),
        vrshrn_n_s32(gy32_hi, 16));
    gy = vsubq_s16(gy, cr);

    uint8x8_t r = vqmovun_s16(vaddq_s16(yy, ry));
    uint8x8_t g = vqmovun_s16(vaddq_s16(yy, gy));
    uint8x8_t b = vqmovun_s16(vaddq_s16(yy, by));

    // Interleave to RGBX and store 8 pixels (32 bytes).
    uint8x8x4_t rgbx;
    rgbx.val[0] = r;
    rgbx.val[1] = g;
    rgbx.val[2] = b;
    rgbx.val[3] = alpha;
    vst4_u8(dst_iter, rgbx);

    dst_iter += 32u;
    up0 += 8u;
    up1 += 8u;
    up2 += 8u;
    x += 8u;
  }

  for (; x < x_end; x++) {
    uint32_t color =                                //
        wuffs_base__color_ycc__as__color_u32_abgr(  //
            *up0++, *up1++, *up2++);
    wuffs_base__poke_u32le__no_bounds_check(dst_iter, color);
    dst_iter += 4u;
  }
}

#endif  // defined(WUFFS_PRIVATE_IMPL__CPU_ARCH__ARM_NEON)
// ‼ WUFFS MULTI-FILE SECTION -arm_neon
