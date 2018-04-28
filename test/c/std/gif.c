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

for cc in clang gcc; do
  $cc -std=c99 -Wall -Werror gif.c && ./a.out
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

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../gen/c/std/gif.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/gif.c"
#endif

// ---------------- Basic Tests

void test_basic_bad_receiver() {
  CHECK_FOCUS(__func__);
  wuffs_base__image_config ic = {{0}};
  wuffs_base__io_reader src = {{0}};
  wuffs_gif__status status = wuffs_gif__decoder__decode_config(NULL, &ic, src);
  if (status != WUFFS_GIF__ERROR_BAD_RECEIVER) {
    FAIL("decode_config: got %d, want %d", status,
         WUFFS_GIF__ERROR_BAD_RECEIVER);
  }
}

void test_basic_initializer_not_called() {
  CHECK_FOCUS(__func__);
  wuffs_gif__decoder dec = {{0}};
  wuffs_base__image_config ic = {{0}};
  wuffs_base__io_reader src = {{0}};
  wuffs_gif__status status = wuffs_gif__decoder__decode_config(&dec, &ic, src);
  if (status != WUFFS_GIF__ERROR_INITIALIZER_NOT_CALLED) {
    FAIL("decode_config: got %d, want %d", status,
         WUFFS_GIF__ERROR_INITIALIZER_NOT_CALLED);
  }
}

void test_basic_wuffs_version_bad() {
  CHECK_FOCUS(__func__);
  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, 0, 0);  // 0 is not WUFFS_VERSION.
  if (dec.private_impl.status != WUFFS_GIF__ERROR_BAD_WUFFS_VERSION) {
    FAIL("decode_config: got %d, want %d", dec.private_impl.status,
         WUFFS_GIF__ERROR_BAD_WUFFS_VERSION);
    return;
  }
}

void test_basic_wuffs_version_good() {
  CHECK_FOCUS(__func__);
  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);
  if (dec.private_impl.magic != WUFFS_BASE__MAGIC) {
    FAIL("magic: got %u, want %u", dec.private_impl.magic, WUFFS_BASE__MAGIC);
    return;
  }
  if (dec.private_impl.f_lzw.private_impl.f_literal_width != 8) {
    FAIL("f_literal_width: got %u, want %u",
         dec.private_impl.f_lzw.private_impl.f_literal_width, 8);
    return;
  }
}

void test_basic_status_is_error() {
  CHECK_FOCUS(__func__);
  if (wuffs_gif__status__is_error(WUFFS_GIF__STATUS_OK)) {
    FAIL("is_error(OK) returned true");
    return;
  }
  if (!wuffs_gif__status__is_error(WUFFS_GIF__ERROR_BAD_WUFFS_VERSION)) {
    FAIL("is_error(BAD_WUFFS_VERSION) returned false");
    return;
  }
  if (wuffs_gif__status__is_error(WUFFS_GIF__SUSPENSION_SHORT_WRITE)) {
    FAIL("is_error(SHORT_WRITE) returned true");
    return;
  }
  if (!wuffs_gif__status__is_error(WUFFS_GIF__ERROR_LZW_CODE_IS_OUT_OF_RANGE)) {
    FAIL("is_error(LZW_CODE_IS_OUT_OF_RANGE) returned false");
    return;
  }
}

