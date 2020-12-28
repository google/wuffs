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
  $CC -std=c99 -Wall -Werror png.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).

Add the "wuffs mimic cflags" (everything after the colon below) to the C
compiler flags (after the .c file) to run the mimic tests.

To manually run the benchmarks, replace "-Wall -Werror" with "-O3" and replace
the first "./a.out" with "./a.out -bench". Combine these changes with the
"wuffs mimic cflags" to run the mimic benchmarks.
*/

// !! wuffs mimic cflags: -DWUFFS_MIMIC -lpng

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
#define WUFFS_CONFIG__MODULE__ADLER32
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__CRC32
#define WUFFS_CONFIG__MODULE__DEFLATE
#define WUFFS_CONFIG__MODULE__PNG
#define WUFFS_CONFIG__MODULE__ZLIB

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/png.c"
#endif

// ---------------- PNG Tests

const char*  //
wuffs_png_decode(uint64_t* n_bytes_out,
                 wuffs_base__io_buffer* dst,
                 uint32_t wuffs_initialize_flags,
                 wuffs_base__pixel_format pixfmt,
                 wuffs_base__io_buffer* src) {
  wuffs_png__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_png__decoder__initialize(&dec, sizeof dec, WUFFS_VERSION,
                                              wuffs_initialize_flags));
  return do_run__wuffs_base__image_decoder(
      wuffs_png__decoder__upcast_as__wuffs_base__image_decoder(&dec),
      n_bytes_out, dst, pixfmt, src);
}

const char*  //
do_wuffs_png_swizzle(wuffs_png__decoder* dec,
                     uint32_t width,
                     uint32_t height,
                     uint8_t filter_distance,
                     wuffs_base__slice_u8 dst,
                     wuffs_base__slice_u8 workbuf) {
  dec->private_impl.f_width = width;
  dec->private_impl.f_height = height;
  dec->private_impl.f_bytes_per_row = width;
  dec->private_impl.f_filter_distance = filter_distance;

  CHECK_STATUS("prepare",
               wuffs_base__pixel_swizzler__prepare(
                   &dec->private_impl.f_swizzler,
                   wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y),
                   wuffs_base__empty_slice_u8(),
                   wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y),
                   wuffs_base__empty_slice_u8(), WUFFS_BASE__PIXEL_BLEND__SRC));

  wuffs_base__pixel_config pc = ((wuffs_base__pixel_config){});
  wuffs_base__pixel_config__set(&pc, WUFFS_BASE__PIXEL_FORMAT__Y,
                                WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width,
                                height);
  wuffs_base__pixel_buffer pb = ((wuffs_base__pixel_buffer){});

  CHECK_STATUS("set_from_slice",
               wuffs_base__pixel_buffer__set_from_slice(&pb, &pc, dst));
  CHECK_STATUS("filter_and_swizzle",
               wuffs_png__decoder__filter_and_swizzle(dec, &pb, workbuf));
  return NULL;
}

// --------

const char*  //
test_wuffs_png_decode_interface() {
  CHECK_FOCUS(__func__);
  wuffs_png__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_png__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
  return do_test__wuffs_base__image_decoder(
      wuffs_png__decoder__upcast_as__wuffs_base__image_decoder(&dec),
      "test/data/bricks-gray.png", 0, SIZE_MAX, 160, 120, 0xFF060606);
}

