// Copyright 2024 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

// ----------------

/*
This test program is typically run indirectly, by the "wuffs test" or "wuffs
bench" commands. These commands take an optional "-mimic" flag to check that
Wuffs' output mimics (i.e. exactly matches) other libraries' output, such as
giflib for GIF, libpng for PNG, etc.

To manually run this test:

for CC in clang gcc; do
  $CC -std=c99 -Wall -Werror webp.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).

Add the "wuffs mimic cflags" (everything after the colon below) to the C
compiler flags (after the .c file) to run the mimic tests.

To manually run the benchmarks, replace "-Wall -Werror" with "-O3" and replace
the first "./a.out" with "./a.out -bench". Combine these changes with the
"wuffs mimic cflags" to run the mimic benchmarks.
*/

// ¿ wuffs mimic cflags: -DWUFFS_MIMIC -lwebp

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
#define WUFFS_CONFIG__MODULE__VP8
#define WUFFS_CONFIG__MODULE__WEBP

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/webp.c"
#endif

static wuffs_webp__decoder g_webp_decoder;

// ---------------- WebP Tests

const char*  //
wuffs_webp_decode(uint64_t* n_bytes_out,
                  wuffs_base__io_buffer* dst,
                  uint32_t wuffs_initialize_flags,
                  wuffs_base__pixel_format pixfmt,
                  uint32_t* quirks_ptr,
                  size_t quirks_len,
                  wuffs_base__io_buffer* src) {
  wuffs_webp__decoder* dec = &g_webp_decoder;
  CHECK_STATUS("initialize",
               wuffs_webp__decoder__initialize(dec, sizeof *dec, WUFFS_VERSION,
                                               wuffs_initialize_flags));
  return do_run__wuffs_base__image_decoder(
      wuffs_webp__decoder__upcast_as__wuffs_base__image_decoder(dec),
      n_bytes_out, dst, pixfmt, quirks_ptr, quirks_len, src);
}

// --------