void test_basic_status_strings() {
  CHECK_FOCUS(__func__);
  const char* s0 = wuffs_gif__status__string(WUFFS_GIF__STATUS_OK);
  const char* t0 = "ok";
  if (strcmp(s0, t0)) {
    FAIL("got \"%s\", want \"%s\"", s0, t0);
    return;
  }
  const char* s1 =
      wuffs_gif__status__string(WUFFS_GIF__ERROR_BAD_WUFFS_VERSION);
  const char* t1 = "bad wuffs version";
  if (strcmp(s1, t1)) {
    FAIL("got \"%s\", want \"%s\"", s1, t1);
    return;
  }
  const char* s2 = wuffs_gif__status__string(WUFFS_GIF__SUSPENSION_SHORT_WRITE);
  const char* t2 = "short write";
  if (strcmp(s2, t2)) {
    FAIL("got \"%s\", want \"%s\"", s2, t2);
    return;
  }
  const char* s3 =
      wuffs_gif__status__string(WUFFS_GIF__ERROR_LZW_CODE_IS_OUT_OF_RANGE);
  const char* t3 = "gif: LZW code is out of range";
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

void test_basic_sub_struct_initializer() {
  CHECK_FOCUS(__func__);
  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);
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

// ---------------- LZW Tests

bool do_test_wuffs_lzw_decode(const char* src_filename,
                              uint64_t src_size,
                              const char* want_filename,
                              uint64_t want_size,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  wuffs_base__io_buffer got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__io_buffer want = {.ptr = global_want_buffer, .len = BUFFER_SIZE};
  wuffs_base__io_buffer src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  if (!read_file(&src, src_filename)) {
    return false;
  }
  if (src.wi != src_size) {
    FAIL("src size: got %d, want %d", (int)(src.wi), (int)(src_size));
    return false;
  }
  // The first byte in that file, the LZW literal width, should be 0x08.
  uint8_t literal_width = src.ptr[0];
  if (literal_width != 0x08) {
    FAIL("LZW literal width: got %d, want %d", (int)(src.ptr[0]), 0x08);
    return false;
  }
  src.ri++;

  if (!read_file(&want, want_filename)) {
    return false;
  }
  if (want.wi != want_size) {
    FAIL("want size: got %d, want %d", (int)(want.wi), (int)(want_size));
    return false;
  }

  wuffs_gif__lzw_decoder dec;
  wuffs_gif__lzw_decoder__initialize(&dec, WUFFS_VERSION, 0);
  wuffs_gif__lzw_decoder__set_literal_width(&dec, literal_width);
  int num_iters = 0;
  uint64_t wlim = 0;
  uint64_t rlim = 0;
  while (true) {
    num_iters++;
    wuffs_base__io_writer got_writer = wuffs_base__io_buffer__writer(&got);
    if (wlimit) {
      wuffs_base__io_writer__set_limit(&got_writer, wlimit);
      wlim = wlimit;
      got_writer.private_impl.limit.ptr_to_len = &wlim;
    }
    wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);
    if (rlimit) {
      wuffs_base__io_reader__set_limit(&src_reader, rlimit);
      rlim = rlimit;
      src_reader.private_impl.limit.ptr_to_len = &rlim;
    }
    size_t old_wi = got.wi;
    size_t old_ri = src.ri;

    wuffs_gif__status status =
        wuffs_gif__lzw_decoder__decode(&dec, got_writer, src_reader);
    if (status == WUFFS_GIF__STATUS_OK) {
      if (src.ri != src.wi) {
        FAIL("decode returned \"ok\" but src was not exhausted");
        return false;
      }
      break;
    }
    if ((status != WUFFS_GIF__SUSPENSION_SHORT_READ) &&
        (status != WUFFS_GIF__SUSPENSION_SHORT_WRITE)) {
      FAIL("decode: got %" PRIi32 " (%s), want %" PRIi32 " (%s) or %" PRIi32
           " (%s)",
           status, wuffs_gif__status__string(status),
           WUFFS_GIF__SUSPENSION_SHORT_READ,
           wuffs_gif__status__string(WUFFS_GIF__SUSPENSION_SHORT_READ),
           WUFFS_GIF__SUSPENSION_SHORT_WRITE,
           wuffs_gif__status__string(WUFFS_GIF__SUSPENSION_SHORT_WRITE));
      return false;
    }

    if (got.wi < old_wi) {
      FAIL("write index got.wi went backwards");
      return false;
    }
    if (src.ri < old_ri) {
      FAIL("read index src.ri went backwards");
      return false;
    }
    if ((got.wi == old_wi) && (src.ri == old_ri)) {
      FAIL("no progress was made");
      return false;
    }
  }

  if (wlimit || rlimit) {
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

  return io_buffers_equal("", &got, &want);
}

void test_wuffs_lzw_decode_many_big_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_lzw_decode("../../data/bricks-gray.indexes.giflzw", 14731,
                           "../../data/bricks-gray.indexes", 19200, 0, 4096);
}

