// Copyright 2020 The Wuffs Authors.
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
  $CC -std=c99 -Wall -Werror nie.c && ./a.out
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
#define WUFFS_CONFIG__MODULE__NIE

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
// No mimic library.
#endif

// ---------------- NIE Tests

const char*  //
test_wuffs_nie_decode_interface() {
  CHECK_FOCUS(__func__);
  wuffs_nie__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_nie__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
  return do_test__wuffs_base__image_decoder(
      wuffs_nie__decoder__upcast_as__wuffs_base__image_decoder(&dec),
      "test/data/hippopotamus.nie", 0, SIZE_MAX, 36, 28, 0xFFF5F5F5);
}

const char*  //
test_wuffs_nie_decode_truncated_input() {
  CHECK_FOCUS(__func__);

  wuffs_base__io_buffer src =
      wuffs_base__ptr_u8__reader(g_src_array_u8, 0, false);
  wuffs_nie__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_nie__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  wuffs_base__status status =
      wuffs_nie__decoder__decode_image_config(&dec, NULL, &src);
  if (status.repr != wuffs_base__suspension__short_read) {
    RETURN_FAIL("closed=false: have \"%s\", want \"%s\"", status.repr,
                wuffs_base__suspension__short_read);
  }

  src.meta.closed = true;
  status = wuffs_nie__decoder__decode_image_config(&dec, NULL, &src);
  if (status.repr != wuffs_nie__error__truncated_input) {
    RETURN_FAIL("closed=true: have \"%s\", want \"%s\"", status.repr,
                wuffs_nie__error__truncated_input);
  }
  return NULL;
}

const char*  //
test_wuffs_nie_decode_frame_config() {
  CHECK_FOCUS(__func__);
  wuffs_nie__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_nie__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  wuffs_base__frame_config fc = ((wuffs_base__frame_config){});
  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = g_src_slice_u8,
  });
  CHECK_STRING(read_file(&src, "test/data/crude-flag.nie"));
  CHECK_STATUS("decode_frame_config #0",
               wuffs_nie__decoder__decode_frame_config(&dec, &fc, &src));

  wuffs_base__status status =
      wuffs_nie__decoder__decode_frame_config(&dec, &fc, &src);
  if (status.repr != wuffs_base__note__end_of_data) {
    RETURN_FAIL("decode_frame_config #1: have \"%s\", want \"%s\"", status.repr,
                wuffs_base__note__end_of_data);
  }
  return NULL;
}

