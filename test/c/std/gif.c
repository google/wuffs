// Copyright 2017 The Wuffs Authors.
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

/*
This test program is typically run indirectly, by the "wuffs test" or "wuffs
bench" commands. These commands take an optional "-mimic" flag to check that
Wuffs' output mimics (i.e. exactly matches) other libraries' output, such as
giflib for GIF, libpng for PNG, etc.

To manually run this test:

for CC in clang gcc; do
  $CC -std=c99 -Wall -Werror gif.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).

Add the "wuffs mimic cflags" (everything after the colon below) to the C
compiler flags (after the .c file) to run the mimic tests.

To manually run the benchmarks, replace "-Wall -Werror" with "-O3" and replace
the first "./a.out" with "./a.out -bench". Combine these changes with the
"wuffs mimic cflags" to run the mimic benchmarks.
*/

// !! wuffs mimic cflags: -DWUFFS_MIMIC -lgif

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.h whitelist which parts of Wuffs to build. That file contains
// the entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__GIF
#define WUFFS_CONFIG__MODULE__LZW

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/unsupported-snapshot.h"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/gif.c"
#endif

// ---------------- Basic Tests

void test_basic_bad_receiver() {
  CHECK_FOCUS(__func__);
  wuffs_base__image_config ic = ((wuffs_base__image_config){});
  wuffs_base__io_reader src = ((wuffs_base__io_reader){});
  wuffs_base__status status = wuffs_gif__decoder__decode_config(NULL, &ic, src);
  if (status != WUFFS_BASE__ERROR_BAD_RECEIVER) {
    FAIL("decode_config: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status), WUFFS_BASE__ERROR_BAD_RECEIVER,
         wuffs_gif__status__string(WUFFS_BASE__ERROR_BAD_RECEIVER));
  }
}

void test_basic_bad_sizeof_receiver() {
  CHECK_FOCUS(__func__);
  wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
  wuffs_gif__decoder__check_wuffs_version(&dec, 0, WUFFS_VERSION);
  if (dec.private_impl.status != WUFFS_BASE__ERROR_BAD_SIZEOF_RECEIVER) {
    FAIL("decode_config: got %" PRIi32 " (%s), want %" PRIi32 " (%s)",
         dec.private_impl.status,
         wuffs_gif__status__string(dec.private_impl.status),
         WUFFS_BASE__ERROR_BAD_SIZEOF_RECEIVER,
         wuffs_gif__status__string(WUFFS_BASE__ERROR_BAD_SIZEOF_RECEIVER));
    return;
  }
}

void test_basic_bad_wuffs_version() {
  CHECK_FOCUS(__func__);
  wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
  wuffs_gif__decoder__check_wuffs_version(&dec, sizeof dec,
                                          WUFFS_VERSION ^ 0x123456789ABC);
  if (dec.private_impl.status != WUFFS_BASE__ERROR_BAD_WUFFS_VERSION) {
    FAIL("decode_config: got %" PRIi32 " (%s), want %" PRIi32 " (%s)",
         dec.private_impl.status,
         wuffs_gif__status__string(dec.private_impl.status),
         WUFFS_BASE__ERROR_BAD_WUFFS_VERSION,
         wuffs_gif__status__string(WUFFS_BASE__ERROR_BAD_WUFFS_VERSION));
    return;
  }
}

void test_basic_check_wuffs_version_not_called() {
  CHECK_FOCUS(__func__);
  wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
  wuffs_base__image_config ic = ((wuffs_base__image_config){});
  wuffs_base__io_reader src = ((wuffs_base__io_reader){});
  wuffs_base__status status = wuffs_gif__decoder__decode_config(&dec, &ic, src);
  if (status != WUFFS_BASE__ERROR_CHECK_WUFFS_VERSION_NOT_CALLED) {
    FAIL("decode_config: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status),
         WUFFS_BASE__ERROR_CHECK_WUFFS_VERSION_NOT_CALLED,
         wuffs_gif__status__string(
             WUFFS_BASE__ERROR_CHECK_WUFFS_VERSION_NOT_CALLED));
  }
}