const char*  //
test_wuffs_png_decode_filters() {
  CHECK_FOCUS(__func__);

  uint8_t src_rows[2][12] = {
      // "WhatsInAName".
      {0x57, 0x68, 0x61, 0x74, 0x73, 0x49, 0x6E, 0x41, 0x4E, 0x61, 0x6D, 0x65},
      // "SmellAsSweet".
      {0x53, 0x6D, 0x65, 0x6C, 0x6C, 0x41, 0x73, 0x53, 0x77, 0x65, 0x65, 0x74},
  };

  uint8_t want_rows[4 * 4 * 2][12] = {
      // Sub:1.
      {0x57, 0xBF, 0x20, 0x94, 0x07, 0x50, 0xBE, 0xFF, 0x4D, 0xAE, 0x1B, 0x80},
      {0x53, 0xC0, 0x25, 0x91, 0xFD, 0x3E, 0xB1, 0x04, 0x7B, 0xE0, 0x45, 0xB9},
      // Sub:2.
      {0x57, 0x68, 0xB8, 0xDC, 0x2B, 0x25, 0x99, 0x66, 0xE7, 0xC7, 0x54, 0x2C},
      {0x53, 0x6D, 0xB8, 0xD9, 0x24, 0x1A, 0x97, 0x6D, 0x0E, 0xD2, 0x73, 0x46},
      // Sub:3.
      {0x57, 0x68, 0x61, 0xCB, 0xDB, 0xAA, 0x39, 0x1C, 0xF8, 0x9A, 0x89, 0x5D},
      {0x53, 0x6D, 0x65, 0xBF, 0xD9, 0xA6, 0x32, 0x2C, 0x1D, 0x97, 0x91, 0x91},
      // Sub:4.
      {0x57, 0x68, 0x61, 0x74, 0xCA, 0xB1, 0xCF, 0xB5, 0x18, 0x12, 0x3C, 0x1A},
      {0x53, 0x6D, 0x65, 0x6C, 0xBF, 0xAE, 0xD8, 0xBF, 0x36, 0x13, 0x3D, 0x33},
      // Up:1.
      {0x57, 0x68, 0x61, 0x74, 0x73, 0x49, 0x6E, 0x41, 0x4E, 0x61, 0x6D, 0x65},
      {0xAA, 0xD5, 0xC6, 0xE0, 0xDF, 0x8A, 0xE1, 0x94, 0xC5, 0xC6, 0xD2, 0xD9},
      // Up:2.
      {0x57, 0x68, 0x61, 0x74, 0x73, 0x49, 0x6E, 0x41, 0x4E, 0x61, 0x6D, 0x65},
      {0xAA, 0xD5, 0xC6, 0xE0, 0xDF, 0x8A, 0xE1, 0x94, 0xC5, 0xC6, 0xD2, 0xD9},
      // Up:3.
      {0x57, 0x68, 0x61, 0x74, 0x73, 0x49, 0x6E, 0x41, 0x4E, 0x61, 0x6D, 0x65},
      {0xAA, 0xD5, 0xC6, 0xE0, 0xDF, 0x8A, 0xE1, 0x94, 0xC5, 0xC6, 0xD2, 0xD9},
      // Up:4.
      {0x57, 0x68, 0x61, 0x74, 0x73, 0x49, 0x6E, 0x41, 0x4E, 0x61, 0x6D, 0x65},
      {0xAA, 0xD5, 0xC6, 0xE0, 0xDF, 0x8A, 0xE1, 0x94, 0xC5, 0xC6, 0xD2, 0xD9},
      // Average:1.
      {0x57, 0x93, 0xAA, 0xC9, 0xD7, 0xB4, 0xC8, 0xA5, 0xA0, 0xB1, 0xC5, 0xC7},
      {0x7E, 0xF5, 0x34, 0xEA, 0x4C, 0xC1, 0x37, 0xC1, 0x27, 0xD1, 0x30, 0xEF},
      // Average:2.
      {0x57, 0x68, 0x8C, 0xA8, 0xB9, 0x9D, 0xCA, 0x8F, 0xB3, 0xA8, 0xC6, 0xB9},
      {0x7E, 0xA1, 0xEA, 0x10, 0x3D, 0x97, 0xF6, 0xE6, 0x4B, 0x2C, 0xED, 0xE6},
      // Average:3.
      {0x57, 0x68, 0x61, 0x9F, 0xA7, 0x79, 0xBD, 0x94, 0x8A, 0xBF, 0xB7, 0xAA},
      {0x7E, 0xA1, 0x95, 0xFA, 0x10, 0xC8, 0x4E, 0xA5, 0x20, 0xEB, 0x13, 0xD9},
      // Average:4.
      {0x57, 0x68, 0x61, 0x74, 0x9E, 0x7D, 0x9E, 0x7B, 0x9D, 0x9F, 0xBC, 0xA2},
      {0x7E, 0xA1, 0x95, 0xA6, 0xFA, 0xD0, 0x0C, 0xE3, 0x42, 0x1C, 0xC9, 0x36},
      // Paeth:1.
      {0x57, 0xBF, 0x20, 0x94, 0x07, 0x50, 0xBE, 0xFF, 0x4D, 0xAE, 0x1B, 0x80},
      {0xAA, 0x2C, 0x85, 0x00, 0x6C, 0xAD, 0x31, 0x84, 0xC4, 0x29, 0x80, 0xF4},
      // Paeth:2.
      {0x57, 0x68, 0xB8, 0xDC, 0x2B, 0x25, 0x99, 0x66, 0xE7, 0xC7, 0x54, 0x2C},
      {0xAA, 0xD5, 0x1D, 0x48, 0x89, 0x66, 0x0C, 0xB9, 0x10, 0x2C, 0x75, 0xA0},
      // Paeth:3.
      {0x57, 0x68, 0x61, 0xCB, 0xDB, 0xAA, 0x39, 0x1C, 0xF8, 0x9A, 0x89, 0x5D},
      {0xAA, 0xD5, 0xC6, 0x37, 0x47, 0x07, 0xAA, 0x6F, 0x7E, 0x0F, 0xEE, 0xD1},
      // Paeth:4.
      {0x57, 0x68, 0x61, 0x74, 0xCA, 0xB1, 0xCF, 0xB5, 0x18, 0x12, 0x3C, 0x1A},
      {0xAA, 0xD5, 0xC6, 0xE0, 0x36, 0x16, 0x42, 0x33, 0x8F, 0x77, 0xA1, 0x8E},
  };

  wuffs_png__decoder dec;
  CHECK_STATUS("initialize", wuffs_png__decoder__initialize(
                                 &dec, sizeof dec, WUFFS_VERSION,
                                 WUFFS_INITIALIZE__DEFAULT_OPTIONS));

  int filter;
  for (filter = 1; filter <= 4; filter++) {
    int filter_distance;
    for (filter_distance = 1; filter_distance <= 4; filter_distance++) {
      // For the top row, the Paeth filter (4) is equivalent to the Sub filter
      // (1), but the Paeth implementation is simpler if it can assume that
      // there is a previous row.
      uint8_t top_row_filter = (filter != 4) ? filter : 1;

      g_work_slice_u8.ptr[13 * 0] = top_row_filter;
      memcpy(g_work_slice_u8.ptr + (13 * 0) + 1, src_rows[0], 12);
      g_work_slice_u8.ptr[13 * 1] = filter;
      memcpy(g_work_slice_u8.ptr + (13 * 1) + 1, src_rows[1], 12);

      CHECK_STRING(do_wuffs_png_swizzle(
          &dec, 12, 2, filter_distance, g_have_slice_u8,
          wuffs_base__make_slice_u8(g_work_slice_u8.ptr, 13 * 2)));

      wuffs_base__io_buffer have =
          wuffs_base__ptr_u8__reader(g_have_slice_u8.ptr, 12 * 2, true);
      have.meta.ri = have.meta.wi;

      int index = (8 * (filter - 1)) + (2 * (filter_distance - 1));
      memcpy(g_want_slice_u8.ptr + (12 * 0), want_rows[index + 0], 12);
      memcpy(g_want_slice_u8.ptr + (12 * 1), want_rows[index + 1], 12);

      wuffs_base__io_buffer want =
          wuffs_base__ptr_u8__reader(g_want_slice_u8.ptr, 12 * 2, true);
      want.meta.ri = want.meta.wi;

      char prefix_buf[256];
      sprintf(prefix_buf, "filter=%d, filter_distance=%d ", filter,
              filter_distance);
      CHECK_STRING(check_io_buffers_equal(prefix_buf, &have, &want));
    }
  }

  return NULL;
}

