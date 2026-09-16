// Copyright 2026 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

// ----------------

// This is a small, self-contained, single-file C library to decode basic
// Handsum image files. Basic means that it only supports Handsum's default
// configuration, c3q4: color=3 (RGB, no Alpha) and quality=4 (best).
//
// It is a C port of some (decoding only and c3q4 only) of this Go library:
// https://pkg.go.dev/github.com/google/wuffs/lib/handsum
//
// To decode other configurations, like c1q1 (Gray, worst quality) or c4q2
// (RGBA, medium-low quality), use the github.com/google/wuffs Go or
// C-transpiled-from-memory-safe-Wuffs implementations.
//
// To use this file as a "foo.c"-like implementation, instead of a "foo.h"-like
// header, #define BASIC_HANDSUM_DECODE_IMPLEMENTATION before #include'ing or
// compiling it.
//
// As an option, you may also #define
// BASIC_HANDSUM_DECODE_CONFIG__STATIC_FUNCTIONS to make these functions have
// static storage. This can help the compiler ignore or discard unused code,
// which can produce faster compiles and smaller binaries.

#ifndef BASIC_HANDSUM_DECODE_INCLUDE_GUARD
#define BASIC_HANDSUM_DECODE_INCLUDE_GUARD

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#if defined(BASIC_HANDSUM_DECODE_CONFIG__STATIC_FUNCTIONS)
#define BASIC_HANDSUM_DECODE__MAYBE_STATIC static
#else
#define BASIC_HANDSUM_DECODE__MAYBE_STATIC
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct basic_handsum_decode__pixel_buffer_type {
  uint32_t width;   // Measured in pixels, not bytes.
  uint32_t height;  // Measured in pixels, not bytes.
  uint8_t bgra_pixels[1024];
} basic_handsum_decode__pixel_buffer;

// BASIC_HANDSUM_DECODE__RESULT__ETC can be returned by
// basic_handsum_decode__decode. Zero means OK. Non-zero means an error.
#define BASIC_HANDSUM_DECODE__RESULT__OK 0
#define BASIC_HANDSUM_DECODE__RESULT__DST_IS_NULL 1
#define BASIC_HANDSUM_DECODE__RESULT__SRC_IS_TOO_SHORT 2
#define BASIC_HANDSUM_DECODE__RESULT__SRC_IS_NOT_HANDSUM_C3Q4 3

// Decodes the source bytes into the destination pixel buffer, producing 4
// bytes per pixel (in B, G, R, A order). The bgra_pixels' row stride will be
// (4 * width) bytes.
//
// It only supports c3q4 Handsum files: color=3 (so alpha is always 0xFF) and
// quality=4 (best). Every valid c3q4 Handsum file is exactly 147 bytes long.
//
// On success, it returns zero and dst's width and height will range between 1
// and 16, inclusive.
//
// On failure, it returns non-zero and, if dst is non-NULL, its width and
// height will be set to zero.
//
// This function is thread-safe.
BASIC_HANDSUM_DECODE__MAYBE_STATIC int  //
basic_handsum_decode__decode(basic_handsum_decode__pixel_buffer* dst,
                             const uint8_t* src_ptr,
                             const size_t src_len);

// --------

#ifdef BASIC_HANDSUM_DECODE_IMPLEMENTATION

#ifdef __wasm__
static void  //
basic_handsum_decode__memcpy(uint8_t* restrict dst,
                             const uint8_t* restrict src,
                             size_t n) {
  while (n--) {
    *dst++ = *src++;
  }
}
#else
#include <string.h>
#define basic_handsum_decode__memcpy memcpy
#endif

static void  //
basic_handsum_decode__smooth_block_seams_16x16(uint8_t* b) {
  static const uint8_t smoothing_pairs_16x16[28][2] = {
      {0x07, 0x08}, {0x17, 0x18}, {0x27, 0x28}, {0x37, 0x38},
      {0x47, 0x48}, {0x57, 0x58}, {0x67, 0x68},

      {0x70, 0x80}, {0x71, 0x81}, {0x72, 0x82}, {0x73, 0x83},
      {0x74, 0x84}, {0x75, 0x85}, {0x76, 0x86},

      {0x79, 0x89}, {0x7A, 0x8A}, {0x7B, 0x8B}, {0x7C, 0x8C},
      {0x7D, 0x8D}, {0x7E, 0x8E}, {0x7F, 0x8F},

      {0x97, 0x98}, {0xA7, 0xA8}, {0xB7, 0xB8}, {0xC7, 0xC8},
      {0xD7, 0xD8}, {0xE7, 0xE8}, {0xF7, 0xF8},
  };

  for (int i = 0; i < 28; i++) {
    uint8_t p0 = smoothing_pairs_16x16[i][0];
    uint8_t p1 = smoothing_pairs_16x16[i][1];
    uint32_t v0 = b[p0];
    uint32_t v1 = b[p1];
    b[p0] = ((3 * v0) + v1 + 2) / 4;
    b[p1] = ((3 * v1) + v0 + 2) / 4;
  }

  uint32_t v77 = b[0x77];
  uint32_t v78 = b[0x78];
  uint32_t v88 = b[0x88];
  uint32_t v87 = b[0x87];

  b[0x77] = ((9 * v77) + (3 * v78) + v88 + (3 * v87) + 8) / 16;
  b[0x78] = ((9 * v78) + (3 * v88) + v87 + (3 * v77) + 8) / 16;
  b[0x88] = ((9 * v88) + (3 * v87) + v77 + (3 * v78) + 8) / 16;
  b[0x87] = ((9 * v87) + (3 * v77) + v78 + (3 * v88) + 8) / 16;
}