void test_basic_status_is_error() {
  CHECK_FOCUS(__func__);
  if (wuffs_base__status__is_error(WUFFS_BASE__STATUS_OK)) {
    FAIL("is_error(OK) returned true");
    return;
  }
  if (!wuffs_base__status__is_error(WUFFS_BASE__ERROR_BAD_WUFFS_VERSION)) {
    FAIL("is_error(BAD_WUFFS_VERSION) returned false");
    return;
  }
  if (wuffs_base__status__is_error(WUFFS_BASE__SUSPENSION_SHORT_WRITE)) {
    FAIL("is_error(SHORT_WRITE) returned true");
    return;
  }
  if (!wuffs_base__status__is_error(WUFFS_GIF__ERROR_BAD_HEADER)) {
    FAIL("is_error(BAD_HEADER) returned false");
    return;
  }
}

void test_basic_status_strings() {
  CHECK_FOCUS(__func__);
  const char* s0 = wuffs_gif__status__string(WUFFS_BASE__STATUS_OK);
  const char* t0 = "ok";
  if (strcmp(s0, t0)) {
    FAIL("got \"%s\", want \"%s\"", s0, t0);
    return;
  }
  const char* s1 =
      wuffs_gif__status__string(WUFFS_BASE__ERROR_BAD_WUFFS_VERSION);
  const char* t1 = "bad wuffs version";
  if (strcmp(s1, t1)) {
    FAIL("got \"%s\", want \"%s\"", s1, t1);
    return;
  }
  const char* s2 =
      wuffs_gif__status__string(WUFFS_BASE__SUSPENSION_SHORT_WRITE);
  const char* t2 = "short write";
  if (strcmp(s2, t2)) {
    FAIL("got \"%s\", want \"%s\"", s2, t2);
    return;
  }
  const char* s3 = wuffs_gif__status__string(WUFFS_GIF__ERROR_BAD_HEADER);
  const char* t3 = "gif: bad header";
  if (strcmp(s3, t3)) {
    FAIL("got \"%s\", want \"%s\"", s3, t3);
    return;
  }
  const char* s4 = wuffs_gif__status__string(-254);
  const char* t4 = "unknown status";
  if (strcmp(s4, t4)) {
    FAIL("got \"%s\", want \"%s\"", s4, t4);
    return;
  }
}

void test_basic_status_used_package() {
  CHECK_FOCUS(__func__);
  // The function call here is from "std/gif" but the argument is from
  // "std/lzw". The former package depends on the latter.
  const char* s0 = wuffs_gif__status__string(WUFFS_LZW__ERROR_BAD_CODE);
  const char* t0 = "lzw: bad code";
  if (strcmp(s0, t0)) {
    FAIL("got \"%s\", want \"%s\"", s0, t0);
    return;
  }
}

void test_basic_sub_struct_initializer() {
  CHECK_FOCUS(__func__);
  wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
  wuffs_gif__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);
  if (dec.private_impl.magic != WUFFS_BASE__MAGIC) {
    FAIL("outer magic: got %u, want %u", dec.private_impl.magic,
         WUFFS_BASE__MAGIC);
    return;
  }
  if (dec.private_impl.f_lzw.private_impl.magic != WUFFS_BASE__MAGIC) {
    FAIL("inner magic: got %u, want %u",
         dec.private_impl.f_lzw.private_impl.magic, WUFFS_BASE__MAGIC);
    return;
  }
}

// ---------------- GIF Tests

const char* wuffs_gif_decode(wuffs_base__io_buffer* dst,
                             wuffs_base__io_buffer* src) {
  wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
  wuffs_gif__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);
  wuffs_base__image_buffer ib = ((wuffs_base__image_buffer){});
  wuffs_base__image_config ic = ((wuffs_base__image_config){});
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(src);
  wuffs_base__status s =
      wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
  if (s) {
    return wuffs_gif__status__string(s);
  }
  s = wuffs_base__image_buffer__set_from_slice(
      &ib, ic,
      ((wuffs_base__slice_u8){
          .ptr = global_pixel_buffer,
          .len = WUFFS_TESTLIB_ARRAY_SIZE(global_pixel_buffer),
      }));
  if (s) {
    return wuffs_gif__status__string(s);
  }

  while (true) {
    s = wuffs_gif__decoder__decode_frame(&dec, &ib, src_reader,
                                         ((wuffs_base__slice_u8){}));
    if (s == WUFFS_BASE__SUSPENSION_END_OF_DATA) {
      break;
    }
    if (s) {
      return wuffs_gif__status__string(s);
    }
    const char* msg = copy_to_io_buffer_from_image_buffer(dst, &ib);
    if (msg) {
      return msg;
    }
  }
  return NULL;
}