const char*  //
test_wuffs_webp_decode_interface_lossless() {
  CHECK_FOCUS(__func__);
  wuffs_webp__decoder* dec = &g_webp_decoder;
  CHECK_STATUS("initialize",
               wuffs_webp__decoder__initialize(
                   dec, sizeof *dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
  return do_test__wuffs_base__image_decoder(
      wuffs_webp__decoder__upcast_as__wuffs_base__image_decoder(dec),
      "test/data/bricks-color.lossless.webp", 160, 120, 0xFF022460, NULL);
}

const char*  //
test_wuffs_webp_decode_interface_lossy() {
  CHECK_FOCUS(__func__);
  wuffs_webp__decoder* dec = &g_webp_decoder;
  CHECK_STATUS("initialize",
               wuffs_webp__decoder__initialize(
                   dec, sizeof *dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
  return do_test__wuffs_base__image_decoder(
      wuffs_webp__decoder__upcast_as__wuffs_base__image_decoder(dec),
      "test/data/bricks-color.lossy.webp", 160, 120, 0xFF032665, NULL);
}

const char*  //
test_wuffs_webp_decode_interface_vp8x_alpha_lossy() {
  CHECK_FOCUS(__func__);
  wuffs_webp__decoder* dec = &g_webp_decoder;
  CHECK_STATUS("initialize",
               wuffs_webp__decoder__initialize(
                   dec, sizeof *dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
  return do_test__wuffs_base__image_decoder(
      wuffs_webp__decoder__upcast_as__wuffs_base__image_decoder(dec),
      "test/data/hippopotamus.masked-with-muybridge.lossy.webp", 36, 28,
      0x7D787878, NULL);
}

const char*  //
test_wuffs_webp_decode_many_small_reads() {
  CHECK_FOCUS(__func__);
  wuffs_webp__decoder* dec = &g_webp_decoder;
  CHECK_STATUS("initialize",
               wuffs_webp__decoder__initialize(
                   dec, sizeof *dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = g_src_slice_u8,
  });
  CHECK_STRING(read_file(&src, "test/data/mona-lisa.21x32.q0.lossy.webp"));

  wuffs_base__image_config ic = ((wuffs_base__image_config){});
  CHECK_STATUS("decode_image_config",
               wuffs_webp__decoder__decode_image_config(dec, &ic, &src));

  const uint32_t w = 21;
  if (wuffs_base__pixel_config__width(&ic.pixcfg) != 21) {
    RETURN_FAIL("width: have %" PRIu32 ", want 21",
                wuffs_base__pixel_config__width(&ic.pixcfg));
  }
  const uint32_t h = 32;
  if (wuffs_base__pixel_config__height(&ic.pixcfg) != 32) {
    RETURN_FAIL("height: have %" PRIu32 ", want 32",
                wuffs_base__pixel_config__height(&ic.pixcfg));
  }
  if (wuffs_base__pixel_config__pixel_format(&ic.pixcfg).repr !=
      WUFFS_BASE__PIXEL_FORMAT__BGRX) {
    RETURN_FAIL("pixel_format: have %" PRIu32
                ", want WUFFS_BASE__PIXEL_FORMAT__BGRX",
                wuffs_base__pixel_config__pixel_format(&ic.pixcfg).repr);
  }

  // 30 is 12 for the RIFF container header, 8 for the RIFF chunk header and
  // 10 for the VP8 header.
  if (wuffs_base__image_config__first_frame_io_position(&ic) != 30) {
    RETURN_FAIL("first_frame_io_position: have %" PRIu64 ", want 30",
                wuffs_base__image_config__first_frame_io_position(&ic));
  }

  wuffs_base__pixel_buffer pb = ((wuffs_base__pixel_buffer){});
  CHECK_STATUS("set_from_slice", wuffs_base__pixel_buffer__set_from_slice(
                                     &pb, &ic.pixcfg, g_pixel_slice_u8));
  wuffs_base__pixel_buffer__set_color_u32_argb_premul_fill_rect(
      &pb, wuffs_base__make_rect_ie_u32(0, 0, w, h), 0xFF234567);

  wuffs_base__range_ii_u64 r = wuffs_webp__decoder__workbuf_len(dec);
  if (wuffs_base__range_ii_u64__is_empty(&r)) {
    RETURN_FAIL("workbuf_len: indeterminate workbuf length");
  }
  uint64_t m = r.min_incl;
  wuffs_base__slice_u8 workbuf = g_work_slice_u8;
  if (workbuf.len > m) {
    workbuf.len = m;
  }

  const uint64_t rlimit = 10;
  int num_iters = 0;
  while (true) {
    num_iters++;
    wuffs_base__io_buffer limited_src = make_limited_reader(src, rlimit);
    size_t old_ri = src.meta.ri;

    wuffs_base__status status = wuffs_webp__decoder__decode_frame(
        dec, &pb, &limited_src, WUFFS_BASE__PIXEL_BLEND__SRC, workbuf, NULL);
    src.meta.ri += limited_src.meta.ri;

    if (wuffs_base__status__is_ok(&status)) {
      break;
    }
    if (status.repr != wuffs_base__suspension__short_read) {
      RETURN_FAIL("decode_frame: have \"%s\", want \"%s\"", status.repr,
                  wuffs_base__suspension__short_read);
    }

    if (src.meta.ri < old_ri) {
      RETURN_FAIL("read index src.meta.ri went backwards");
    } else if (src.meta.ri == old_ri) {
      RETURN_FAIL("no progress was made");
    }
  }

  if (num_iters <= 1) {
    RETURN_FAIL("num_iters: have %d, want > 1", num_iters);
  }

  wuffs_base__color_u32_argb_premul last_pixel =
      wuffs_base__pixel_buffer__color_u32_at(&pb, w - 1, h - 1);
  if (last_pixel != 0xFF210A1B) {
    RETURN_FAIL("last_pixel: have 0x%" PRIX32 ", want 0xFF210A1B", last_pixel);
  }

  return NULL;
}

const char*  //
test_wuffs_webp_decode_filter_alph() {
  CHECK_FOCUS(__func__);

  const int width = 41;
  const int height = 6;

  // src is arbitrary bytes, a subset of a noise texture.
  const uint8_t src[1][246] = {
      {
          0x3C, 0xD4, 0xB6, 0x77, 0x64, 0xAB, 0xEF, 0x03, 0x7D, 0x2F, 0xC0,
          0x24, 0x79, 0x3E, 0x87, 0x9A, 0x36, 0x7F, 0xBC, 0xF0, 0x42, 0xAF,
          0x2D, 0x63, 0x22, 0xC3, 0x4E, 0x33, 0xE9, 0x67, 0x9C, 0x01, 0xA9,
          0x23, 0x50, 0xDD, 0x36, 0x26, 0x4B, 0x94, 0xDC,

          0x14, 0xFB, 0x47, 0x5F, 0xF2, 0x13, 0x53, 0xE7, 0xD7, 0x28, 0x8A,
          0x07, 0xC6, 0x54, 0xE5, 0xB5, 0x0E, 0x78, 0xAE, 0x14, 0x84, 0xC0,
          0x20, 0x7D, 0x95, 0xEA, 0x06, 0x8A, 0xFA, 0xC3, 0x70, 0x5C, 0xA6,
          0x88, 0xCC, 0x09, 0xBA, 0xA6, 0x68, 0x1E, 0xB3,

          0x5A, 0xA5, 0x68, 0xFD, 0x18, 0x90, 0x70, 0x3A, 0xF0, 0x5F, 0xCA,
          0x44, 0x2B, 0x58, 0x12, 0xBD, 0x43, 0x61, 0xB0, 0x9D, 0x0E, 0x3F,
          0xEC, 0xB6, 0x36, 0x6E, 0x94, 0xDE, 0x2F, 0xCE, 0x77, 0x92, 0x11,
          0xE2, 0x37, 0x81, 0xA0, 0x46, 0xCD, 0xDF, 0xA3,

          0x05, 0x97, 0xDC, 0x73, 0xE4, 0x35, 0x6D, 0xF2, 0xC7, 0x1D, 0x56,
          0x7B, 0xCE, 0x2A, 0x17, 0x54, 0xE9, 0x21, 0x7E, 0x4E, 0xEB, 0x3B,
          0xF9, 0x47, 0xCA, 0x75, 0xB9, 0xD9, 0x0B, 0x5A, 0x27, 0x8A, 0x4D,
          0xF5, 0x1F, 0xB7, 0x8E, 0xD0, 0xA4, 0x28, 0x81,

          0xD9, 0x32, 0xE4, 0xA3, 0x8F, 0x65, 0xD8, 0x9B, 0x42, 0xB2, 0x04,
          0x8B, 0x19, 0xBE, 0x9B, 0x00, 0x51, 0x22, 0x31, 0xF6, 0x7C, 0xBF,
          0x1A, 0x6C, 0xB0, 0x3C, 0x63, 0x0A, 0xF9, 0x53, 0x15, 0x93, 0x67,
          0xB8, 0x4B, 0x01, 0xF4, 0x82, 0xBE, 0x11, 0xFD,

          0x61, 0xC4, 0xA4, 0x57, 0x6D, 0x2C, 0xDD, 0xED, 0xB1, 0x92, 0x63,
          0xA8, 0xE2, 0x35, 0xC8, 0x7F, 0xD5, 0x9D, 0x49, 0x85, 0xAE, 0xE2,
          0x41, 0x0C, 0xFE, 0x76, 0xC5, 0x20, 0x45, 0xAC, 0x30, 0xCB, 0x73,
          0x24, 0xEF, 0xDA, 0x80, 0xAB, 0x5E, 0x89, 0x6E,
      },
  };

  const uint8_t wants[3][246] = {
      {
          0x3C, 0x10, 0xC6, 0x3D, 0xA1, 0x4C, 0x3B, 0x3E, 0xBB, 0xEA, 0xAA,
          0xCE, 0x47, 0x85, 0x0C, 0xA6, 0xDC, 0x5B, 0x17, 0x07, 0x49, 0xF8,
          0x25, 0x88, 0xAA, 0x6D, 0xBB, 0xEE, 0xD7, 0x3E, 0xDA, 0xDB, 0x84,
          0xA7, 0xF7, 0xD4, 0x0A, 0x30, 0x7B, 0x0F, 0xEB,

          0x50, 0x4B, 0x92, 0xF1, 0xE3, 0xF6, 0x49, 0x30, 0x07, 0x2F, 0xB9,
          0xC0, 0x86, 0xDA, 0xBF, 0x74, 0x82, 0xFA, 0xA8, 0xBC, 0x40, 0x00,
          0x20, 0x9D, 0x32, 0x1C, 0x22, 0xAC, 0xA6, 0x69, 0xD9, 0x35, 0xDB,
          0x63, 0x2F, 0x38, 0xF2, 0x98, 0x00, 0x1E, 0xD1,

          0xAA, 0x4F, 0xB7, 0xB4, 0xCC, 0x5C, 0xCC, 0x06, 0xF6, 0x55, 0x1F,
          0x63, 0x8E, 0xE6, 0xF8, 0xB5, 0xF8, 0x59, 0x09, 0xA6, 0xB4, 0xF3,
          0xDF, 0x95, 0xCB, 0x39, 0xCD, 0xAB, 0xDA, 0xA8, 0x1F, 0xB1, 0xC2,
          0xA4, 0xDB, 0x5C, 0xFC, 0x42, 0x0F, 0xEE, 0x91,

          0xAF, 0x46, 0x22, 0x95, 0x79, 0xAE, 0x1B, 0x0D, 0xD4, 0xF1, 0x47,
          0xC2, 0x90, 0xBA, 0xD1, 0x25, 0x0E, 0x2F, 0xAD, 0xFB, 0xE6, 0x21,
          0x1A, 0x61, 0x2B, 0xA0, 0x59, 0x32, 0x3D, 0x97, 0xBE, 0x48, 0x95,
          0x8A, 0xA9, 0x60, 0xEE, 0xBE, 0x62, 0x8A, 0x0B,

          0x88, 0xBA, 0x9E, 0x41, 0xD0, 0x35, 0x0D, 0xA8, 0xEA, 0x9C, 0xA0,
          0x2B, 0x44, 0x02, 0x9D, 0x9D, 0xEE, 0x10, 0x41, 0x37, 0xB3, 0x72,
          0x8C, 0xF8, 0xA8, 0xE4, 0x47, 0x51, 0x4A, 0x9D, 0xB2, 0x45, 0xAC,
          0x64, 0xAF, 0xB0, 0xA4, 0x26, 0xE4, 0xF5, 0xF2,

          0xE9, 0xAD, 0x51, 0xA8, 0x15, 0x41, 0x1E, 0x0B, 0xBC, 0x4E, 0xB1,
          0x59, 0x3B, 0x70, 0x38, 0xB7, 0x8C, 0x29, 0x72, 0xF7, 0xA5, 0x87,
          0xC8, 0xD4, 0xD2, 0x48, 0x0D, 0x2D, 0x72, 0x1E, 0x4E, 0x19, 0x8C,
          0xB0, 0x9F, 0x79, 0xF9, 0xA4, 0x02, 0x8B, 0xF9,
      },

      {
          0x3C, 0x10, 0xC6, 0x3D, 0xA1, 0x4C, 0x3B, 0x3E, 0xBB, 0xEA, 0xAA,
          0xCE, 0x47, 0x85, 0x0C, 0xA6, 0xDC, 0x5B, 0x17, 0x07, 0x49, 0xF8,
          0x25, 0x88, 0xAA, 0x6D, 0xBB, 0xEE, 0xD7, 0x3E, 0xDA, 0xDB, 0x84,
          0xA7, 0xF7, 0xD4, 0x0A, 0x30, 0x7B, 0x0F, 0xEB,

          0x50, 0x0B, 0x0D, 0x9C, 0x93, 0x5F, 0x8E, 0x25, 0x92, 0x12, 0x34,
          0xD5, 0x0D, 0xD9, 0xF1, 0x5B, 0xEA, 0xD3, 0xC5, 0x1B, 0xCD, 0xB8,
          0x45, 0x05, 0x3F, 0x57, 0xC1, 0x78, 0xD1, 0x01, 0x4A, 0x37, 0x2A,
          0x2F, 0xC3, 0xDD, 0xC4, 0xD6, 0xE3, 0x2D, 0x9E,

          0xAA, 0xB0, 0x75, 0x99, 0xAB, 0xEF, 0xFE, 0x5F, 0x82, 0x71, 0xFE,
          0x19, 0x38, 0x31, 0x03, 0x18, 0x2D, 0x34, 0x75, 0xB8, 0xDB, 0xF7,
          0x31, 0xBB, 0x75, 0xC5, 0x55, 0x56, 0x00, 0xCF, 0xC1, 0xC9, 0x3B,
          0x11, 0xFA, 0x5E, 0x64, 0x1C, 0xB0, 0x0C, 0x41,

          0xAF, 0x47, 0x51, 0x0C, 0x8F, 0x24, 0x6B, 0x51, 0x49, 0x8E, 0x54,
          0x94, 0x06, 0x5B, 0x1A, 0x6C, 0x16, 0x55, 0xF3, 0x06, 0xC6, 0x32,
          0x2A, 0x02, 0x3F, 0x3A, 0x0E, 0x2F, 0x0B, 0x29, 0xE8, 0x53, 0x88,
          0x06, 0x19, 0x15, 0xF2, 0xEC, 0x54, 0x34, 0xC2,

          0x88, 0x79, 0x35, 0xAF, 0x1E, 0x89, 0x43, 0xEC, 0x8B, 0x40, 0x58,
          0x1F, 0x1F, 0x19, 0xB5, 0x6C, 0x67, 0x77, 0x24, 0xFC, 0x42, 0xF1,
          0x44, 0x6E, 0xEF, 0x76, 0x71, 0x39, 0x04, 0x7C, 0xFD, 0xE6, 0xEF,
          0xBE, 0x64, 0x16, 0xE6, 0x6E, 0x12, 0x45, 0xBF,

          0xE9, 0x3D, 0xD9, 0x06, 0x8B, 0xB5, 0x20, 0xD9, 0x3C, 0xD2, 0xBB,
          0xC7, 0x01, 0x4E, 0x7D, 0xEB, 0x3C, 0x14, 0x6D, 0x81, 0xF0, 0xD3,
          0x85, 0x7A, 0xED, 0xEC, 0x36, 0x59, 0x49, 0x28, 0x2D, 0xB1, 0x62,
          0xE2, 0x53, 0xF0, 0x66, 0x19, 0x70, 0xCE, 0x2D,
      },

      {
          0x3C, 0x10, 0xC6, 0x3D, 0xA1, 0x4C, 0x3B, 0x3E, 0xBB, 0xEA, 0xAA,
          0xCE, 0x47, 0x85, 0x0C, 0xA6, 0xDC, 0x5B, 0x17, 0x07, 0x49, 0xF8,
          0x25, 0x88, 0xAA, 0x6D, 0xBB, 0xEE, 0xD7, 0x3E, 0xDA, 0xDB, 0x84,
          0xA7, 0xF7, 0xD4, 0x0A, 0x30, 0x7B, 0x0F, 0xEB,

          0x50, 0x1F, 0x1C, 0x5F, 0xB5, 0x73, 0xB5, 0x9F, 0xD6, 0x27, 0x8A,
          0xB5, 0xF4, 0x53, 0xE5, 0xB4, 0xF8, 0xEF, 0x59, 0x5D, 0x23, 0x92,
          0x20, 0x00, 0xB7, 0x64, 0xB8, 0x75, 0x58, 0xC3, 0x6F, 0xCC, 0x1B,
          0xC6, 0xCB, 0xB1, 0xBA, 0x86, 0x39, 0x1E, 0xAD,

          0xAA, 0x1E, 0x83, 0xC3, 0x17, 0x90, 0x42, 0x66, 0x8D, 0x5F, 0x8C,
          0xFB, 0x2A, 0x58, 0xFC, 0x88, 0x0F, 0x67, 0xB0, 0x51, 0x25, 0xD3,
          0x4D, 0xE3, 0x35, 0x6E, 0x56, 0xF1, 0x03, 0x3C, 0x77, 0x66, 0x11,
          0x9E, 0xDA, 0x41, 0xEA, 0xFC, 0x7C, 0x40, 0x72,

          0xAF, 0xBA, 0xDB, 0x72, 0xE4, 0x34, 0x6D, 0x83, 0x71, 0x60, 0xE3,
          0x7A, 0xCE, 0x26, 0xE1, 0xC1, 0x31, 0xAA, 0x71, 0x60, 0x1F, 0x08,
          0xF9, 0x46, 0xCA, 0x74, 0x15, 0x89, 0x0B, 0x9E, 0x00, 0x8A, 0x82,
          0xF4, 0x1E, 0xB7, 0x8D, 0x6F, 0xA4, 0x90, 0x43,

          0x88, 0xC5, 0xCA, 0x04, 0x05, 0x65, 0x76, 0x27, 0x57, 0xF8, 0x03,
          0x8B, 0xF8, 0x0E, 0x64, 0x44, 0x51, 0xEC, 0xE4, 0xC9, 0x04, 0xBF,
          0x19, 0x6C, 0xA0, 0x86, 0x8A, 0x08, 0xF9, 0x52, 0x15, 0x32, 0x91,
          0xB7, 0x4B, 0xE5, 0xAF, 0x13, 0x06, 0x11, 0xFD,

          0xE9, 0xC3, 0x6C, 0x57, 0xC5, 0x2B, 0x19, 0xED, 0xB0, 0x91, 0x63,
          0x93, 0xE1, 0x35, 0x53, 0xB2, 0x94, 0x9C, 0xDD, 0x47, 0xAE, 0xE1,
          0x7C, 0xDB, 0xFD, 0x59, 0x22, 0x20, 0x44, 0xAC, 0x9F, 0x87, 0x59,
          0xA3, 0x26, 0x9A, 0xE4, 0xF3, 0x44, 0xD8, 0x6D,
      },
  };

  wuffs_webp__decoder dec = {0};
  dec.private_impl.f_width = width;
  dec.private_impl.f_height = height;

  for (int filter = 0; filter < 3; filter++) {
    uint8_t buf[246];
    memcpy(&buf[0], &src[0][0], 246);
    wuffs_base__slice_u8 s = wuffs_base__make_slice_u8(&buf[0], 246);

    switch (filter) {
      case 0:
        wuffs_webp__decoder__filter_alph_horizontal(&dec, s);
        break;
      case 1:
        wuffs_webp__decoder__filter_alph_vertical(&dec, s);
        break;
      case 2:
        wuffs_webp__decoder__filter_alph_gradient(&dec, s);
        break;
    }

    if (0 == memcmp(&buf[0], &wants[filter][0], 246)) {
      continue;
    }

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        int i = (y * width) + x;
        int have = buf[i];
        int want = wants[filter][i];
        if (have != want) {
          RETURN_FAIL("filter=%d, y=%d, x=%d: have 0x%02X, want 0x%02X", filter,
                      y, x, have, want);
        }
      }
    }

    return "unreachable";
  }
  return NULL;
}

// ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

const char*  //
do_test_mimic_webp_decode(const char* filename) {
  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = g_src_slice_u8,
  });
  CHECK_STRING(read_file(&src, filename));

  src.meta.ri = 0;
  wuffs_base__io_buffer have = ((wuffs_base__io_buffer){
      .data = g_have_slice_u8,
  });
  CHECK_STRING(wuffs_webp_decode(
      NULL, &have, WUFFS_INITIALIZE__DEFAULT_OPTIONS,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, &src));

  src.meta.ri = 0;
  wuffs_base__io_buffer want = ((wuffs_base__io_buffer){
      .data = g_want_slice_u8,
  });
  CHECK_STRING(mimic_webp_decode(
      NULL, &want, WUFFS_INITIALIZE__DEFAULT_OPTIONS,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, &src));

  return check_io_buffers_equal("", &have, &want);
}

const char*  //
test_mimic_webp_lossless_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/bricks-gray.lossless.webp");
}

const char*  //
test_mimic_webp_lossless_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/hat.lossless.webp");
}