static void  //
basic_handsum_decode__smooth_block_seams_8x8_q4(uint8_t* b) {
  for (int y = 1; y < 7; y += 2) {
    for (int x = 1; x < 7; x += 2) {
      int o = (y * 8) + x;

      uint32_t v0 = b[o + 0];
      uint32_t v1 = b[o + 1];
      uint32_t v9 = b[o + 9];
      uint32_t v8 = b[o + 8];

      b[o + 0] = ((9 * v0) + (3 * v1) + v9 + (3 * v8) + 8) / 16;
      b[o + 1] = ((9 * v1) + (3 * v9) + v8 + (3 * v0) + 8) / 16;
      b[o + 9] = ((9 * v9) + (3 * v8) + v0 + (3 * v1) + 8) / 16;
      b[o + 8] = ((9 * v8) + (3 * v0) + v1 + (3 * v9) + 8) / 16;
    }

    {
      uint32_t v0 = b[000 + y];
      uint32_t v1 = b[001 + y];
      b[000 + y] = ((3 * v0) + v1 + 2) / 4;
      b[001 + y] = ((3 * v1) + v0 + 2) / 4;
    }

    {
      uint32_t v0 = b[070 + y];
      uint32_t v1 = b[071 + y];
      b[070 + y] = ((3 * v0) + v1 + 2) / 4;
      b[071 + y] = ((3 * v1) + v0 + 2) / 4;
    }

    int y8 = y * 8;

    {
      uint32_t v0 = b[000 + y8];
      uint32_t v1 = b[010 + y8];
      b[000 + y8] = ((3 * v0) + v1 + 2) / 4;
      b[010 + y8] = ((3 * v1) + v0 + 2) / 4;
    }

    {
      uint32_t v0 = b[007 + y8];
      uint32_t v1 = b[017 + y8];
      b[007 + y8] = ((3 * v0) + v1 + 2) / 4;
      b[017 + y8] = ((3 * v1) + v0 + 2) / 4;
    }
  }
}

// Return (x + 0x80), if in range, otherwise clamped to 0x00 or 0xFF.
static uint8_t  //
basic_handsum_decode__bias_and_clamp(int x) {
  x += 0x80;
  return ((x >> 8) == 0) ? x : ~(x >> 31);
}

static int  //
basic_handsum_decode__decode_block_q4(uint8_t* dst_ptr,
                                      size_t dst_stride,
                                      const uint8_t* src_ptr,
                                      size_t bit_offset) {
  uint8_t tmp[64];
  for (int i = 0; i < 16; i++) {
    int nibble0 = (src_ptr[bit_offset >> 3] >> (bit_offset & 4)) & 15;
    int dct0 = (nibble0 * 0x22) - 256;
    bit_offset += 4;

    int nibble1 = (src_ptr[bit_offset >> 3] >> (bit_offset & 4)) & 15;
    int dct1 = (nibble1 * 0x10) - 128;
    bit_offset += 4;

    int nibble2 = (src_ptr[bit_offset >> 3] >> (bit_offset & 4)) & 15;
    int dct2 = (nibble2 * 0x10) - 128;
    bit_offset += 4;

    int x2 = 2 * (i & 3);
    int y2 = 2 * (i >> 2);
    int o2 = (y2 * 8) + x2;

    tmp[o2 + 0] =
        basic_handsum_decode__bias_and_clamp((+dct0 + dct1 + dct2 + 1) >> 1);
    tmp[o2 + 1] =
        basic_handsum_decode__bias_and_clamp((+dct0 - dct1 + dct2 + 1) >> 1);
    tmp[o2 + 8] =
        basic_handsum_decode__bias_and_clamp((+dct0 + dct1 - dct2 + 1) >> 1);
    tmp[o2 + 9] =
        basic_handsum_decode__bias_and_clamp((+dct0 - dct1 - dct2 + 1) >> 1);
  }

  basic_handsum_decode__smooth_block_seams_8x8_q4(&tmp[0]);

  for (size_t i = 0; i < 8; i++) {
    size_t di = i * dst_stride;
    size_t ti = i * 8;
    basic_handsum_decode__memcpy(&dst_ptr[di], &tmp[ti], 8);
  }

  return bit_offset;
}