bool do_test_wuffs_gif_decode(const char* filename,
                              const char* palette_filename,
                              const char* indexes_filename,
                              uint64_t rlimit) {
  wuffs_base__io_buffer got =
      ((wuffs_base__io_buffer){.ptr = global_got_buffer, .len = BUFFER_SIZE});
  wuffs_base__io_buffer src =
      ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});

  if (!read_file(&src, filename)) {
    return false;
  }

  wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
  wuffs_gif__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);

  wuffs_base__image_buffer ib = ((wuffs_base__image_buffer){});
  {
    wuffs_base__image_config ic = ((wuffs_base__image_config){});
    wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);
    wuffs_base__status status =
        wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
    if (status != WUFFS_BASE__STATUS_OK) {
      FAIL("decode_config: got %" PRIi32 " (%s)", status,
           wuffs_gif__status__string(status));
      return false;
    }
    if (wuffs_base__image_config__pixel_format(&ic) !=
        WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL_INDEXED) {
      FAIL("pixel_format: got 0x%08" PRIX32 ", want 0x%08" PRIX32,
           wuffs_base__image_config__pixel_format(&ic),
           WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL_INDEXED);
      return false;
    }

    // bricks-dither.gif is a 160 × 120, opaque, still (not animated) GIF.
    if (wuffs_base__image_config__width(&ic) != 160) {
      FAIL("width: got %" PRIu32 ", want 160",
           wuffs_base__image_config__width(&ic));
      return false;
    }
    if (wuffs_base__image_config__height(&ic) != 120) {
      FAIL("height: got %" PRIu32 ", want 120",
           wuffs_base__image_config__height(&ic));
      return false;
    }
    if (wuffs_base__image_config__num_loops(&ic) != 1) {
      FAIL("num_loops: got %" PRIu32 ", want 1",
           wuffs_base__image_config__num_loops(&ic));
      return false;
    }
    if (!wuffs_base__image_config__first_frame_is_opaque(&ic)) {
      FAIL("first_frame_is_opaque: got false, want true");
      return false;
    }
    status = wuffs_base__image_buffer__set_from_slice(
        &ib, ic,
        ((wuffs_base__slice_u8){
            .ptr = global_pixel_buffer,
            .len = WUFFS_TESTLIB_ARRAY_SIZE(global_pixel_buffer),
        }));
    if (status) {
      FAIL("set_from_slice: %s", wuffs_gif__status__string(status));
      return false;
    }
  }

  int num_iters = 0;
  while (true) {
    num_iters++;
    wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);
    if (rlimit) {
      set_reader_limit(&src_reader, rlimit);
    }
    size_t old_ri = src.ri;

    wuffs_base__status status = wuffs_gif__decoder__decode_frame(
        &dec, &ib, src_reader, ((wuffs_base__slice_u8){}));

    if (status == WUFFS_BASE__STATUS_OK) {
      const char* msg = copy_to_io_buffer_from_image_buffer(&got, &ib);
      if (msg) {
        FAIL("%s", msg);
        return false;
      }
      break;
    }
    if (status != WUFFS_BASE__SUSPENSION_SHORT_READ) {
      FAIL("decode_frame: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
           wuffs_gif__status__string(status), WUFFS_BASE__SUSPENSION_SHORT_READ,
           wuffs_gif__status__string(WUFFS_BASE__SUSPENSION_SHORT_READ));
      return false;
    }

    if (src.ri < old_ri) {
      FAIL("read index src.ri went backwards");
      return false;
    }
    if (src.ri == old_ri) {
      FAIL("no progress was made");
      return false;
    }
  }

  if (rlimit) {
    if (num_iters <= 1) {
      FAIL("num_iters: got %d, want > 1", num_iters);
      return false;
    }
  } else {
    if (num_iters != 1) {
      FAIL("num_iters: got %d, want 1", num_iters);
      return false;
    }
  }

  wuffs_base__slice_u8 pal_got_slice = wuffs_base__image_buffer__palette(&ib);
  wuffs_base__io_buffer pal_got = {.ptr = pal_got_slice.ptr,
                                   .len = pal_got_slice.len,
                                   .wi = pal_got_slice.len};
  wuffs_base__io_buffer pal_want = {.ptr = global_palette_buffer,
                                    .len = 4 * 256};
  if (!read_file(&pal_want, palette_filename)) {
    return false;
  }
  if (!io_buffers_equal("palette ", &pal_got, &pal_want)) {
    return false;
  }

  wuffs_base__io_buffer ind_want = {.ptr = global_want_buffer,
                                    .len = BUFFER_SIZE};
  if (!read_file(&ind_want, indexes_filename)) {
    return false;
  }
  if (!io_buffers_equal("indexes ", &got, &ind_want)) {
    return false;
  }

  {
    if (src.ri == src.wi) {
      FAIL("decode_frame returned \"ok\" but src was exhausted");
      return false;
    }
    wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);
    wuffs_base__status status = wuffs_gif__decoder__decode_frame(
        &dec, &ib, src_reader, ((wuffs_base__slice_u8){}));
    if (status != WUFFS_BASE__SUSPENSION_END_OF_DATA) {
      FAIL("decode_frame: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
           wuffs_gif__status__string(status),
           WUFFS_BASE__SUSPENSION_END_OF_DATA,
           wuffs_gif__status__string(WUFFS_BASE__SUSPENSION_END_OF_DATA));
      return false;
    }
    if (src.ri != src.wi) {
      FAIL("decode_frame returned \"end of data\" but src was not exhausted");
      return false;
    }
  }

  return true;
}