const char*  //
test_wuffs_png_decode_frame_config() {
  CHECK_FOCUS(__func__);
  wuffs_png__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_png__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  wuffs_base__frame_config fc = ((wuffs_base__frame_config){});
  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = g_src_slice_u8,
  });
  CHECK_STRING(read_file(&src, "test/data/hibiscus.regular.png"));
  CHECK_STATUS("decode_frame_config #0",
               wuffs_png__decoder__decode_frame_config(&dec, &fc, &src));

  wuffs_base__status status =
      wuffs_png__decoder__decode_frame_config(&dec, &fc, &src);
  if (status.repr != wuffs_base__note__end_of_data) {
    RETURN_FAIL("decode_frame_config #1: have \"%s\", want \"%s\"", status.repr,
                wuffs_base__note__end_of_data);
  }
  return NULL;
}

// ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

const char*  //
do_test_mimic_png_decode(const char* filename) {
  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = g_src_slice_u8,
  });
  CHECK_STRING(read_file(&src, filename));

  src.meta.ri = 0;
  wuffs_base__io_buffer have = ((wuffs_base__io_buffer){
      .data = g_have_slice_u8,
  });
  CHECK_STRING(wuffs_png_decode(
      NULL, &have, WUFFS_INITIALIZE__DEFAULT_OPTIONS,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      &src));

  src.meta.ri = 0;
  wuffs_base__io_buffer want = ((wuffs_base__io_buffer){
      .data = g_want_slice_u8,
  });
  CHECK_STRING(mimic_png_decode(
      NULL, &want, WUFFS_INITIALIZE__DEFAULT_OPTIONS,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      &src));

  return check_io_buffers_equal("", &have, &want);
}