void test_wuffs_lzw_decode_many_small_writes_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_lzw_decode("../../data/bricks-gray.indexes.giflzw", 14731,
                           "../../data/bricks-gray.indexes", 19200, 41, 43);
}

void test_wuffs_lzw_decode_bricks_dither() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_lzw_decode("../../data/bricks-dither.indexes.giflzw", 14923,
                           "../../data/bricks-dither.indexes", 19200, 0, 0);
}

void test_wuffs_lzw_decode_bricks_nodither() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_lzw_decode("../../data/bricks-nodither.indexes.giflzw", 13382,
                           "../../data/bricks-nodither.indexes", 19200, 0, 0);
}

void test_wuffs_lzw_decode_pi() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_lzw_decode("../../data/pi.txt.giflzw", 50550,
                           "../../data/pi.txt", 100003, 0, 0);
}

// ---------------- LZW Benches

bool do_bench_wuffs_lzw_decode(const char* filename, uint64_t reps) {
  wuffs_base__io_buffer dst = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__io_buffer src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};
  wuffs_base__io_writer dst_writer = wuffs_base__io_buffer__writer(&dst);
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);

  if (!read_file(&src, filename)) {
    return false;
  }
  if (src.wi <= 0) {
    FAIL("src size: got %d, want > 0", (int)(src.wi));
    return false;
  }
  uint8_t literal_width = src.ptr[0];
  if (literal_width != 0x08) {
    FAIL("LZW literal width: got %d, want %d", (int)(src.ptr[0]), 0x08);
    return false;
  }

  bench_start();
  uint64_t n_bytes = 0;
  uint64_t i;
  for (i = 0; i < reps; i++) {
    dst.wi = 0;
    src.ri = 1;  // Skip the literal width.
    wuffs_gif__lzw_decoder dec;
    wuffs_gif__lzw_decoder__initialize(&dec, WUFFS_VERSION, 0);
    wuffs_gif__status s =
        wuffs_gif__lzw_decoder__decode(&dec, dst_writer, src_reader);
    if (s) {
      FAIL("decode: %" PRIi32 " (%s)", s, wuffs_gif__status__string(s));
      return false;
    }
    n_bytes += dst.wi;
  }
  bench_finish(reps, n_bytes);
  return true;
}

void bench_wuffs_lzw_decode_20k() {
  CHECK_FOCUS(__func__);
  do_bench_wuffs_lzw_decode("../../data/bricks-gray.indexes.giflzw", 5000);
}

void bench_wuffs_lzw_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_wuffs_lzw_decode("../../data/pi.txt.giflzw", 1000);
}

// ---------------- GIF Tests

const char* wuffs_gif_decode(wuffs_base__io_buffer* dst,
                             wuffs_base__io_buffer* src) {
  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);
  wuffs_base__image_config ic = {{0}};
  wuffs_base__io_writer dst_writer = wuffs_base__io_buffer__writer(dst);
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(src);
  wuffs_gif__status s =
      wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
  if (s) {
    return wuffs_gif__status__string(s);
  }
  s = wuffs_gif__decoder__decode_frame(&dec, dst_writer, src_reader);
  if (s) {
    return wuffs_gif__status__string(s);
  }
  return NULL;
}