void test_wuffs_gif_call_sequence() {
  CHECK_FOCUS(__func__);

  wuffs_base__io_buffer src =
      ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});

  if (!read_file(&src, "../../data/bricks-dither.gif")) {
    return;
  }

  wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
  wuffs_gif__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);

  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);

  wuffs_base__status status =
      wuffs_gif__decoder__decode_config(&dec, NULL, src_reader);
  if (status != WUFFS_BASE__STATUS_OK) {
    FAIL("decode_config: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status), WUFFS_BASE__STATUS_OK,
         wuffs_gif__status__string(WUFFS_BASE__STATUS_OK));
    return;
  }

  status = wuffs_gif__decoder__decode_config(&dec, NULL, src_reader);
  if (status != WUFFS_BASE__ERROR_INVALID_CALL_SEQUENCE) {
    FAIL("decode_config: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status),
         WUFFS_BASE__ERROR_INVALID_CALL_SEQUENCE,
         wuffs_gif__status__string(WUFFS_BASE__ERROR_INVALID_CALL_SEQUENCE));
    return;
  }
}

bool do_test_wuffs_gif_decode_animated(
    const char* filename,
    uint32_t want_num_loops,
    uint32_t want_num_frames,
    wuffs_base__rect_ie_u32* want_dirty_rects) {
  wuffs_base__io_buffer src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  if (!read_file(&src, filename)) {
    return false;
  }

  wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
  wuffs_gif__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);

  wuffs_base__image_buffer ib = ((wuffs_base__image_buffer){});
  wuffs_base__image_config ic = ((wuffs_base__image_config){});
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);

  wuffs_base__status status =
      wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
  if (status != WUFFS_BASE__STATUS_OK) {
    FAIL("decode_config: got %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status));
    return false;
  }

  if (wuffs_base__image_config__num_loops(&ic) != want_num_loops) {
    FAIL("num_loops: got %" PRIu32 ", want %" PRIu32,
         wuffs_base__image_config__num_loops(&ic), want_num_loops);
    return false;
  }
  status = wuffs_base__image_buffer__set_from_slice(
      &ib, ic,
      ((wuffs_base__slice_u8){
          .ptr = global_pixel_buffer,
          .len = WUFFS_TESTLIB_ARRAY_SIZE(global_pixel_buffer),
      }));
  if (status) {
    FAIL("set_from_slice: %s", wuffs_gif__status__string(status));
    return false;
  }

  uint32_t i;
  for (i = 0; i < want_num_frames; i++) {
    status = wuffs_gif__decoder__decode_frame(&dec, &ib, src_reader,
                                              ((wuffs_base__slice_u8){}));
    if (status != WUFFS_BASE__STATUS_OK) {
      FAIL("decode_frame #%" PRIu32 ": got %" PRIi32 " (%s)", i, status,
           wuffs_gif__status__string(status));
      return false;
    }

    if (want_dirty_rects) {
      wuffs_base__rect_ie_u32 got = wuffs_base__image_buffer__dirty_rect(&ib);
      wuffs_base__rect_ie_u32 want = want_dirty_rects[i];
      if (!wuffs_base__rect_ie_u32__equals(&got, want)) {
        FAIL("decode_frame #%" PRIu32 ": dirty_rect: got (%" PRIu32 ", %" PRIu32
             ")-(%" PRIu32 ", %" PRIu32 "), want (%" PRIu32 ", %" PRIu32
             ")-(%" PRIu32 ", %" PRIu32 ")",
             i, got.min_incl_x, got.min_incl_y, got.max_excl_x, got.max_excl_y,
             want.min_incl_x, want.min_incl_y, want.max_excl_x,
             want.max_excl_y);
        return false;
      }
    }
  }

  // There should be no more frames.
  status = wuffs_gif__decoder__decode_frame(&dec, &ib, src_reader,
                                            ((wuffs_base__slice_u8){}));
  if (status != WUFFS_BASE__SUSPENSION_END_OF_DATA) {
    FAIL("decode_frame: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status), WUFFS_BASE__SUSPENSION_END_OF_DATA,
         wuffs_gif__status__string(WUFFS_BASE__SUSPENSION_END_OF_DATA));
    return false;
  }

  uint64_t got_num_frames = wuffs_gif__decoder__frame_count(&dec);
  if (got_num_frames != want_num_frames) {
    FAIL("frame_count: got %" PRIu64 ", want %" PRIu32, got_num_frames,
         want_num_frames);
    return false;
  }

  // TODO: test calling wuffs_base__image_buffer__loop.
  return true;
}

