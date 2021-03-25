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

// ----------------

/*
This test program is typically run indirectly, by the "wuffs test" or "wuffs
bench" commands. These commands take an optional "-mimic" flag to check that
Wuffs' output mimics (i.e. exactly matches) other libraries' output, such as
giflib for GIF, libpng for PNG, etc.

To manually run this test:

for CC in clang gcc; do
  $CC -std=c99 -Wall -Werror wbmp.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).

Add the "wuffs mimic cflags" (everything after the colon below) to the C
compiler flags (after the .c file) to run the mimic tests.

To manually run the benchmarks, replace "-Wall -Werror" with "-O3" and replace
the first "./a.out" with "./a.out -bench". Combine these changes with the
"wuffs mimic cflags" to run the mimic benchmarks.
*/

// ¿ wuffs mimic cflags: -DWUFFS_MIMIC

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c choose which parts of Wuffs to build. That file contains the
// entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__WBMP

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
// No mimic library.
#endif

// ---------------- Pixel Swizzler Tests

void  //
fill_palette_with_grays(wuffs_base__pixel_buffer* pb) {
  wuffs_base__slice_u8 palette = wuffs_base__pixel_buffer__palette(pb);
  if (palette.len != 1024) {
    return;
  }
  int i;
  for (i = 0; i < 256; i++) {
    palette.ptr[(4 * i) + 0] = i;
    palette.ptr[(4 * i) + 1] = i;
    palette.ptr[(4 * i) + 2] = i;
    palette.ptr[(4 * i) + 3] = 0xFF;
  }
}

void  //
fill_palette_with_nrgba_transparent_yellows(wuffs_base__pixel_buffer* pb) {
  wuffs_base__slice_u8 palette = wuffs_base__pixel_buffer__palette(pb);
  if (palette.len != 1024) {
    return;
  }
  int i;
  for (i = 0; i < 256; i++) {
    palette.ptr[(4 * i) + 0] = 0x00;
    palette.ptr[(4 * i) + 1] = 0x99;
    palette.ptr[(4 * i) + 2] = 0xCC;
    palette.ptr[(4 * i) + 3] = i;
  }
}

bool  //
colors_differ(wuffs_base__color_u32_argb_premul color0,
              wuffs_base__color_u32_argb_premul color1,
              uint32_t per_channel_tolerance) {
  uint32_t shift;
  for (shift = 0; shift < 32; shift += 8) {
    uint32_t channel0 = 0xFF & (color0 >> shift);
    uint32_t channel1 = 0xFF & (color1 >> shift);
    uint32_t delta =
        (channel0 > channel1) ? (channel0 - channel1) : (channel1 - channel0);
    if (delta > per_channel_tolerance) {
      return true;
    }
  }
  return false;
}