bool do_test_wuffs_gif_decode(const char* filename,
                              const char* palette_filename,
                              const char* indexes_filename,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  wuffs_base__io_buffer got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__io_buffer src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  if (!read_file(&src, filename)) {
    return false;
  }

  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);

  {
    wuffs_base__image_config ic = {{0}};
    wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);
    wuffs_gif__status status =
        wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
    if (status != WUFFS_GIF__STATUS_OK) {
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

    // bricks-dither.gif is a 160 × 120 still (not animated) GIF.
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
  }

  int num_iters = 0;
  uint64_t wlim = 0;
  uint64_t rlim = 0;
  while (true) {
    num_iters++;
    wuffs_base__io_writer got_writer = wuffs_base__io_buffer__writer(&got);
    if (wlimit) {
      wuffs_base__io_writer__set_limit(&got_writer, wlimit);
      wlim = wlimit;
      got_writer.private_impl.limit.ptr_to_len = &wlim;
    }
    wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);
    if (rlimit) {
      wuffs_base__io_reader__set_limit(&src_reader, rlimit);
      rlim = rlimit;
      src_reader.private_impl.limit.ptr_to_len = &rlim;
    }
    size_t old_wi = got.wi;
    size_t old_ri = src.ri;

    wuffs_gif__status status =
        wuffs_gif__decoder__decode_frame(&dec, got_writer, src_reader);
    if (status == WUFFS_GIF__STATUS_OK) {
      break;
    }
    if ((status != WUFFS_GIF__SUSPENSION_SHORT_READ) &&
        (status != WUFFS_GIF__SUSPENSION_SHORT_WRITE)) {
      FAIL("decode_frame: got %" PRIi32 " (%s), want %" PRIi32
           " (%s) or %" PRIi32 " (%s)",
           status, wuffs_gif__status__string(status),
           WUFFS_GIF__SUSPENSION_SHORT_READ,
           wuffs_gif__status__string(WUFFS_GIF__SUSPENSION_SHORT_READ),
           WUFFS_GIF__SUSPENSION_SHORT_WRITE,
           wuffs_gif__status__string(WUFFS_GIF__SUSPENSION_SHORT_WRITE));
      return false;
    }

    if (got.wi < old_wi) {
      FAIL("write index got.wi went backwards");
      return false;
    }
    if (src.ri < old_ri) {
      FAIL("read index src.ri went backwards");
      return false;
    }
    if ((got.wi == old_wi) && (src.ri == old_ri)) {
      FAIL("no progress was made");
      return false;
    }
  }

  if (wlimit || rlimit) {
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

  // TODO: provide a public API for getting the palette.
  wuffs_base__io_buffer pal_got = {.ptr = dec.private_impl.f_gct,
                                   .len = 3 * 256};
  wuffs_base__io_buffer pal_want = {.ptr = global_palette_buffer,
                                    .len = 3 * 256};
  pal_got.wi = 3 * 256;
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
    wuffs_base__io_writer got_writer = wuffs_base__io_buffer__writer(&got);
    wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);
    wuffs_gif__status status =
        wuffs_gif__decoder__decode_frame(&dec, got_writer, src_reader);
    if (status != WUFFS_GIF__SUSPENSION_END_OF_DATA) {
      FAIL("decode_frame: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
           wuffs_gif__status__string(status), WUFFS_GIF__SUSPENSION_END_OF_DATA,
           wuffs_gif__status__string(WUFFS_GIF__SUSPENSION_END_OF_DATA));
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

  wuffs_base__io_buffer got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__io_buffer src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  if (!read_file(&src, "../../data/bricks-dither.gif")) {
    return;
  }

  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);

  wuffs_base__io_writer got_writer = wuffs_base__io_buffer__writer(&got);
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);

  wuffs_gif__status status =
      wuffs_gif__decoder__decode_frame(&dec, got_writer, src_reader);
  if (status != WUFFS_GIF__ERROR_INVALID_CALL_SEQUENCE) {
    FAIL("decode_frame: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status),
         WUFFS_GIF__ERROR_INVALID_CALL_SEQUENCE,
         wuffs_gif__status__string(WUFFS_GIF__ERROR_INVALID_CALL_SEQUENCE));
    return;
  }
}