void test_wuffs_gif_decode_animated_big() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode_animated("../../data/gifplayer-muybridge.gif", 0,
                                    380, NULL);
}

void test_wuffs_gif_decode_animated_medium() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode_animated("../../data/muybridge.gif", 0, 15, NULL);
}

void test_wuffs_gif_decode_animated_small() {
  CHECK_FOCUS(__func__);
  // animated-red-blue.gif's num_loops should be 3. The value explicitly in the
  // wire format is 0x0002, but that value means "repeat 2 times after the
  // first play", so the total number of loops is 3.
  const uint32_t want_num_loops = 3;
  wuffs_base__rect_ie_u32 want_rects[4] = {
      make_rect_ie_u32(0, 0, 64, 48),
      make_rect_ie_u32(15, 31, 52, 40),
      make_rect_ie_u32(15, 0, 64, 40),
      make_rect_ie_u32(15, 0, 64, 40),
  };
  do_test_wuffs_gif_decode_animated(
      "../../data/animated-red-blue.gif", want_num_loops,
      WUFFS_TESTLIB_ARRAY_SIZE(want_rects), want_rects);
}

void test_wuffs_gif_decode_frame_out_of_bounds() {
  CHECK_FOCUS(__func__);
  wuffs_base__io_buffer src =
      ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});
  if (!read_file(&src, "../../data/artificial/gif-frame-out-of-bounds.gif")) {
    return;
  }

  wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
  wuffs_gif__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);
  wuffs_base__image_config ic = ((wuffs_base__image_config){});
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);
  wuffs_base__status s =
      wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
  if (s) {
    FAIL("decode_config: %s", wuffs_gif__status__string(s));
    return;
  }

  // The nominal width and height for the overall image is 2×2, but its first
  // frame extends those bounds to 4×2. See
  // test/data/artificial/gif-frame-out-of-bounds.gif.make-artificial.txt for
  // more discussion.

  if (wuffs_base__image_config__width(&ic) != 4) {
    FAIL("width: got %" PRIu32 ", want 4",
         wuffs_base__image_config__width(&ic));
    return;
  }
  if (wuffs_base__image_config__height(&ic) != 2) {
    FAIL("height: got %" PRIu32 ", want 2",
         wuffs_base__image_config__height(&ic));
    return;
  }
}