const char*  //
test_mimic_webp_lossless_decode_image_77k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/bricks-dither.lossless.webp");
}

const char*  //
test_mimic_webp_lossless_decode_image_552k_32bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode(
      "test/data/hibiscus.primitive.lossless.webp");
}

const char*  //
test_mimic_webp_lossless_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/harvesters.lossless.webp");
}

const char*  //
test_mimic_webp_lossy_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/bricks-gray.lossy.webp");
}

const char*  //
test_mimic_webp_lossy_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/hat.lossy.webp");
}

const char*  //
test_mimic_webp_lossy_decode_image_77k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/bricks-color.lossy.webp");
}

const char*  //
test_mimic_webp_lossy_decode_image_552k_32bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/hibiscus.primitive.lossy.webp");
}

const char*  //
test_mimic_webp_lossy_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_test_mimic_webp_decode("test/data/harvesters.lossy.webp");
}

#endif  // WUFFS_MIMIC

// ---------------- WebP Benches

const char*  //
bench_wuffs_webp_lossless_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y), NULL, 0,
      "test/data/bricks-gray.lossless.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_wuffs_webp_lossless_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/hat.lossless.webp", 0, SIZE_MAX, 30);
}

const char*  //
bench_wuffs_webp_lossless_decode_image_77k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/bricks-dither.lossless.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_wuffs_webp_lossless_decode_image_552k_32bpp() {
  uint32_t q = WUFFS_BASE__QUIRK_IGNORE_CHECKSUM;
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      &q, 1, "test/data/hibiscus.primitive.lossless.webp", 0, SIZE_MAX, 4);
}