void test_wuffs_gif_decode_animated() {
  CHECK_FOCUS(__func__);

  wuffs_base__io_buffer got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__io_buffer src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  if (!read_file(&src, "../../data/animated-red-blue.gif")) {
    return;
  }

  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);

  wuffs_base__image_config ic = {{0}};
  wuffs_base__io_writer got_writer = wuffs_base__io_buffer__writer(&got);
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);

  wuffs_gif__status status =
      wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
  if (status != WUFFS_GIF__STATUS_OK) {
    FAIL("decode_config: got %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status));
    return;
  }

  // animated-red-blue.gif's num_loops should be 3. The value explicitly in the
  // wire format is 0x0002, but that value means "repeat 2 times after the
  // first play", so the total number of loops is 3.
  if (wuffs_base__image_config__num_loops(&ic) != 3) {
    FAIL("num_loops: got %" PRIu32 ", want 3",
         wuffs_base__image_config__num_loops(&ic));
    return;
  }

  // animated-red-blue.gif should have 4 frames.
  int i;
  for (i = 0; i < 4; i++) {
    got.wi = 0;
    status = wuffs_gif__decoder__decode_frame(&dec, got_writer, src_reader);
    if (status != WUFFS_GIF__STATUS_OK) {
      FAIL("decode_frame #%d: got %" PRIi32 " (%s)", i, status,
           wuffs_gif__status__string(status));
      return;
    }
    // TODO: check that the frame top/left/width/height is:
    //  -  0, 0,64,48 for frame #0
    //  - 15,31,37, 9 for frame #1
    //  - 15, 0,49,40 for frame #2
    //  - 15, 0,49,40 for frame #3
  }

  // There should be no more frames.
  got.wi = 0;
  status = wuffs_gif__decoder__decode_frame(&dec, got_writer, src_reader);
  if (status != WUFFS_GIF__SUSPENSION_END_OF_DATA) {
    FAIL("decode_frame: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status), WUFFS_GIF__SUSPENSION_END_OF_DATA,
         wuffs_gif__status__string(WUFFS_GIF__SUSPENSION_END_OF_DATA));
    return;
  }

  // TODO: test calling wuffs_base__image_buffer__loop.
}

void test_wuffs_gif_decode_input_is_a_gif() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../data/bricks-dither.gif",
                           "../../data/bricks-dither.palette",
                           "../../data/bricks-dither.indexes", 0, 0);
}

void test_wuffs_gif_decode_input_is_a_gif_many_big_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../data/bricks-dither.gif",
                           "../../data/bricks-dither.palette",
                           "../../data/bricks-dither.indexes", 0, 4096);
}

void test_wuffs_gif_decode_input_is_a_gif_many_medium_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../data/bricks-dither.gif",
                           "../../data/bricks-dither.palette",
                           "../../data/bricks-dither.indexes", 0, 787);
  // The magic 787 tickles being in the middle of a decode_extension skip32
  // call.
  //
  // TODO: has 787 changed since we decode the image_config separately?
}

void test_wuffs_gif_decode_input_is_a_gif_many_small_writes_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../data/bricks-dither.gif",
                           "../../data/bricks-dither.palette",
                           "../../data/bricks-dither.indexes", 11, 13);
}

void test_wuffs_gif_decode_input_is_a_png() {
  CHECK_FOCUS(__func__);

  wuffs_base__io_buffer src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  if (!read_file(&src, "../../data/bricks-dither.png")) {
    return;
  }

  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);
  wuffs_base__image_config ic = {{0}};
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);

  wuffs_gif__status status =
      wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
  if (status != WUFFS_GIF__ERROR_BAD_GIF_HEADER) {
    FAIL("decode_config: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status), WUFFS_GIF__ERROR_BAD_GIF_HEADER,
         wuffs_gif__status__string(WUFFS_GIF__ERROR_BAD_GIF_HEADER));
    return;
  }
}

  // ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

bool do_test_mimic_gif_decode(const char* filename) {
  wuffs_base__io_buffer src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};
  if (!read_file(&src, filename)) {
    return false;
  }

  src.ri = 0;
  wuffs_base__io_buffer got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  const char* got_msg = wuffs_gif_decode(&got, &src);
  if (got_msg) {
    FAIL("%s", got_msg);
    return false;
  }

  src.ri = 0;
  wuffs_base__io_buffer want = {.ptr = global_want_buffer, .len = BUFFER_SIZE};
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

void test_mimic_gif_decode_pjw_thumbnail() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../data/pjw-thumbnail.gif");
}

