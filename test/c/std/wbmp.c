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

// !! wuffs mimic cflags: -DWUFFS_MIMIC

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c whitelist which parts of Wuffs to build. That file contains
// the entire Wuffs standard library, implementing a variety of codecs and file
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
test_wuffs_pixel_swizzler_swizzle() {
  CHECK_FOCUS(__func__);

  const uint32_t width = 5;
  const uint32_t height = 5;
  uint8_t dummy_palette_array[1024];
  wuffs_base__pixel_swizzler swizzler;

  const struct {
    wuffs_base__color_u32_argb_premul pixel;
    uint32_t pixfmt_repr;
  } srcs[] = {
      {
          .pixel = 0xFF444444,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__Y,
      },
      {
          .pixel = 0xFF444444,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY,
      },
      {
          .pixel = 0xFF443300,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGR,
      },
      {
          .pixel = 0x55443300,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL,
      },
  };

  const struct {
    wuffs_base__color_u32_argb_premul pixel;
    uint32_t pixfmt_repr;
  } dsts[] = {
      {
          .pixel = 0xFF000010,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGR_565,
      },
      {
          .pixel = 0xFF000040,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGR,
      },
      {
          .pixel = 0x80000040,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL,
      },
      {
          .pixel = 0x80000040,
          .pixfmt_repr = WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
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
    fill_palette_with_grays(&src_pixbuf);

    // Set and check the middle src pixel.
    CHECK_STATUS("set_color_u32_at",
                 wuffs_base__pixel_buffer__set_color_u32_at(
                     &src_pixbuf, width / 2, height / 2, srcs[s].pixel));
    wuffs_base__color_u32_argb_premul have_src_pixel =
        wuffs_base__pixel_buffer__color_u32_at(&src_pixbuf, width / 2,
                                               height / 2);
    if (have_src_pixel != srcs[s].pixel) {
      RETURN_FAIL("s=%d: src_pixel: have 0x%08" PRIX32 ", want 0x%08" PRIX32, s,
                  have_src_pixel, srcs[s].pixel);
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
      fill_palette_with_grays(&dst_pixbuf);
      wuffs_base__pixel_format dst_pixfmt =
          wuffs_base__make_pixel_format(dsts[d].pixfmt_repr);
      wuffs_base__pixel_alpha_transparency dst_transparency =
          wuffs_base__pixel_format__transparency(&dst_pixfmt);

      wuffs_base__slice_u8 dst_palette =
          wuffs_base__pixel_buffer__palette(&dst_pixbuf);
      if (dst_palette.len == 0) {
        dst_palette = wuffs_base__make_slice_u8(
            &dummy_palette_array[0],
            WUFFS_TESTLIB_ARRAY_SIZE(dummy_palette_array));
      }

      int b;
      for (b = 0; b < WUFFS_TESTLIB_ARRAY_SIZE(blends); b++) {
        // Set the middle dst pixel.
        CHECK_STATUS("set_color_u32_at",
                     wuffs_base__pixel_buffer__set_color_u32_at(
                         &dst_pixbuf, width / 2, height / 2, dsts[d].pixel));

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
          want_dst_pixel = srcs[s].pixel;
        } else if (blends[b] == WUFFS_BASE__PIXEL_BLEND__SRC_OVER) {
          tolerance += 1;
          want_dst_pixel = wuffs_base__composite_premul_premul_u32_axxx(
              dsts[d].pixel, srcs[s].pixel);
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
  if (src.meta.ri != src.meta.wi) {
    RETURN_FAIL("at end of data: ri (%zu) doesn't equal wi (%zu)", src.meta.ri,
                src.meta.wi);
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

// No WBMP benches.

// ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

// No mimic benches.

#endif  // WUFFS_MIMIC

// ---------------- Manifest

proc g_tests[] = {

    // These pixel_swizzler tests are really testing the Wuffs base library.
    // They aren't specific to the std/wbmp code, but putting them here is as
    // good as any other place.
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

// No WBMP benches.

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