const char*  //
test_mimic_png_decode_19k_8bpp() {
  CHECK_FOCUS(__func__);
  // libpng automatically applies the "gAMA" chunk (with no matching "sRGB"
  // chunk) but Wuffs does not. To make the comparison more like-for-like,
  // especially in emitting identical BGRA pixels, patch the source file by
  // replacing the "gAMA" with the nonsense "hAMA". ASCII 'g' is 0x67.
  return do_test_mimic_png_decode("@25=67=68;test/data/bricks-gray.png");
}

const char*  //
test_mimic_png_decode_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_png_decode("test/data/hat.png");
}

const char*  //
test_mimic_png_decode_77k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_png_decode("test/data/bricks-dither.png");
}

const char*  //
test_mimic_png_decode_552k_32bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_png_decode("test/data/hibiscus.primitive.png");
}

const char*  //
test_mimic_png_decode_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_png_decode("test/data/harvesters.png");
}

#endif  // WUFFS_MIMIC

// ---------------- PNG Benches

const char*  //
bench_wuffs_png_decode_19k_8bpp() {
  CHECK_FOCUS(__func__);
  // libpng automatically applies the "gAMA" chunk (with no matching "sRGB"
  // chunk) but Wuffs does not. To make the comparison more like-for-like,
  // especially in emitting identical BGRA pixels, patch the source file by
  // replacing the "gAMA" with the nonsense "hAMA". ASCII 'g' is 0x67.
  return do_bench_image_decode(
      &wuffs_png_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y),
      "@25=67=68;test/data/bricks-gray.png", 0, SIZE_MAX, 50);
}

const char*  //
bench_wuffs_png_decode_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_png_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      "test/data/hat.png", 0, SIZE_MAX, 30);
}

const char*  //
bench_wuffs_png_decode_77k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_png_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      "test/data/bricks-dither.png", 0, SIZE_MAX, 50);
}

const char*  //
bench_wuffs_png_decode_552k_32bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_png_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      "test/data/hibiscus.primitive.png", 0, SIZE_MAX, 4);
}

const char*  //
bench_wuffs_png_decode_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_png_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      "test/data/harvesters.png", 0, SIZE_MAX, 1);
}

// ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

const char*  //
bench_mimic_png_decode_19k_8bpp() {
  CHECK_FOCUS(__func__);
  // libpng automatically applies the "gAMA" chunk (with no matching "sRGB"
  // chunk) but Wuffs does not. To make the comparison more like-for-like,
  // especially in emitting identical BGRA pixels, patch the source file by
  // replacing the "gAMA" with the nonsense "hAMA". ASCII 'g' is 0x67.
  return do_bench_image_decode(
      &mimic_png_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y),
      "@25=67=68;test/data/bricks-gray.png", 0, SIZE_MAX, 50);
}

const char*  //
bench_mimic_png_decode_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_png_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      "test/data/hat.png", 0, SIZE_MAX, 30);
}

const char*  //
bench_mimic_png_decode_77k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_png_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      "test/data/bricks-dither.png", 0, SIZE_MAX, 50);
}

const char*  //
bench_mimic_png_decode_552k_32bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_png_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      "test/data/hibiscus.primitive.png", 0, SIZE_MAX, 4);
}

const char*  //
bench_mimic_png_decode_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_png_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      "test/data/harvesters.png", 0, SIZE_MAX, 1);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

proc g_tests[] = {

    test_wuffs_png_decode_filters,
    test_wuffs_png_decode_frame_config,
    test_wuffs_png_decode_interface,

#ifdef WUFFS_MIMIC

    test_mimic_png_decode_19k_8bpp,
    test_mimic_png_decode_40k_24bpp,
    test_mimic_png_decode_77k_8bpp,
    test_mimic_png_decode_552k_32bpp,
    test_mimic_png_decode_4002k_24bpp,

#endif  // WUFFS_MIMIC

    NULL,
};

proc g_benches[] = {

    bench_wuffs_png_decode_19k_8bpp,
    bench_wuffs_png_decode_40k_24bpp,
    bench_wuffs_png_decode_77k_8bpp,
    bench_wuffs_png_decode_552k_32bpp,
    bench_wuffs_png_decode_4002k_24bpp,

#ifdef WUFFS_MIMIC

    bench_mimic_png_decode_19k_8bpp,
    bench_mimic_png_decode_40k_24bpp,
    bench_mimic_png_decode_77k_8bpp,
    bench_mimic_png_decode_552k_32bpp,
    bench_mimic_png_decode_4002k_24bpp,

#endif  // WUFFS_MIMIC

    NULL,
};

int  //
main(int argc, char** argv) {
  g_proc_package_name = "std/png";
  return test_main(argc, argv, g_tests, g_benches);
}