#endif  // WUFFS_MIMIC

// ---------------- GIF Benches

bool do_bench_gif_decode(const char* (*decode_func)(wuffs_base__io_buffer*,
                                                    wuffs_base__io_buffer*),
                         const char* filename,
                         uint64_t reps) {
  wuffs_base__io_buffer dst = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__io_buffer src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  if (!read_file(&src, filename)) {
    return false;
  }

  bench_start();
  uint64_t n_bytes = 0;
  uint64_t i;
  for (i = 0; i < reps; i++) {
    dst.wi = 0;
    src.ri = 0;
    const char* error_msg = decode_func(&dst, &src);
    if (error_msg) {
      FAIL("%s", error_msg);
      return false;
    }
    n_bytes += dst.wi;
  }
  bench_finish(reps, n_bytes);
  return true;
}

void bench_wuffs_gif_decode_1k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../data/pjw-thumbnail.gif", 200000);
}

void bench_wuffs_gif_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../data/hat.gif", 10000);
}

void bench_wuffs_gif_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../data/hibiscus.gif", 1000);
}

void bench_wuffs_gif_decode_1000k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../data/harvesters.gif", 100);
}

  // ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

void bench_mimic_gif_decode_1k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../data/pjw-thumbnail.gif", 200000);
}

void bench_mimic_gif_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../data/hat.gif", 10000);
}

void bench_mimic_gif_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../data/hibiscus.gif", 1000);
}

void bench_mimic_gif_decode_1000k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../data/harvesters.gif", 100);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

    // These basic tests are really testing the Wuffs compiler. They aren't
    // specific to the std/gif code, but putting them here is as good as any
    // other place.
    test_basic_bad_receiver,            //
    test_basic_initializer_not_called,  //
    test_basic_wuffs_version_bad,       //
    test_basic_wuffs_version_good,      //
    test_basic_status_is_error,         //
    test_basic_status_strings,          //
    test_basic_sub_struct_initializer,  //

    test_wuffs_lzw_decode_many_big_reads,           //
    test_wuffs_lzw_decode_many_small_writes_reads,  //
    test_wuffs_lzw_decode_bricks_dither,            //
    test_wuffs_lzw_decode_bricks_nodither,          //
    test_wuffs_lzw_decode_pi,                       //

    test_wuffs_gif_call_sequence,                                  //
    test_wuffs_gif_decode_animated,                                //
    test_wuffs_gif_decode_input_is_a_gif,                          //
    test_wuffs_gif_decode_input_is_a_gif_many_big_reads,           //
    test_wuffs_gif_decode_input_is_a_gif_many_medium_reads,        //
    test_wuffs_gif_decode_input_is_a_gif_many_small_writes_reads,  //
    test_wuffs_gif_decode_input_is_a_png,                          //

#ifdef WUFFS_MIMIC

    test_mimic_gif_decode_bricks_dither,    //
    test_mimic_gif_decode_bricks_gray,      //
    test_mimic_gif_decode_bricks_nodither,  //
    test_mimic_gif_decode_harvesters,       //
    test_mimic_gif_decode_hat,              //
    test_mimic_gif_decode_hibiscus,         //
    test_mimic_gif_decode_pjw_thumbnail,    //

#endif  // WUFFS_MIMIC

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

    bench_wuffs_lzw_decode_20k,   //
    bench_wuffs_lzw_decode_100k,  //

    bench_wuffs_gif_decode_1k,     //
    bench_wuffs_gif_decode_10k,    //
    bench_wuffs_gif_decode_100k,   //
    bench_wuffs_gif_decode_1000k,  //

#ifdef WUFFS_MIMIC

    bench_mimic_gif_decode_1k,     //
    bench_mimic_gif_decode_10k,    //
    bench_mimic_gif_decode_100k,   //
    bench_mimic_gif_decode_1000k,  //

#endif  // WUFFS_MIMIC

    NULL,
};

int main(int argc, char** argv) {
  proc_filename = "std/gif.c";
  return test_main(argc, argv, tests, benches);
}