void test_wuffs_gif_decode_input_is_a_gif() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../data/bricks-dither.gif",
                           "../../data/bricks-dither.palette",
                           "../../data/bricks-dither.indexes", 0);
}

void test_wuffs_gif_decode_input_is_a_gif_many_big_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../data/bricks-dither.gif",
                           "../../data/bricks-dither.palette",
                           "../../data/bricks-dither.indexes", 4096);
}

void test_wuffs_gif_decode_input_is_a_gif_many_medium_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../data/bricks-dither.gif",
                           "../../data/bricks-dither.palette",
                           "../../data/bricks-dither.indexes", 787);
  // The magic 787 tickles being in the middle of a decode_extension skip
  // call.
  //
  // TODO: has 787 changed since we decode the image_config separately?
}

void test_wuffs_gif_decode_input_is_a_gif_many_small_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../data/bricks-dither.gif",
                           "../../data/bricks-dither.palette",
                           "../../data/bricks-dither.indexes", 13);
}

void test_wuffs_gif_decode_input_is_a_png() {
  CHECK_FOCUS(__func__);

  wuffs_base__io_buffer src =
      ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});

  if (!read_file(&src, "../../data/bricks-dither.png")) {
    return;
  }

  wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
  wuffs_gif__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);
  wuffs_base__image_config ic = ((wuffs_base__image_config){});
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);

  wuffs_base__status status =
      wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
  if (status != WUFFS_GIF__ERROR_BAD_HEADER) {
    FAIL("decode_config: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status), WUFFS_GIF__ERROR_BAD_HEADER,
         wuffs_gif__status__string(WUFFS_GIF__ERROR_BAD_HEADER));
    return;
  }
}

  // ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

bool do_test_mimic_gif_decode(const char* filename) {
  wuffs_base__io_buffer src =
      ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});
  if (!read_file(&src, filename)) {
    return false;
  }

  src.ri = 0;
  wuffs_base__io_buffer got =
      ((wuffs_base__io_buffer){.ptr = global_got_buffer, .len = BUFFER_SIZE});
  const char* got_msg = wuffs_gif_decode(&got, &src);
  if (got_msg) {
    FAIL("%s", got_msg);
    return false;
  }

  src.ri = 0;
  wuffs_base__io_buffer want =
      ((wuffs_base__io_buffer){.ptr = global_want_buffer, .len = BUFFER_SIZE});
  const char* want_msg = mimic_gif_decode(&want, &src);
  if (want_msg) {
    FAIL("%s", want_msg);
    return false;
  }

  if (!io_buffers_equal("", &got, &want)) {
    return false;
  }

  // TODO: check the palette RGB values, not just the palette indexes (pixels).

  return true;
}

void test_mimic_gif_decode_animated_small() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/animated-red-blue.gif");
}

void test_mimic_gif_decode_bricks_dither() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/bricks-dither.gif");
}

void test_mimic_gif_decode_bricks_gray() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/bricks-gray.gif");
}

void test_mimic_gif_decode_bricks_nodither() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/bricks-nodither.gif");
}

void test_mimic_gif_decode_gifplayer_muybridge() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/gifplayer-muybridge.gif");
}

void test_mimic_gif_decode_harvesters() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/harvesters.gif");
}

void test_mimic_gif_decode_hat() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/hat.gif");
}

void test_mimic_gif_decode_hibiscus() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/hibiscus.gif");
}

void test_mimic_gif_decode_hippopotamus_interlaced() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/hippopotamus.interlaced.gif");
}

void test_mimic_gif_decode_hippopotamus_regular() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/hippopotamus.regular.gif");
}

void test_mimic_gif_decode_muybridge() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/muybridge.gif");
}

void test_mimic_gif_decode_pjw_thumbnail() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/pjw-thumbnail.gif");
}

#endif  // WUFFS_MIMIC

// ---------------- GIF Benches