const char*  //
bench_wuffs_webp_lossless_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/harvesters.lossless.webp", 0, SIZE_MAX, 1);
}

const char*  //
bench_wuffs_webp_lossy_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y), NULL, 0,
      "test/data/bricks-gray.lossy.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_wuffs_webp_lossy_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/hat.lossy.webp", 0, SIZE_MAX, 30);
}

const char*  //
bench_wuffs_webp_lossy_decode_image_77k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/bricks-color.lossy.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_wuffs_webp_lossy_decode_image_552k_32bpp() {
  uint32_t q = WUFFS_BASE__QUIRK_IGNORE_CHECKSUM;
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      &q, 1, "test/data/hibiscus.primitive.lossy.webp", 0, SIZE_MAX, 4);
}

const char*  //
bench_wuffs_webp_lossy_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/harvesters.lossy.webp", 0, SIZE_MAX, 1);
}

// ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

const char*  //
bench_mimic_webp_lossless_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y), NULL, 0,
      "test/data/bricks-gray.lossless.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_mimic_webp_lossless_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/hat.lossless.webp", 0, SIZE_MAX, 30);
}

const char*  //
bench_mimic_webp_lossless_decode_image_77k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/bricks-dither.lossless.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_mimic_webp_lossless_decode_image_552k_32bpp() {
  uint32_t q = WUFFS_BASE__QUIRK_IGNORE_CHECKSUM;
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      &q, 1, "test/data/hibiscus.primitive.lossless.webp", 0, SIZE_MAX, 4);
}

