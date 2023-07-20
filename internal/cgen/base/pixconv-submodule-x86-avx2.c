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

// ‼ WUFFS MULTI-FILE SECTION +x86_avx2
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
    const uint8_t* up2) {
  if ((x + 32u) > x_end) {
    wuffs_base__pixel_swizzler__swizzle_ycc__convert_bgrx(  //
        dst, x, x_end, y, up0, up1, up2);
    return;
  }

  size_t dst_stride = dst->private_impl.planes[0].stride;
  uint8_t* dst_iter = dst->private_impl.planes[0].ptr +
                      (dst_stride * ((size_t)y)) + (4 * ((size_t)x));

  // u0001 = u16x16 [0x0001 .. 0x0001]
  // u00FF = u16x16 [0x00FF .. 0x00FF]
  // uFF80 = u16x16 [0xFF80 .. 0xFF80]
  // uFFFF = u16x16 [0xFFFF .. 0xFFFF]
  const __m256i u0001 = _mm256_set1_epi16(0x0001);
  const __m256i u00FF = _mm256_set1_epi16(0x00FF);
  const __m256i uFF80 = _mm256_set1_epi16(0xFF80);
  const __m256i uFFFF = _mm256_set1_epi16(0xFFFF);

  // p8000_p0000 = u16x16 [0x8000 0x0000 .. 0x8000 0x0000]
  const __m256i p8000_p0000 = _mm256_set_epi16(                        //
      0x0000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000,  //
      0x0000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000, 0x0000, 0x8000);

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
  // 16-bit SIMD lanes. The fractional parts range from -0.5 .. +0.5 (as
  // floating-point) which is from -0x8000 .. +0x8000 (as fixed-point).
  //
  //  -0x3A5E = -0x20000 + 0x1C5A2     The B:Cb factor.
  //  +0x66E9 = -0x10000 + 0x166E9     The R:Cr factor.
  //  -0x581A = +0x00000 - 0x0581A     The G:Cb factor.
  //  +0x492E = +0x10000 - 0x0B6D2     The G:Cr factor.
  const __m256i m3A5E = _mm256_set1_epi16(-0x3A5E);
  const __m256i p66E9 = _mm256_set1_epi16(+0x66E9);
  const __m256i m581A_p492E = _mm256_set_epi16(  //
      +0x492E, -0x581A, +0x492E, -0x581A,        //
      +0x492E, -0x581A, +0x492E, -0x581A,        //
      +0x492E, -0x581A, +0x492E, -0x581A,        //
      +0x492E, -0x581A, +0x492E, -0x581A);

  while (x < x_end) {
    // Load chroma values in even and odd columns (the high 8 bits of each
    // u16x16 element are zero) and then subtract 0x0080.
    //
    // cb_all = u8x32  [cb.00 cb.01 cb.02 cb.03 .. cb.1C cb.1D cb.1E cb.1F]
    // cb_eve = i16x16 [cb.00-0x80  cb.02-0x80  .. cb.1C-0x80  cb.1E-0x80 ]
    // cb_odd = i16x16 [cb.01-0x80  cb.03-0x80  .. cb.1D-0x80  cb.1F-0x80 ]
    //
    // Ditto for the cr_xxx Chroma-Red values.
    __m256i cb_all = _mm256_lddqu_si256((const __m256i*)(const void*)up1);
    __m256i cr_all = _mm256_lddqu_si256((const __m256i*)(const void*)up2);
    __m256i cb_eve = _mm256_add_epi16(uFF80, _mm256_and_si256(cb_all, u00FF));
    __m256i cr_eve = _mm256_add_epi16(uFF80, _mm256_and_si256(cr_all, u00FF));
    __m256i cb_odd = _mm256_add_epi16(uFF80, _mm256_srli_epi16(cb_all, 8));
    __m256i cr_odd = _mm256_add_epi16(uFF80, _mm256_srli_epi16(cr_all, 8));

    // ----

    // Calculate:
    //
    //  B-Y = (+1.77200 * Cb)                 as floating-point
    //  R-Y = (+1.40200 * Cr)                 as floating-point
    //
    //  B-Y = ((0x2_0000 - 0x3A5E) * Cb)      as fixed-point
    //  R-Y = ((0x1_0000 + 0x66E9) * Cr)      as fixed-point
    //
    //  B-Y = ((-0x3A5E * Cb) + ("2.0" * Cb))
    //  R-Y = ((+0x66E9 * Cr) + ("1.0" * Cr))

    // Multiply by m3A5E or p66E9, taking the high 16 bits. There's also a
    // doubling (add x to itself), adding-of-1 and halving (shift right by 1).
    // That makes multiply-and-take-high round to nearest (instead of down).
    __m256i tmp_by_eve = _mm256_srai_epi16(
        _mm256_add_epi16(
            _mm256_mulhi_epi16(_mm256_add_epi16(cb_eve, cb_eve), m3A5E), u0001),
        1);
    __m256i tmp_by_odd = _mm256_srai_epi16(
        _mm256_add_epi16(
            _mm256_mulhi_epi16(_mm256_add_epi16(cb_odd, cb_odd), m3A5E), u0001),
        1);
    __m256i tmp_ry_eve = _mm256_srai_epi16(
        _mm256_add_epi16(
            _mm256_mulhi_epi16(_mm256_add_epi16(cr_eve, cr_eve), p66E9), u0001),
        1);
    __m256i tmp_ry_odd = _mm256_srai_epi16(
        _mm256_add_epi16(
            _mm256_mulhi_epi16(_mm256_add_epi16(cr_odd, cr_odd), p66E9), u0001),
        1);

    // Add (2 * Cb) and (1 * Cr).
    __m256i by_eve =
        _mm256_add_epi16(tmp_by_eve, _mm256_add_epi16(cb_eve, cb_eve));
    __m256i by_odd =
        _mm256_add_epi16(tmp_by_odd, _mm256_add_epi16(cb_odd, cb_odd));
    __m256i ry_eve = _mm256_add_epi16(tmp_ry_eve, cr_eve);
    __m256i ry_odd = _mm256_add_epi16(tmp_ry_odd, cr_odd);

    // ----

    // Calculate:
    //
    //  G-Y = (-0.34414 * Cb) +
    //        (-0.71414 * Cr)                 as floating-point
    //
    //  G-Y = ((+0x0_0000 - 0x581A) * Cb) +
    //        ((-0x1_0000 + 0x492E) * Cr)     as fixed-point
    //
    //  G-Y =  (-0x581A * Cb) +
    //         (+0x492E * Cr) - ("1.0" * Cr)

    // Multiply-add to get ((-0x581A * Cb) + (+0x492E * Cr)).
    __m256i tmp0_gy_eve_lo = _mm256_madd_epi16(  //
        _mm256_unpacklo_epi16(cb_eve, cr_eve), m581A_p492E);
    __m256i tmp0_gy_eve_hi = _mm256_madd_epi16(  //
        _mm256_unpackhi_epi16(cb_eve, cr_eve), m581A_p492E);
    __m256i tmp0_gy_odd_lo = _mm256_madd_epi16(  //
        _mm256_unpacklo_epi16(cb_odd, cr_odd), m581A_p492E);
    __m256i tmp0_gy_odd_hi = _mm256_madd_epi16(  //
        _mm256_unpackhi_epi16(cb_odd, cr_odd), m581A_p492E);

    // Divide the i32x8 vectors by (1 << 16), rounding to nearest.
    __m256i tmp1_gy_eve_lo =
        _mm256_srai_epi32(_mm256_add_epi32(tmp0_gy_eve_lo, p8000_p0000), 16);
    __m256i tmp1_gy_eve_hi =
        _mm256_srai_epi32(_mm256_add_epi32(tmp0_gy_eve_hi, p8000_p0000), 16);
    __m256i tmp1_gy_odd_lo =
        _mm256_srai_epi32(_mm256_add_epi32(tmp0_gy_odd_lo, p8000_p0000), 16);
    __m256i tmp1_gy_odd_hi =
        _mm256_srai_epi32(_mm256_add_epi32(tmp0_gy_odd_hi, p8000_p0000), 16);

    // Pack the ((-0x581A * Cb) + (+0x492E * Cr)) as i16x16 and subtract Cr.
    __m256i gy_eve = _mm256_sub_epi16(
        _mm256_packs_epi32(tmp1_gy_eve_lo, tmp1_gy_eve_hi), cr_eve);
    __m256i gy_odd = _mm256_sub_epi16(
        _mm256_packs_epi32(tmp1_gy_odd_lo, tmp1_gy_odd_hi), cr_odd);

    // ----

    // Add Y to (B-Y), (G-Y) and (R-Y) to produce B, G and R.
    //
    // For the resultant packed_x_xxx vectors, only elements 0 ..= 7 and 16 ..=
    // 23 of the 32-element vectors matter (since we'll unpacklo but not
    // unpackhi them). Let … denote 8 ignored consecutive u8 values and let %
    // denote 0xFF. We'll end this section with:
    //
    // packed_b_eve = u8x32 [b00 b02 .. b0C b0E  …  b10 b12 .. b1C b1E  …]
    // packed_b_odd = u8x32 [b01 b03 .. b0D b0F  …  b11 b13 .. b1D b1F  …]
    // packed_g_eve = u8x32 [g00 g02 .. g0C g0E  …  g10 g12 .. g1C g1E  …]
    // packed_g_odd = u8x32 [g01 g03 .. g0D g0F  …  g11 g13 .. g1D g1F  …]
    // packed_r_eve = u8x32 [r00 r02 .. r0C r0E  …  r10 r12 .. r1C r1E  …]
    // packed_r_odd = u8x32 [r01 r03 .. r0D r0F  …  r11 r13 .. r1D r1F  …]
    // uFFFF        = u8x32 [  %   % ..   %   %  …    %   % ..   %   %  …]

    __m256i yy_all = _mm256_lddqu_si256((const __m256i*)(const void*)up0);
    __m256i yy_eve = _mm256_and_si256(yy_all, u00FF);
    __m256i yy_odd = _mm256_srli_epi16(yy_all, 8);

    __m256i loose_b_eve = _mm256_add_epi16(by_eve, yy_eve);
    __m256i loose_b_odd = _mm256_add_epi16(by_odd, yy_odd);
    __m256i packed_b_eve = _mm256_packus_epi16(loose_b_eve, loose_b_eve);
    __m256i packed_b_odd = _mm256_packus_epi16(loose_b_odd, loose_b_odd);

    __m256i loose_g_eve = _mm256_add_epi16(gy_eve, yy_eve);
    __m256i loose_g_odd = _mm256_add_epi16(gy_odd, yy_odd);
    __m256i packed_g_eve = _mm256_packus_epi16(loose_g_eve, loose_g_eve);
    __m256i packed_g_odd = _mm256_packus_epi16(loose_g_odd, loose_g_odd);

    __m256i loose_r_eve = _mm256_add_epi16(ry_eve, yy_eve);
    __m256i loose_r_odd = _mm256_add_epi16(ry_odd, yy_odd);
    __m256i packed_r_eve = _mm256_packus_epi16(loose_r_eve, loose_r_eve);
    __m256i packed_r_odd = _mm256_packus_epi16(loose_r_odd, loose_r_odd);

    // ----

    // Mix those values (unpacking in 8, 16 and then 32 bit units) to get the
    // desired BGRX/RGBX order.
    //
    // From here onwards, all of our __m256i registers are u8x32.

    // mix00 = [b00 g00 b02 g02 .. b0E g0E b10 g10 .. b1C g1C b1E g1E]
    // mix01 = [b01 g01 b03 g03 .. b0F g0F b11 g11 .. b1D g1D b1F g1F]
    // mix02 = [r00   % r02   % .. r0E   % r10   % .. r1C   % r1E   %]
    // mix03 = [r01   % r03   % .. r0F   % r11   % .. r1D   % r1F   %]
    __m256i mix00 = _mm256_unpacklo_epi8(packed_b_eve, packed_g_eve);
    __m256i mix01 = _mm256_unpacklo_epi8(packed_b_odd, packed_g_odd);
    __m256i mix02 = _mm256_unpacklo_epi8(packed_r_eve, uFFFF);
    __m256i mix03 = _mm256_unpacklo_epi8(packed_r_odd, uFFFF);

    // mix10 = [b00 g00 r00 %  b02 g02 r02 %  b04 g04 r04 %  b06 g06 r06 %
    //          b10 g10 r10 %  b12 g12 r12 %  b14 g14 r14 %  b16 g16 r16 %]
    // mix11 = [b01 g01 r01 %  b03 g03 r03 %  b05 g05 r05 %  b07 g07 r07 %
    //          b11 g11 r11 %  b13 g13 r13 %  b15 g15 r15 %  b17 g17 r17 %]
    // mix12 = [b08 g08 r08 %  b0A g0A r0A %  b0C g0C r0C %  b0E g0E r0E %
    //          b18 g18 r18 %  b1A g1A r1A %  b1C g1C r1C %  b1E g1E r1E %]
    // mix13 = [b09 g09 r09 %  b0B g0B r0B %  b0D g0D r0D %  b0F g0F r0F %
    //          b19 g19 r19 %  b1B g1B r1B %  b1D g1D r1D %  b1F g1F r1F %]
    __m256i mix10 = _mm256_unpacklo_epi16(mix00, mix02);
    __m256i mix11 = _mm256_unpacklo_epi16(mix01, mix03);
    __m256i mix12 = _mm256_unpackhi_epi16(mix00, mix02);
    __m256i mix13 = _mm256_unpackhi_epi16(mix01, mix03);

    // mix20 = [b00 g00 r00 %  b01 g01 r01 %  b02 g02 r02 %  b03 g03 r03 %
    //          b10 g10 r10 %  b11 g11 r11 %  b12 g12 r12 %  b13 g13 r13 %]
    // mix21 = [b04 g04 r04 %  b05 g05 r05 %  b06 g06 r06 %  b07 g07 r07 %
    //          b14 g14 r14 %  b15 g15 r15 %  b16 g16 r16 %  b17 g17 r17 %]
    // mix22 = [b08 g08 r08 %  b09 g09 r09 %  b0A g0A r0A %  b0B g0B r0B %
    //          b18 g18 r18 %  b19 g19 r19 %  b1A g1A r1A %  b1B g1B r1B %]
    // mix23 = [b0C g0C r0C %  b0D g0D r0D %  b0E g0E r0E %  b0F g0F r0F %
    //          b1C g1C r1C %  b1D g1D r1D %  b1E g1E r1E %  b1F g1F r1F %]
    __m256i mix20 = _mm256_unpacklo_epi32(mix10, mix11);
    __m256i mix21 = _mm256_unpackhi_epi32(mix10, mix11);
    __m256i mix22 = _mm256_unpacklo_epi32(mix12, mix13);
    __m256i mix23 = _mm256_unpackhi_epi32(mix12, mix13);

    // mix30 = [b00 g00 r00 %  b01 g01 r01 %  b02 g02 r02 %  b03 g03 r03 %
    //          b04 g04 r04 %  b05 g05 r05 %  b06 g06 r06 %  b07 g07 r07 %]
    // mix31 = [b08 g08 r08 %  b09 g09 r09 %  b0A g0A r0A %  b0B g0B r0B %
    //          b0C g0C r0C %  b0D g0D r0D %  b0E g0E r0E %  b0F g0F r0F %]
    // mix32 = [b10 g10 r10 %  b11 g11 r11 %  b12 g12 r12 %  b13 g13 r13 %
    //          b14 g14 r14 %  b15 g15 r15 %  b16 g16 r16 %  b17 g17 r17 %]
    // mix33 = [b18 g18 r18 %  b19 g19 r19 %  b1A g1A r1A %  b1B g1B r1B %
    //          b1C g1C r1C %  b1D g1D r1D %  b1E g1E r1E %  b1F g1F r1F %]
    __m256i mix30 = _mm256_permute2x128_si256(mix20, mix21, 0x20);
    __m256i mix31 = _mm256_permute2x128_si256(mix22, mix23, 0x20);
    __m256i mix32 = _mm256_permute2x128_si256(mix20, mix21, 0x31);
    __m256i mix33 = _mm256_permute2x128_si256(mix22, mix23, 0x31);

    // Write out four u8x32 SIMD registers (128 bytes, 32 BGRX/RGBX pixels).
    _mm256_storeu_si256((__m256i*)(void*)(dst_iter + 0x00), mix30);
    _mm256_storeu_si256((__m256i*)(void*)(dst_iter + 0x20), mix31);
    _mm256_storeu_si256((__m256i*)(void*)(dst_iter + 0x40), mix32);
    _mm256_storeu_si256((__m256i*)(void*)(dst_iter + 0x60), mix33);

    // Advance by up to 32 pixels. The first iteration might be smaller than 32
    // so that all of the remaining steps are exactly 32.
    uint32_t n = 32 - (31 & (x - x_end));
    dst_iter += 4 * n;
    up0 += n;
    up1 += n;
    up2 += n;
    x += n;
  }
}
#endif  // defined(WUFFS_BASE__CPU_ARCH__X86_FAMILY)
// ‼ WUFFS MULTI-FILE SECTION -x86_avx2