const char*  //
test_wuffs_pixel_buffer_fill_rect() {
  CHECK_FOCUS(__func__);

  const uint32_t width = 5;
  const uint32_t height = 5;

  const struct {
    wuffs_base__color_u32_argb_premul color;
    uint32_t pixfmt_repr;
  } dsts[] = {
      {
          .color = 0xFF000010,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGR_565,
      },
      {
          .color = 0xFF000040,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGR,
      },
      {
          .color = 0x88000048,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL,
      },
      {
          .color = 0x88000048,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL_4X16LE,
      },
      {
          .color = 0x88000048,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
      },
      {
          .color = 0xFF000040,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRX,
      },
      {
          .color = 0x88000048,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL,
      },
      {
          .color = 0x88000048,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL,
      },
  };

  int d;
  for (d = 0; d < WUFFS_TESTLIB_ARRAY_SIZE(dsts); d++) {
    // Allocate the dst_pixbuf.
    wuffs_base__pixel_config dst_pixcfg = ((wuffs_base__pixel_config){});
    wuffs_base__pixel_config__set(&dst_pixcfg, dsts[d].pixfmt_repr,
                                  WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width,
                                  height);
    wuffs_base__pixel_buffer dst_pixbuf = ((wuffs_base__pixel_buffer){});
    CHECK_STATUS("set_from_slice",
                 wuffs_base__pixel_buffer__set_from_slice(
                     &dst_pixbuf, &dst_pixcfg, g_have_slice_u8));

    int orientation;
    for (orientation = 0; orientation < 2; orientation++) {
      // Reset to transparent black (or its closest approximation).
      uint32_t y;
      for (y = 0; y < height; y++) {
        uint32_t x;
        for (x = 0; x < width; x++) {
          CHECK_STATUS(
              "set_color_u32_at",
              wuffs_base__pixel_buffer__set_color_u32_at(&dst_pixbuf, x, y, 0));
        }
      }

      // Fill a rectangle:
      //  - orientation == 0 means 1 pixel wide.
      //  - orientation == 1 means 1 pixel high.
      wuffs_base__rect_ie_u32 rect =
          orientation ? wuffs_base__make_rect_ie_u32(0, height / 2, width,
                                                     1 + (height / 2))
                      : wuffs_base__make_rect_ie_u32(width / 2, 0,
                                                     1 + (width / 2), height);
      CHECK_STATUS("set_color_u32_fill_rect",
                   wuffs_base__pixel_buffer__set_color_u32_fill_rect(
                       &dst_pixbuf, rect, dsts[d].color));

      // Check the middle dst pixel.
      wuffs_base__color_u32_argb_premul want_dst_pixel = dsts[d].color;
      wuffs_base__color_u32_argb_premul have_dst_pixel =
          wuffs_base__pixel_buffer__color_u32_at(&dst_pixbuf, width / 2,
                                                 height / 2);
      if (colors_differ(have_dst_pixel, want_dst_pixel, 0)) {
        RETURN_FAIL("d=%d, orientation=%d: dst_pixel: have 0x%08" PRIX32
                    ", want 0x%08" PRIX32,
                    d, orientation, have_dst_pixel, want_dst_pixel);
      }
    }
  }
  return NULL;
}