bool do_bench_gif_decode(const char* (*decode_func)(wuffs_base__io_buffer*,
                                                    wuffs_base__io_buffer*),
                         const char* filename,
                         uint64_t iters_unscaled) {
  wuffs_base__io_buffer dst =
      ((wuffs_base__io_buffer){.ptr = global_got_buffer, .len = BUFFER_SIZE});
  wuffs_base__io_buffer src =
      ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});

  if (!read_file(&src, filename)) {
    return false;
  }

  bench_start();
  uint64_t n_bytes = 0;
  uint64_t i;
  uint64_t iters = iters_unscaled * iterscale;
  for (i = 0; i < iters; i++) {
    dst.wi = 0;
    src.ri = 0;
    const char* error_msg = decode_func(&dst, &src);
    if (error_msg) {
      FAIL("%s", error_msg);
      return false;
    }
    n_bytes += dst.wi;
  }
  bench_finish(iters, n_bytes);
  return true;
}

void bench_wuffs_gif_decode_1k_bw() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../data/pjw-thumbnail.gif", 2000);
}

void bench_wuffs_gif_decode_1k_color() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../data/hippopotamus.regular.gif",
                      1000);
}

void bench_wuffs_gif_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../data/hat.gif", 100);
}

void bench_wuffs_gif_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../data/hibiscus.gif", 10);
}

void bench_wuffs_gif_decode_1000k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../data/harvesters.gif", 1);
}

  // ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

void bench_mimic_gif_decode_1k_bw() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../data/pjw-thumbnail.gif", 2000);
}

void bench_mimic_gif_decode_1k_color() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../data/hippopotamus.regular.gif",
                      1000);
}

void bench_mimic_gif_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../data/hat.gif", 100);
}

void bench_mimic_gif_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../data/hibiscus.gif", 10);
}

void bench_mimic_gif_decode_1000k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../data/harvesters.gif", 1);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

    // These basic tests are really testing the Wuffs compiler. They aren't
    // specific to the std/gif code, but putting them here is as good as any
    // other place.
    test_basic_bad_receiver,                    //
    test_basic_bad_sizeof_receiver,             //
    test_basic_bad_wuffs_version,               //
    test_basic_check_wuffs_version_not_called,  //
    test_basic_status_is_error,                 //
    test_basic_status_strings,                  //
    test_basic_status_used_package,             //
    test_basic_sub_struct_initializer,          //

    test_wuffs_gif_call_sequence,                            //
    test_wuffs_gif_decode_animated_big,                      //
    test_wuffs_gif_decode_animated_medium,                   //
    test_wuffs_gif_decode_animated_small,                    //
    test_wuffs_gif_decode_frame_out_of_bounds,               //
    test_wuffs_gif_decode_input_is_a_gif,                    //
    test_wuffs_gif_decode_input_is_a_gif_many_big_reads,     //
    test_wuffs_gif_decode_input_is_a_gif_many_medium_reads,  //
    test_wuffs_gif_decode_input_is_a_gif_many_small_reads,   //
    test_wuffs_gif_decode_input_is_a_png,                    //

#ifdef WUFFS_MIMIC

    test_mimic_gif_decode_animated_small,           //
    test_mimic_gif_decode_bricks_dither,            //
    test_mimic_gif_decode_bricks_gray,              //
    test_mimic_gif_decode_bricks_nodither,          //
    test_mimic_gif_decode_gifplayer_muybridge,      //
    test_mimic_gif_decode_harvesters,               //
    test_mimic_gif_decode_hat,                      //
    test_mimic_gif_decode_hibiscus,                 //
    test_mimic_gif_decode_hippopotamus_interlaced,  //
    test_mimic_gif_decode_hippopotamus_regular,     //
    test_mimic_gif_decode_muybridge,                //
    test_mimic_gif_decode_pjw_thumbnail,            //

#endif  // WUFFS_MIMIC

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

    bench_wuffs_gif_decode_1k_bw,     //
    bench_wuffs_gif_decode_1k_color,  //
    bench_wuffs_gif_decode_10k,       //
    bench_wuffs_gif_decode_100k,      //
    bench_wuffs_gif_decode_1000k,     //

#ifdef WUFFS_MIMIC

    bench_mimic_gif_decode_1k_bw,     //
    bench_mimic_gif_decode_1k_color,  //
    bench_mimic_gif_decode_10k,       //
    bench_mimic_gif_decode_100k,      //
    bench_mimic_gif_decode_1000k,     //

#endif  // WUFFS_MIMIC

    NULL,
};

int main(int argc, char** argv) {
  proc_package_name = "std/gif";
  return test_main(argc, argv, tests, benches);
}