const char*  //
bench_mimic_webp_lossless_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/harvesters.lossless.webp", 0, SIZE_MAX, 1);
}

const char*  //
bench_mimic_webp_lossy_decode_image_19k_8bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__Y), NULL, 0,
      "test/data/bricks-gray.lossy.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_mimic_webp_lossy_decode_image_40k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/hat.lossy.webp", 0, SIZE_MAX, 30);
}

const char*  //
bench_mimic_webp_lossy_decode_image_77k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/bricks-color.lossy.webp", 0, SIZE_MAX, 50);
}

const char*  //
bench_mimic_webp_lossy_decode_image_552k_32bpp() {
  uint32_t q = WUFFS_BASE__QUIRK_IGNORE_CHECKSUM;
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      &q, 1, "test/data/hibiscus.primitive.lossy.webp", 0, SIZE_MAX, 4);
}

const char*  //
bench_mimic_webp_lossy_decode_image_4002k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &mimic_webp_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/harvesters.lossy.webp", 0, SIZE_MAX, 1);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

proc g_tests[] = {

    test_wuffs_webp_decode_filter_alph,
    test_wuffs_webp_decode_interface_lossless,
    test_wuffs_webp_decode_interface_lossy,
    test_wuffs_webp_decode_interface_vp8x_alpha_lossy,
    test_wuffs_webp_decode_many_small_reads,

#ifdef WUFFS_MIMIC

    test_mimic_webp_lossless_decode_image_19k_8bpp,
    test_mimic_webp_lossless_decode_image_40k_24bpp,
    test_mimic_webp_lossless_decode_image_77k_8bpp,
    test_mimic_webp_lossless_decode_image_552k_32bpp,
    test_mimic_webp_lossless_decode_image_4002k_24bpp,
    test_mimic_webp_lossy_decode_image_19k_8bpp,
    test_mimic_webp_lossy_decode_image_40k_24bpp,
    test_mimic_webp_lossy_decode_image_77k_24bpp,
    test_mimic_webp_lossy_decode_image_552k_32bpp,
    test_mimic_webp_lossy_decode_image_4002k_24bpp,

#endif  // WUFFS_MIMIC

    NULL,
};