const char*  //
test_wuffs_pixel_swizzler_swizzle() {
  CHECK_FOCUS(__func__);

  const uint32_t width = 22;
  const uint32_t height = 5;
  uint8_t fallback_palette_array[1024];
  wuffs_base__pixel_swizzler swizzler;

  const struct {
    wuffs_base__color_u32_argb_premul color;
    uint32_t pixfmt_repr;
  } srcs[] = {
      // When updating this list, also consider updating the pixel formats that
      // fuzz/c/std/pixel_swizzler_fuzzer.c exercises.
      {
          .color = 0xFF444444,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__Y,
      },
      {
          .color = 0xFF444444,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__Y_16BE,
      },
      {
          .color = 0x55443300,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL,
      },
      {
          .color = 0xFF444444,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY,
      },
      {
          .color = 0xFF102031,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGR_565,
      },
      {
          .color = 0xFF443300,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGR,
      },
      {
          .color = 0x55443300,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL,
      },
      {
          .color = 0x55443300,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL_4X16LE,
      },
      {
          .color = 0x55443300,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
      },
      {
          .color = 0xFF443300,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRX,
      },
      {
          .color = 0xFF443300,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__RGB,
      },
      {
          .color = 0x55443300,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL,
      },
      {
          .color = 0x55443300,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL,
      },
  };

  const struct {
    wuffs_base__color_u32_argb_premul color;
    uint32_t pixfmt_repr;
  } dsts[] = {
      // When updating this list, also consider updating the pixel formats that
      // fuzz/c/std/pixel_swizzler_fuzzer.c exercises and those that
      // wuffs_aux::DecodeImageCallbacks::SelectPixfmt accepts.
      {
          .color = 0xFF000010,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGR_565,
      },
      {
          .color = 0xFF000040,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGR,
      },
      {
          .color = 0x80000040,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL,
      },
      {
          .color = 0x80123456,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL_4X16LE,
      },
      {
          .color = 0x80000040,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
      },
      {
          .color = 0x33002233,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL,
      },
      {
          .color = 0x33002233,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL,
      },
  };

  const wuffs_base__pixel_blend blends[] = {
      WUFFS_BASE__PIXEL_BLEND__SRC,
      WUFFS_BASE__PIXEL_BLEND__SRC_OVER,
  };

  int s;
  for (s = 0; s < WUFFS_TESTLIB_ARRAY_SIZE(srcs); s++) {
    // Allocate the src_pixbuf.
    wuffs_base__pixel_config src_pixcfg = ((wuffs_base__pixel_config){});
    wuffs_base__pixel_config__set(&src_pixcfg, srcs[s].pixfmt_repr,
                                  WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width,
                                  height);
    wuffs_base__pixel_buffer src_pixbuf = ((wuffs_base__pixel_buffer){});
    CHECK_STATUS("set_from_slice",
                 wuffs_base__pixel_buffer__set_from_slice(
                     &src_pixbuf, &src_pixcfg, g_src_slice_u8));
    if (srcs[s].pixfmt_repr ==
        WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL) {
      fill_palette_with_nrgba_transparent_yellows(&src_pixbuf);
    } else {
      fill_palette_with_grays(&src_pixbuf);
    }

    // Set and check the middle src pixel.
    CHECK_STATUS("set_color_u32_at",
                 wuffs_base__pixel_buffer__set_color_u32_at(
                     &src_pixbuf, width / 2, height / 2, srcs[s].color));
    wuffs_base__color_u32_argb_premul have_src_pixel =
        wuffs_base__pixel_buffer__color_u32_at(&src_pixbuf, width / 2,
                                               height / 2);
    if (have_src_pixel != srcs[s].color) {
      RETURN_FAIL("s=%d: src_pixel: have 0x%08" PRIX32 ", want 0x%08" PRIX32, s,
                  have_src_pixel, srcs[s].color);
    }

    int d;
    for (d = 0; d < WUFFS_TESTLIB_ARRAY_SIZE(dsts); d++) {
      // Allocate the dst_pixbuf.
      wuffs_base__pixel_config dst_pixcfg = ((wuffs_base__pixel_config){});
      wuffs_base__pixel_config__set(&dst_pixcfg, dsts[d].pixfmt_repr,
                                    WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width,
                                    height);
      wuffs_base__pixel_buffer dst_pixbuf = ((wuffs_base__pixel_buffer){});
      CHECK_STATUS("set_from_slice",
                   wuffs_base__pixel_buffer__set_from_slice(
                       &dst_pixbuf, &dst_pixcfg, g_have_slice_u8));
      wuffs_base__pixel_format dst_pixfmt =
          wuffs_base__make_pixel_format(dsts[d].pixfmt_repr);
      wuffs_base__pixel_alpha_transparency dst_transparency =
          wuffs_base__pixel_format__transparency(&dst_pixfmt);

      wuffs_base__slice_u8 dst_palette =
          wuffs_base__pixel_buffer__palette(&dst_pixbuf);
      if (dst_palette.len == 0) {
        dst_palette = wuffs_base__make_slice_u8(
            &fallback_palette_array[0],
            WUFFS_TESTLIB_ARRAY_SIZE(fallback_palette_array));
      }

      int b;
      for (b = 0; b < WUFFS_TESTLIB_ARRAY_SIZE(blends); b++) {
        // Set the middle dst pixel.
        CHECK_STATUS("set_color_u32_at",
                     wuffs_base__pixel_buffer__set_color_u32_at(
                         &dst_pixbuf, width / 2, height / 2, dsts[d].color));

        // Swizzle.
        CHECK_STATUS(
            "prepare",
            wuffs_base__pixel_swizzler__prepare(
                &swizzler, dst_pixfmt, dst_palette,
                wuffs_base__make_pixel_format(srcs[s].pixfmt_repr),
                wuffs_base__pixel_buffer__palette(&src_pixbuf), blends[b]));
        wuffs_base__pixel_swizzler__swizzle_interleaved_from_slice(
            &swizzler,
            wuffs_base__table_u8__row(
                wuffs_base__pixel_buffer__plane(&dst_pixbuf, 0), height / 2),
            dst_palette,
            wuffs_base__table_u8__row(
                wuffs_base__pixel_buffer__plane(&src_pixbuf, 0), height / 2));

        // Check the middle dst pixel.
        uint32_t tolerance =
            (dsts[d].pixfmt_repr == WUFFS_BASE__PIXEL_FORMAT__BGR_565) ? 4 : 0;
        wuffs_base__color_u32_argb_premul want_dst_pixel = 0;
        if (blends[b] == WUFFS_BASE__PIXEL_BLEND__SRC) {
          want_dst_pixel = srcs[s].color;
        } else if (blends[b] == WUFFS_BASE__PIXEL_BLEND__SRC_OVER) {
          tolerance += 1;
          want_dst_pixel = wuffs_base__composite_premul_premul_u32_axxx(
              dsts[d].color, srcs[s].color);
        } else {
          return "unsupported blend";
        }
        if (dst_transparency == WUFFS_BASE__PIXEL_ALPHA_TRANSPARENCY__OPAQUE) {
          want_dst_pixel |= 0xFF000000;
        }
        wuffs_base__color_u32_argb_premul have_dst_pixel =
            wuffs_base__pixel_buffer__color_u32_at(&dst_pixbuf, width / 2,
                                                   height / 2);
        if (colors_differ(have_dst_pixel, want_dst_pixel, tolerance)) {
          RETURN_FAIL("s=%d, d=%d, b=%d: dst_pixel: have 0x%08" PRIX32
                      ", want 0x%08" PRIX32 ", per-channel tolerance=%" PRId32,
                      s, d, b, have_dst_pixel, want_dst_pixel, tolerance);
        }
      }
    }
  }
  return NULL;
}

