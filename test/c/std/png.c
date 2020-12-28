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