proc g_benches[] = {

    bench_wuffs_webp_lossless_decode_image_19k_8bpp,
    bench_wuffs_webp_lossless_decode_image_40k_24bpp,
    bench_wuffs_webp_lossless_decode_image_77k_8bpp,
    bench_wuffs_webp_lossless_decode_image_552k_32bpp,
    bench_wuffs_webp_lossless_decode_image_4002k_24bpp,
    bench_wuffs_webp_lossy_decode_image_19k_8bpp,
    bench_wuffs_webp_lossy_decode_image_40k_24bpp,
    bench_wuffs_webp_lossy_decode_image_77k_24bpp,
    bench_wuffs_webp_lossy_decode_image_552k_32bpp,
    bench_wuffs_webp_lossy_decode_image_4002k_24bpp,

#ifdef WUFFS_MIMIC

    bench_mimic_webp_lossless_decode_image_19k_8bpp,
    bench_mimic_webp_lossless_decode_image_40k_24bpp,
    bench_mimic_webp_lossless_decode_image_77k_8bpp,
    bench_mimic_webp_lossless_decode_image_552k_32bpp,
    bench_mimic_webp_lossless_decode_image_4002k_24bpp,
    bench_mimic_webp_lossy_decode_image_19k_8bpp,
    bench_mimic_webp_lossy_decode_image_40k_24bpp,
    bench_mimic_webp_lossy_decode_image_77k_24bpp,
    bench_mimic_webp_lossy_decode_image_552k_32bpp,
    bench_mimic_webp_lossy_decode_image_4002k_24bpp,

#endif  // WUFFS_MIMIC

    NULL,
};

int  //
main(int argc, char** argv) {
  g_proc_package_name = "std/webp";
  return test_main(argc, argv, g_tests, g_benches);
}