// ---------------- WBMP Tests

const char*  //
test_wuffs_wbmp_decode_interface() {
  CHECK_FOCUS(__func__);
  wuffs_wbmp__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_wbmp__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
  return do_test__wuffs_base__image_decoder(
      wuffs_wbmp__decoder__upcast_as__wuffs_base__image_decoder(&dec),
      "test/data/muybridge-frame-000.wbmp", 0, SIZE_MAX, 30, 20, 0xFFFFFFFF);
}

const char*  //
test_wuffs_wbmp_decode_frame_config() {
  CHECK_FOCUS(__func__);
  wuffs_wbmp__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_wbmp__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  wuffs_base__frame_config fc = ((wuffs_base__frame_config){});
  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = g_src_slice_u8,
  });
  CHECK_STRING(read_file(&src, "test/data/hat.wbmp"));
  CHECK_STATUS("decode_frame_config #0",
               wuffs_wbmp__decoder__decode_frame_config(&dec, &fc, &src));

  wuffs_base__status status =
      wuffs_wbmp__decoder__decode_frame_config(&dec, &fc, &src);
  if (status.repr != wuffs_base__note__end_of_data) {
    RETURN_FAIL("decode_frame_config #1: have \"%s\", want \"%s\"", status.repr,
                wuffs_base__note__end_of_data);
  }
  return NULL;
}

const char*  //
test_wuffs_wbmp_decode_image_config() {
  CHECK_FOCUS(__func__);
  wuffs_wbmp__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_wbmp__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  wuffs_base__image_config ic = ((wuffs_base__image_config){});
  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = g_src_slice_u8,
  });
  CHECK_STRING(read_file(&src, "test/data/bricks-nodither.wbmp"));
  CHECK_STATUS("decode_image_config",
               wuffs_wbmp__decoder__decode_image_config(&dec, &ic, &src));

  uint32_t have_width = wuffs_base__pixel_config__width(&ic.pixcfg);
  uint32_t want_width = 160;
  if (have_width != want_width) {
    RETURN_FAIL("width: have %" PRIu32 ", want %" PRIu32, have_width,
                want_width);
  }
  uint32_t have_height = wuffs_base__pixel_config__height(&ic.pixcfg);
  uint32_t want_height = 120;
  if (have_height != want_height) {
    RETURN_FAIL("height: have %" PRIu32 ", want %" PRIu32, have_height,
                want_height);
  }
  return NULL;
}

// ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

// No mimic tests.

#endif  // WUFFS_MIMIC

// ---------------- WBMP Benches

const char*  //
do_bench_wuffs_pixel_swizzler(uint32_t dst_pixfmt_repr,
                              uint32_t src_pixfmt_repr,
                              wuffs_base__pixel_blend pixblend,
                              uint64_t iters_unscaled) {
  const uint32_t width = 80;
  const uint32_t height = 60;

  wuffs_base__pixel_format dst_pixfmt =
      wuffs_base__make_pixel_format(dst_pixfmt_repr);
  wuffs_base__pixel_format src_pixfmt =
      wuffs_base__make_pixel_format(src_pixfmt_repr);
  if ((wuffs_base__pixel_format__bits_per_pixel(&dst_pixfmt) & 7) != 0) {
    return "dst pixfmt has fractional bytes per pixel";
  }
  if ((wuffs_base__pixel_format__bits_per_pixel(&src_pixfmt) & 7) != 0) {
    return "src pixfmt has fractional bytes per pixel";
  }
  const uint32_t dst_bytes_per_row =
      width * wuffs_base__pixel_format__bits_per_pixel(&dst_pixfmt) / 8;
  const uint32_t src_bytes_per_row =
      width * wuffs_base__pixel_format__bits_per_pixel(&src_pixfmt) / 8;

  if (g_have_slice_u8.len < (dst_bytes_per_row * height)) {
    return "dst buffer is too short";
  }
  wuffs_base__io_buffer src = wuffs_base__slice_u8__writer(g_src_slice_u8);
  CHECK_STRING(read_file(&src, "test/data/pi.txt"));
  if (src.meta.wi < (src_bytes_per_row * height)) {
    return "src data is too short";
  }

  uint8_t dst_palette[1024];
  uint8_t src_palette[1024];
  memcpy(dst_palette, g_src_slice_u8.ptr, 1024);
  memcpy(src_palette, g_src_slice_u8.ptr, 1024);

  wuffs_base__pixel_swizzler swizzler;
  CHECK_STATUS(
      "prepare",
      wuffs_base__pixel_swizzler__prepare(
          &swizzler, dst_pixfmt, wuffs_base__make_slice_u8(dst_palette, 1024),
          src_pixfmt, wuffs_base__make_slice_u8(src_palette, 1024), pixblend));

  bench_start();
  uint64_t n_bytes = 0;
  uint64_t i;
  uint64_t iters = iters_unscaled * g_flags.iterscale;
  for (i = 0; i < iters; i++) {
    uint32_t y;
    for (y = 0; y < height; y++) {
      wuffs_base__pixel_swizzler__swizzle_interleaved_from_slice(
          &swizzler,
          wuffs_base__make_slice_u8(
              g_have_slice_u8.ptr + (dst_bytes_per_row * y), dst_bytes_per_row),
          wuffs_base__make_slice_u8(dst_palette, 1024),
          wuffs_base__make_slice_u8(
              g_src_slice_u8.ptr + (src_bytes_per_row * y), src_bytes_per_row));
    }
    n_bytes += dst_bytes_per_row * height;
  }
  bench_finish(iters, n_bytes);
  return NULL;
}

const char*  //
bench_wuffs_pixel_swizzler_bgr_565_rgba_nonpremul_src() {
  CHECK_FOCUS(__func__);
  return do_bench_wuffs_pixel_swizzler(WUFFS_BASE__PIXEL_FORMAT__BGR_565,
                                       WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL,
                                       WUFFS_BASE__PIXEL_BLEND__SRC, 400);
}