static void  //
basic_handsum_decode__scale_and_bias_chroma_down(uint8_t* b) {
  for (int i = 0; i < 64; i++) {
    b[i] = (b[i] >> 1) + 0x3C;
  }
}

static void  //
basic_handsum_decode__upsample_from(uint8_t* restrict dst,
                                    const uint8_t* restrict src) {
  // dst is 16x16, src is 8x8.
  for (int y = 0; y < 16; y++) {
    // sy is: 0, +8, -8, +8, -8, ..., +8, -8, 0.
    int sy = ((y == 0) || (y == 15)) ? 0 : ((y & 1) * 16) - 8;

    for (int x = 0; x < 16; x++) {
      // sx is: 0, +1, -1, +1, -1, ..., +1, -1, 0.
      int sx = ((x == 0) || (x == 15)) ? 0 : ((x & 1) * 2) - 1;

      int s0 = ((y >> 1) * 8) + (x >> 1);
      int v00 = src[s0];
      int v01 = src[s0 + sx];
      int v10 = src[s0 + sy];
      int v11 = src[s0 + sx + sy];

      int d0 = (y * 16) + x;
      dst[d0] = ((9 * v00) + (3 * v01) + (3 * v10) + v11 + 8) / 16;
    }
  }
}

static void  //
basic_handsum_decode__scale_horizontal(
    basic_handsum_decode__pixel_buffer* pixbuf) {
  uint8_t dst[1024];
  uint32_t w = pixbuf->width;
  const uint8_t* src = pixbuf->bgra_pixels;

  for (int y = 0; y < 16; y++) {
    int dstx = 0;
    uint32_t acc0 = 0;
    uint32_t acc1 = 0;
    uint32_t acc2 = 0;
    uint32_t acc3 = 0;
    uint32_t remainder = 16;
    for (int srcx = 0; srcx < 16; srcx++) {
      int si = (64 * y) + (4 * srcx);
      uint32_t s0 = src[si + 0];
      uint32_t s1 = src[si + 1];
      uint32_t s2 = src[si + 2];
      uint32_t s3 = src[si + 3];

      if (remainder > w) {
        remainder -= w;
        acc0 += w * s0;
        acc1 += w * s1;
        acc2 += w * s2;
        acc3 += w * s3;

      } else {
        acc0 += remainder * s0;
        acc1 += remainder * s1;
        acc2 += remainder * s2;
        acc3 += remainder * s3;

        int di = (4 * w * y) + (4 * dstx);
        dst[di + 0] = (acc0 + 8) / 16;
        dst[di + 1] = (acc1 + 8) / 16;
        dst[di + 2] = (acc2 + 8) / 16;
        dst[di + 3] = (acc3 + 8) / 16;
        dstx++;

        uint32_t partial = w - remainder;

        acc0 = partial * s0;
        acc1 = partial * s1;
        acc2 = partial * s2;
        acc3 = partial * s3;

        remainder = 16 - partial;
      }
    }
  }

  for (int y = 0; y < 16; y++) {
    basic_handsum_decode__memcpy(&pixbuf->bgra_pixels[4 * w * y],
                                 &dst[4 * w * y], 4 * w);
  }
}

static void  //
basic_handsum_decode__scale_vertical(
    basic_handsum_decode__pixel_buffer* pixbuf) {
  uint8_t dst[1024];
  uint32_t h = pixbuf->height;
  const uint8_t* src = pixbuf->bgra_pixels;

  for (int x = 0; x < 16; x++) {
    int dsty = 0;
    uint32_t acc0 = 0;
    uint32_t acc1 = 0;
    uint32_t acc2 = 0;
    uint32_t acc3 = 0;
    uint32_t remainder = 16;
    for (int srcy = 0; srcy < 16; srcy++) {
      int si = (64 * srcy) + (4 * x);
      uint32_t s0 = src[si + 0];
      uint32_t s1 = src[si + 1];
      uint32_t s2 = src[si + 2];
      uint32_t s3 = src[si + 3];

      if (remainder > h) {
        remainder -= h;
        acc0 += h * s0;
        acc1 += h * s1;
        acc2 += h * s2;
        acc3 += h * s3;

      } else {
        acc0 += remainder * s0;
        acc1 += remainder * s1;
        acc2 += remainder * s2;
        acc3 += remainder * s3;

        int di = (64 * dsty) + (4 * x);
        dst[di + 0] = (acc0 + 8) / 16;
        dst[di + 1] = (acc1 + 8) / 16;
        dst[di + 2] = (acc2 + 8) / 16;
        dst[di + 3] = (acc3 + 8) / 16;
        dsty++;

        uint32_t partial = h - remainder;

        acc0 = partial * s0;
        acc1 = partial * s1;
        acc2 = partial * s2;
        acc3 = partial * s3;

        remainder = 16 - partial;
      }
    }
  }

  basic_handsum_decode__memcpy(&pixbuf->bgra_pixels[0], &dst[0], 64 * h);
}