const char*  //
do_test_wuffs_nie_decode_animation(bool call_decode_frame) {
  wuffs_nie__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_nie__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = g_src_slice_u8,
  });
  CHECK_STRING(read_file(&src, "test/data/crude-flag.nia"));

  wuffs_base__pixel_buffer pb = ((wuffs_base__pixel_buffer){});

  {
    wuffs_base__image_config ic = ((wuffs_base__image_config){});
    CHECK_STATUS("decode_image_config",
                 wuffs_nie__decoder__decode_image_config(&dec, &ic, &src));
    if (wuffs_base__pixel_config__pixel_format(&ic.pixcfg).repr !=
        WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL) {
      RETURN_FAIL("pixel_format: have 0x%08" PRIX32 ", want 0x%08" PRIX32,
                  wuffs_base__pixel_config__pixel_format(&ic.pixcfg).repr,
                  WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL);
    }

    if (wuffs_base__pixel_config__width(&ic.pixcfg) != 3) {
      RETURN_FAIL("width: have %" PRIu32 ", want 3",
                  wuffs_base__pixel_config__width(&ic.pixcfg));
    }
    if (wuffs_base__pixel_config__height(&ic.pixcfg) != 2) {
      RETURN_FAIL("height: have %" PRIu32 ", want 2",
                  wuffs_base__pixel_config__height(&ic.pixcfg));
    }

    wuffs_base__pixel_config__set(&ic.pixcfg,
                                  WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL,
                                  WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, 3, 2);

    CHECK_STATUS("set_from_slice", wuffs_base__pixel_buffer__set_from_slice(
                                       &pb, &ic.pixcfg, g_pixel_slice_u8));
  }

  for (int i = 0; true; i++) {
    wuffs_base__frame_config fc = ((wuffs_base__frame_config){});
    wuffs_base__status status =
        wuffs_nie__decoder__decode_frame_config(&dec, &fc, &src);
    if (i >= 2) {
      if (status.repr != wuffs_base__note__end_of_data) {
        RETURN_FAIL("decode_frame_config: have \"%s\", want \"%s\"",
                    status.repr, wuffs_base__note__end_of_data);
      }
      break;
    }
    CHECK_STATUS("decode_frame_config", status);

    {
      uint32_t have = wuffs_nie__decoder__num_decoded_frame_configs(&dec);
      uint32_t want = i + 1u;
      if (have != want) {
        RETURN_FAIL("num_decoded_frame_configs: have %" PRIu32
                    ", want %" PRIu32,
                    have, want);
      }
    }

    {
      uint32_t have = wuffs_nie__decoder__num_decoded_frames(&dec);
      uint32_t want = i;
      if (have != want) {
        RETURN_FAIL("num_decoded_frames: have %" PRIu32 ", want %" PRIu32, have,
                    want);
      }
    }

    {
      static const uint64_t wants[] = {0x10, 0x40};
      uint64_t have = wuffs_base__frame_config__io_position(&fc);
      uint64_t want = wants[i];
      if (have != want) {
        RETURN_FAIL("io_position: have %" PRIu64 ", want %" PRIu64, have, want);
      }
    }

    {
      static const uint64_t wants[] = {
          1 * WUFFS_BASE__FLICKS_PER_SECOND,
          2 * WUFFS_BASE__FLICKS_PER_SECOND,
      };
      uint64_t have = wuffs_base__frame_config__duration(&fc);
      uint64_t want = wants[i];
      if (have != want) {
        RETURN_FAIL("duration: have %" PRIu64 ", want %" PRIu64, have, want);
      }
    }

    if (!call_decode_frame) {
      continue;
    }

    CHECK_STATUS("decode_frame",
                 wuffs_nie__decoder__decode_frame(
                     &dec, &pb, &src, WUFFS_BASE__PIXEL_BLEND__SRC,
                     wuffs_base__empty_slice_u8(), NULL));

    {
      static const wuffs_base__color_u32_argb_premul wants[] = {
          0xFF0000FFu,  // Blue.
          0xFF00FF00u,  // Green.
      };
      wuffs_base__color_u32_argb_premul have =
          wuffs_base__pixel_buffer__color_u32_at(&pb, 0, 0);
      wuffs_base__color_u32_argb_premul want = wants[i];
      if (have != want) {
        RETURN_FAIL("color: have 0x%08" PRIX32 ", want 0x%08" PRIX32, have,
                    want);
      }
    }

    {
      uint32_t have = wuffs_nie__decoder__num_animation_loops(&dec);
      uint32_t want = (i >= 1) ? 10 : 0;
      if (have != want) {
        RETURN_FAIL("num_animation_loops: have %" PRIu32 ", want %" PRIu32,
                    have, want);
      }
    }
  }

  return NULL;
}

const char*  //
test_wuffs_nie_decode_animation_sans_decode_frame() {
  CHECK_FOCUS(__func__);
  return do_test_wuffs_nie_decode_animation(false);
}

const char*  //
test_wuffs_nie_decode_animation_with_decode_frame() {
  CHECK_FOCUS(__func__);
  return do_test_wuffs_nie_decode_animation(true);
}

// ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

// No mimic tests.

#endif  // WUFFS_MIMIC

// ---------------- NIE Benches

// No NIE benches.

// ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

// No mimic benches.

#endif  // WUFFS_MIMIC

// ---------------- Manifest

proc g_tests[] = {

    test_wuffs_nie_decode_animation_sans_decode_frame,
    test_wuffs_nie_decode_animation_with_decode_frame,
    test_wuffs_nie_decode_frame_config,
    test_wuffs_nie_decode_interface,
    test_wuffs_nie_decode_truncated_input,

#ifdef WUFFS_MIMIC

// No mimic tests.

#endif  // WUFFS_MIMIC

    NULL,
};

proc g_benches[] = {

// No NIE benches.

#ifdef WUFFS_MIMIC

// No mimic benches.

#endif  // WUFFS_MIMIC

    NULL,
};

int  //
main(int argc, char** argv) {
  g_proc_package_name = "std/nie";
  return test_main(argc, argv, g_tests, g_benches);
}