const char*  //
bench_wuffs_pixel_swizzler_bgr_rgba_nonpremul_src() {
  CHECK_FOCUS(__func__);
  return do_bench_wuffs_pixel_swizzler(WUFFS_BASE__PIXEL_FORMAT__BGR,
                                       WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL,
                                       WUFFS_BASE__PIXEL_BLEND__SRC, 500);
}

const char*  //
bench_wuffs_pixel_swizzler_bgra_nonpremul_rgba_nonpremul_src() {
  CHECK_FOCUS(__func__);
  return do_bench_wuffs_pixel_swizzler(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL,
                                       WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL,
                                       WUFFS_BASE__PIXEL_BLEND__SRC, 8000);
}

const char*  //
bench_wuffs_pixel_swizzler_bgra_premul_y_src() {
  CHECK_FOCUS(__func__);
  return do_bench_wuffs_pixel_swizzler(WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
                                       WUFFS_BASE__PIXEL_FORMAT__Y,
                                       WUFFS_BASE__PIXEL_BLEND__SRC, 3000);
}

const char*  //
bench_wuffs_pixel_swizzler_bgra_premul_indexed_bgra_binary_src() {
  CHECK_FOCUS(__func__);
  return do_bench_wuffs_pixel_swizzler(
      WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
      WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY,
      WUFFS_BASE__PIXEL_BLEND__SRC, 2000);
}

const char*  //
bench_wuffs_pixel_swizzler_bgra_premul_rgb_src() {
  CHECK_FOCUS(__func__);
  return do_bench_wuffs_pixel_swizzler(WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
                                       WUFFS_BASE__PIXEL_FORMAT__RGB,
                                       WUFFS_BASE__PIXEL_BLEND__SRC, 2000);
}

const char*  //
bench_wuffs_pixel_swizzler_bgra_premul_rgba_nonpremul_src() {
  CHECK_FOCUS(__func__);
  return do_bench_wuffs_pixel_swizzler(WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
                                       WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL,
                                       WUFFS_BASE__PIXEL_BLEND__SRC, 1000);
}

const char*  //
bench_wuffs_pixel_swizzler_bgra_premul_rgba_nonpremul_src_over() {
  CHECK_FOCUS(__func__);
  return do_bench_wuffs_pixel_swizzler(WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
                                       WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL,
                                       WUFFS_BASE__PIXEL_BLEND__SRC_OVER, 300);
}

// ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

// No mimic benches.

#endif  // WUFFS_MIMIC

// ---------------- Manifest

proc g_tests[] = {

    // These pixel_buffer / pixel_swizzler tests are really testing the Wuffs
    // base library. They aren't specific to the std/wbmp code, but putting
    // them here is as good as any other place.
    test_wuffs_pixel_buffer_fill_rect,
    test_wuffs_pixel_swizzler_swizzle,

    test_wuffs_wbmp_decode_frame_config,
    test_wuffs_wbmp_decode_image_config,
    test_wuffs_wbmp_decode_interface,

#ifdef WUFFS_MIMIC

// No mimic tests.

#endif  // WUFFS_MIMIC

    NULL,
};

proc g_benches[] = {

    bench_wuffs_pixel_swizzler_bgr_565_rgba_nonpremul_src,
    bench_wuffs_pixel_swizzler_bgr_rgba_nonpremul_src,
    bench_wuffs_pixel_swizzler_bgra_nonpremul_rgba_nonpremul_src,
    bench_wuffs_pixel_swizzler_bgra_premul_y_src,
    bench_wuffs_pixel_swizzler_bgra_premul_indexed_bgra_binary_src,
    bench_wuffs_pixel_swizzler_bgra_premul_rgb_src,
    bench_wuffs_pixel_swizzler_bgra_premul_rgba_nonpremul_src,
    bench_wuffs_pixel_swizzler_bgra_premul_rgba_nonpremul_src_over,

#ifdef WUFFS_MIMIC

// No mimic benches.

#endif  // WUFFS_MIMIC

    NULL,
};

int  //
main(int argc, char** argv) {
  g_proc_package_name = "std/wbmp";
  return test_main(argc, argv, g_tests, g_benches);
}