static void  //
basic_handsum_decode__convert_ycbcr_bgra(
    basic_handsum_decode__pixel_buffer* dst,
    const uint8_t* restrict yy,
    const uint8_t* restrict cb,
    const uint8_t* restrict cr) {
  for (int i = 0; i < 256; i++) {
    int32_t yy1 = (int32_t)(yy[i]) * 0x10101;
    int32_t cb1 = (int32_t)(cb[i]) - 128;
    int32_t cr1 = (int32_t)(cr[i]) - 128;

    int32_t r = yy1 + (91881 * cr1);
    if (((uint32_t)(r)&0xFF000000u) == 0) {
      r >>= 16;
    } else {
      r = -1 ^ (r >> 31);
    }

    int32_t g = yy1 - (22554 * cb1) - (46802 * cr1);
    if (((uint32_t)(g)&0xFF000000u) == 0) {
      g >>= 16;
    } else {
      g = -1 ^ (g >> 31);
    }

    int32_t b = yy1 + (116130 * cb1);
    if (((uint32_t)(b)&0xFF000000u) == 0) {
      b >>= 16;
    } else {
      b = -1 ^ (b >> 31);
    }

    dst->bgra_pixels[(4 * i) + 0] = b;
    dst->bgra_pixels[(4 * i) + 1] = g;
    dst->bgra_pixels[(4 * i) + 2] = r;
    dst->bgra_pixels[(4 * i) + 3] = 0xFF;
  }
}

BASIC_HANDSUM_DECODE__MAYBE_STATIC int  //
basic_handsum_decode__decode(basic_handsum_decode__pixel_buffer* dst,
                             const uint8_t* src_ptr,
                             const size_t src_len) {
  if (dst == NULL) {
    return BASIC_HANDSUM_DECODE__RESULT__DST_IS_NULL;
  }
  dst->width = 0;
  dst->height = 0;

  if (src_len < 147) {
    return BASIC_HANDSUM_DECODE__RESULT__SRC_IS_TOO_SHORT;
  } else if ((src_ptr[0] != 0xFE) ||           //
             (src_ptr[1] != 0xD7) ||           //
             ((src_ptr[2] & 0xE0) != 0x60) ||  //
             ((src_ptr[2] & 0x1F) == 0x1F)) {
    return BASIC_HANDSUM_DECODE__RESULT__SRC_IS_NOT_HANDSUM_C3Q4;
  }

  if ((src_ptr[2] & 0x10) == 0x00) {  // Landscape.
    dst->width = 16;
    dst->height = 1 + (src_ptr[2] & 0x0F);
  } else {  // Portrait.
    dst->width = 1 + (src_ptr[2] & 0x0F);
    dst->height = 16;
  }

  uint8_t luma_quad_block[256];
  uint8_t tmp[64];
  uint8_t cb_quad_block[256];
  uint8_t cr_quad_block[256];
  size_t bit_offset = 24;

  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &luma_quad_block[0x00], 16, src_ptr, bit_offset);
  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &luma_quad_block[0x08], 16, src_ptr, bit_offset);
  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &luma_quad_block[0x80], 16, src_ptr, bit_offset);
  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &luma_quad_block[0x88], 16, src_ptr, bit_offset);

  basic_handsum_decode__smooth_block_seams_16x16(&luma_quad_block[0]);

  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &tmp[0], 8, src_ptr, bit_offset);

  basic_handsum_decode__scale_and_bias_chroma_down(&tmp[0]);
  basic_handsum_decode__upsample_from(&cb_quad_block[0], &tmp[0]);

  bit_offset = basic_handsum_decode__decode_block_q4(  //
      &tmp[0], 8, src_ptr, bit_offset);

  basic_handsum_decode__scale_and_bias_chroma_down(&tmp[0]);
  basic_handsum_decode__upsample_from(&cr_quad_block[0], &tmp[0]);

  basic_handsum_decode__convert_ycbcr_bgra(
      dst, &luma_quad_block[0], &cb_quad_block[0], &cr_quad_block[0]);

  if (dst->width < 16) {
    basic_handsum_decode__scale_horizontal(dst);
  } else if (dst->height < 16) {
    basic_handsum_decode__scale_vertical(dst);
  }

  return BASIC_HANDSUM_DECODE__RESULT__OK;
}

#endif  // BASIC_HANDSUM_DECODE_IMPLEMENTATION

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif  // BASIC_HANDSUM_DECODE_INCLUDE_GUARD
