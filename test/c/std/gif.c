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

void test_basic_bad_argument_out_of_range() {
  CHECK_FOCUS(__func__);
  wuffs_gif__lzw_decoder dec;
  wuffs_gif__lzw_decoder__initialize(&dec, WUFFS_VERSION, 0);

  // Setting to 8 is in the [2..8] range.
  wuffs_gif__lzw_decoder__set_literal_width(&dec, 8);
  if (dec.private_impl.status != WUFFS_GIF__STATUS_OK) {
    FAIL("status: got %d, want %d", dec.private_impl.status,
         WUFFS_GIF__STATUS_OK);
  }

  // Setting to 999 is out of range.
  wuffs_gif__lzw_decoder__set_literal_width(&dec, 999);
  if (dec.private_impl.status != WUFFS_GIF__ERROR_BAD_ARGUMENT) {
    FAIL("status: got %d, want %d", dec.private_impl.status,
         WUFFS_GIF__ERROR_BAD_ARGUMENT);
  }

  // That error status code should be sticky.
  wuffs_gif__lzw_decoder__set_literal_width(&dec, 8);
  if (dec.private_impl.status != WUFFS_GIF__ERROR_BAD_ARGUMENT) {
    FAIL("status: got %d, want %d", dec.private_impl.status,
         WUFFS_GIF__ERROR_BAD_ARGUMENT);
  }
}

void test_basic_bad_receiver() {
  CHECK_FOCUS(__func__);
  wuffs_base__writer1 dst = {0};
  wuffs_base__reader1 src = {0};
  wuffs_gif__status status = wuffs_gif__lzw_decoder__decode(NULL, dst, src);
  if (status != WUFFS_GIF__ERROR_BAD_RECEIVER) {
    FAIL("status: got %d, want %d", status, WUFFS_GIF__ERROR_BAD_RECEIVER);
  }
}

void test_basic_initializer_not_called() {
  CHECK_FOCUS(__func__);
  wuffs_gif__lzw_decoder dec = {{0}};
  wuffs_base__writer1 dst = {0};
  wuffs_base__reader1 src = {0};
  wuffs_gif__status status = wuffs_gif__lzw_decoder__decode(&dec, dst, src);
  if (status != WUFFS_GIF__ERROR_INITIALIZER_NOT_CALLED) {
    FAIL("status: got %d, want %d", status,
         WUFFS_GIF__ERROR_INITIALIZER_NOT_CALLED);
  }
}

void test_basic_wuffs_version_bad() {
  CHECK_FOCUS(__func__);
  wuffs_gif__lzw_decoder dec;
  wuffs_gif__lzw_decoder__initialize(&dec, 0, 0);  // 0 is not WUFFS_VERSION.
  if (dec.private_impl.status != WUFFS_GIF__ERROR_BAD_WUFFS_VERSION) {
    FAIL("status: got %d, want %d", dec.private_impl.status,
         WUFFS_GIF__ERROR_BAD_WUFFS_VERSION);
    return;
  }
}

void test_basic_wuffs_version_good() {
  CHECK_FOCUS(__func__);
  wuffs_gif__lzw_decoder dec;
  wuffs_gif__lzw_decoder__initialize(&dec, WUFFS_VERSION, 0);
  if (dec.private_impl.magic != WUFFS_BASE__MAGIC) {
    FAIL("magic: got %u, want %u", dec.private_impl.magic, WUFFS_BASE__MAGIC);
    return;
  }
  if (dec.private_impl.f_literal_width != 8) {
    FAIL("f_literal_width: got %u, want %u", dec.private_impl.f_literal_width,
         8);
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
  const char* t0 = "gif: ok";
  if (strcmp(s0, t0)) {
    FAIL("got \"%s\", want \"%s\"", s0, t0);
    return;
  }
  const char* s1 =
      wuffs_gif__status__string(WUFFS_GIF__ERROR_BAD_WUFFS_VERSION);
  const char* t1 = "gif: bad wuffs version";
  if (strcmp(s1, t1)) {
    FAIL("got \"%s\", want \"%s\"", s1, t1);
    return;
  }
  const char* s2 = wuffs_gif__status__string(WUFFS_GIF__SUSPENSION_SHORT_WRITE);
  const char* t2 = "gif: short write";
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
  const char* t4 = "gif: unknown status";
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

bool do_test_wuffs_gif_lzw_decode(const char* src_filename,
                                  uint64_t src_size,
                                  const char* want_filename,
                                  uint64_t want_size,
                                  uint64_t wlimit,
                                  uint64_t rlimit) {
  wuffs_base__buf1 got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__buf1 want = {.ptr = global_want_buffer, .len = BUFFER_SIZE};
  wuffs_base__buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

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
    wuffs_base__writer1 got_writer = {.buf = &got};
    if (wlimit) {
      wlim = wlimit;
      got_writer.private_impl.limit.ptr_to_len = &wlim;
    }
    wuffs_base__reader1 src_reader = {.buf = &src};
    if (rlimit) {
      rlim = rlimit;
      src_reader.private_impl.limit.ptr_to_len = &rlim;
    }
    size_t old_wi = got.wi;
    size_t old_ri = src.ri;

    wuffs_gif__status status =
        wuffs_gif__lzw_decoder__decode(&dec, got_writer, src_reader);
    if (status == WUFFS_GIF__STATUS_OK) {
      if (src.ri != src.wi) {
        FAIL("decode returned ok but src was not exhausted");
        return false;
      }
      break;
    }
    if ((status != WUFFS_GIF__SUSPENSION_SHORT_READ) &&
        (status != WUFFS_GIF__SUSPENSION_SHORT_WRITE)) {
      FAIL("status: got %" PRIi32 " (%s), want %" PRIi32 " (%s) or %" PRIi32
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

  return buf1s_equal("", &got, &want);
}

void test_wuffs_gif_lzw_decode_many_big_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_lzw_decode("../../testdata/bricks-gray.indexes.giflzw",
                               14731, "../../testdata/bricks-gray.indexes",
                               19200, 0, 4096);
}

void test_wuffs_gif_lzw_decode_many_small_writes_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_lzw_decode("../../testdata/bricks-gray.indexes.giflzw",
                               14731, "../../testdata/bricks-gray.indexes",
                               19200, 41, 43);
}

void test_wuffs_gif_lzw_decode_bricks_dither() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_lzw_decode("../../testdata/bricks-dither.indexes.giflzw",
                               14923, "../../testdata/bricks-dither.indexes",
                               19200, 0, 0);
}

void test_wuffs_gif_lzw_decode_bricks_nodither() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_lzw_decode("../../testdata/bricks-nodither.indexes.giflzw",
                               13382, "../../testdata/bricks-nodither.indexes",
                               19200, 0, 0);
}

void test_wuffs_gif_lzw_decode_pi() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_lzw_decode("../../testdata/pi.txt.giflzw", 50550,
                               "../../testdata/pi.txt", 100003, 0, 0);
}

// ---------------- LZW Benches

bool do_bench_wuffs_gif_lzw_decode(const char* filename, uint64_t reps) {
  wuffs_base__buf1 dst = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};
  wuffs_base__writer1 dst_writer = {.buf = &dst};
  wuffs_base__reader1 src_reader = {.buf = &src};

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

void bench_wuffs_gif_lzw_decode_20k() {
  CHECK_FOCUS(__func__);
  do_bench_wuffs_gif_lzw_decode("../../testdata/bricks-gray.indexes.giflzw",
                                5000);
}

void bench_wuffs_gif_lzw_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_wuffs_gif_lzw_decode("../../testdata/pi.txt.giflzw", 1000);
}

// ---------------- GIF Tests

const char* wuffs_gif_decode(wuffs_base__buf1* dst, wuffs_base__buf1* src) {
  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);
  wuffs_base__writer1 dst_writer = {.buf = dst};
  wuffs_base__reader1 src_reader = {.buf = src};
  wuffs_gif__status s =
      wuffs_gif__decoder__decode(&dec, dst_writer, src_reader);
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
  wuffs_base__buf1 got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  if (!read_file(&src, filename)) {
    return false;
  }

  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);
  int num_iters = 0;
  uint64_t wlim = 0;
  uint64_t rlim = 0;
  while (true) {
    num_iters++;
    wuffs_base__writer1 got_writer = {.buf = &got};
    if (wlimit) {
      wlim = wlimit;
      got_writer.private_impl.limit.ptr_to_len = &wlim;
    }
    wuffs_base__reader1 src_reader = {.buf = &src};
    if (rlimit) {
      rlim = rlimit;
      src_reader.private_impl.limit.ptr_to_len = &rlim;
    }
    size_t old_wi = got.wi;
    size_t old_ri = src.ri;

    wuffs_gif__status status =
        wuffs_gif__decoder__decode(&dec, got_writer, src_reader);
    if (status == WUFFS_GIF__STATUS_OK) {
      if (src.ri != src.wi) {
        FAIL("decode returned ok but src was not exhausted");
        return false;
      }
      break;
    }
    if ((status != WUFFS_GIF__SUSPENSION_SHORT_READ) &&
        (status != WUFFS_GIF__SUSPENSION_SHORT_WRITE)) {
      FAIL("status: got %" PRIi32 " (%s), want %" PRIi32 " (%s) or %" PRIi32
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

  // TODO: provide a public API for getting the width and height.
  if (dec.private_impl.f_width != 160) {
    FAIL("width: got %d, want %d", dec.private_impl.f_width, 160);
    return false;
  }
  if (dec.private_impl.f_height != 120) {
    FAIL("height: got %d, want %d", dec.private_impl.f_height, 120);
    return false;
  }

  // TODO: provide a public API for getting the palette.
  wuffs_base__buf1 pal_got = {.ptr = dec.private_impl.f_gct, .len = 3 * 256};
  wuffs_base__buf1 pal_want = {.ptr = global_palette_buffer, .len = 3 * 256};
  pal_got.wi = 3 * 256;
  if (!read_file(&pal_want, palette_filename)) {
    return false;
  }
  if (!buf1s_equal("palette ", &pal_got, &pal_want)) {
    return false;
  }

  wuffs_base__buf1 ind_want = {.ptr = global_want_buffer, .len = BUFFER_SIZE};
  if (!read_file(&ind_want, indexes_filename)) {
    return false;
  }
  return buf1s_equal("indexes ", &got, &ind_want);
}

void test_wuffs_gif_decode_input_is_a_gif() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../testdata/bricks-dither.gif",
                           "../../testdata/bricks-dither.palette",
                           "../../testdata/bricks-dither.indexes", 0, 0);
}

void test_wuffs_gif_decode_input_is_a_gif_many_big_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../testdata/bricks-dither.gif",
                           "../../testdata/bricks-dither.palette",
                           "../../testdata/bricks-dither.indexes", 0, 4096);
}

void test_wuffs_gif_decode_input_is_a_gif_many_medium_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../testdata/bricks-dither.gif",
                           "../../testdata/bricks-dither.palette",
                           "../../testdata/bricks-dither.indexes", 0,
                           787);  // 787 tickles being in the middle of a
                                  // decode_extension skip32 call.
}

void test_wuffs_gif_decode_input_is_a_gif_many_small_writes_reads() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_gif_decode("../../testdata/bricks-dither.gif",
                           "../../testdata/bricks-dither.palette",
                           "../../testdata/bricks-dither.indexes", 11, 13);
}

void test_wuffs_gif_decode_input_is_a_png() {
  CHECK_FOCUS(__func__);

  wuffs_base__buf1 got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  if (!read_file(&src, "../../testdata/bricks-dither.png")) {
    return;
  }

  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);
  wuffs_base__writer1 got_writer = {.buf = &got};
  wuffs_base__reader1 src_reader = {.buf = &src};

  wuffs_gif__status status =
      wuffs_gif__decoder__decode(&dec, got_writer, src_reader);
  if (status != WUFFS_GIF__ERROR_BAD_GIF_HEADER) {
    FAIL("status: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
         wuffs_gif__status__string(status), WUFFS_GIF__ERROR_BAD_GIF_HEADER,
         wuffs_gif__status__string(WUFFS_GIF__ERROR_BAD_GIF_HEADER));
    return;
  }
}

// ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

bool do_test_mimic_gif_decode(const char* filename) {
  wuffs_base__buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};
  if (!read_file(&src, filename)) {
    return false;
  }

  src.ri = 0;
  wuffs_base__buf1 got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  const char* got_msg = wuffs_gif_decode(&got, &src);
  if (got_msg) {
    FAIL("%s", got_msg);
    return false;
  }

  src.ri = 0;
  wuffs_base__buf1 want = {.ptr = global_want_buffer, .len = BUFFER_SIZE};
  const char* want_msg = mimic_gif_decode(&want, &src);
  if (want_msg) {
    FAIL("%s", want_msg);
    return false;
  }

  if (!buf1s_equal("", &got, &want)) {
    return false;
  }

  // TODO: check the palette RGB values, not just the palette indexes (pixels).

  return true;
}

void test_mimic_gif_decode_bricks_dither() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../testdata/bricks-dither.gif");
}

void test_mimic_gif_decode_bricks_gray() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../testdata/bricks-gray.gif");
}

void test_mimic_gif_decode_bricks_nodither() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../testdata/bricks-nodither.gif");
}

void test_mimic_gif_decode_harvesters() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../testdata/harvesters.gif");
}

void test_mimic_gif_decode_hat() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../testdata/hat.gif");
}

void test_mimic_gif_decode_hibiscus() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../testdata/hibiscus.gif");
}

void test_mimic_gif_decode_pjw_thumbnail() {
  CHECK_FOCUS(__func__);
  do_test_mimic_gif_decode("../../testdata/pjw-thumbnail.gif");
}

#endif  // WUFFS_MIMIC

// ---------------- GIF Benches

bool do_bench_gif_decode(const char* (*decode_func)(wuffs_base__buf1*,
                                                    wuffs_base__buf1*),
                         const char* filename,
                         uint64_t reps) {
  wuffs_base__buf1 dst = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

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
  do_bench_gif_decode(wuffs_gif_decode, "../../testdata/pjw-thumbnail.gif",
                      200000);
}

void bench_wuffs_gif_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../testdata/hat.gif", 10000);
}

void bench_wuffs_gif_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../testdata/hibiscus.gif", 1000);
}

void bench_wuffs_gif_decode_1000k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(wuffs_gif_decode, "../../testdata/harvesters.gif", 100);
}

// ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

void bench_mimic_gif_decode_1k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../testdata/pjw-thumbnail.gif",
                      200000);
}

void bench_mimic_gif_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../testdata/hat.gif", 10000);
}

void bench_mimic_gif_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../testdata/hibiscus.gif", 1000);
}

void bench_mimic_gif_decode_1000k() {
  CHECK_FOCUS(__func__);
  do_bench_gif_decode(mimic_gif_decode, "../../testdata/harvesters.gif", 100);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {
    // Basic Tests
    test_basic_bad_argument_out_of_range,  //
    test_basic_bad_receiver,               //
    test_basic_initializer_not_called,     //
    test_basic_wuffs_version_bad,          //
    test_basic_wuffs_version_good,         //
    test_basic_status_is_error,            //
    test_basic_status_strings,             //
    test_basic_sub_struct_initializer,     //

    // LZW Tests
    test_wuffs_gif_lzw_decode_many_big_reads,           //
    test_wuffs_gif_lzw_decode_many_small_writes_reads,  //
    test_wuffs_gif_lzw_decode_bricks_dither,            //
    test_wuffs_gif_lzw_decode_bricks_nodither,          //
    test_wuffs_gif_lzw_decode_pi,                       //

    // GIF Tests
    test_wuffs_gif_decode_input_is_a_gif,                          //
    test_wuffs_gif_decode_input_is_a_gif_many_big_reads,           //
    test_wuffs_gif_decode_input_is_a_gif_many_medium_reads,        //
    test_wuffs_gif_decode_input_is_a_gif_many_small_writes_reads,  //
    test_wuffs_gif_decode_input_is_a_png,                          //

#ifdef WUFFS_MIMIC

    // Mimic Tests
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
    // LZW Benches
    bench_wuffs_gif_lzw_decode_20k,   //
    bench_wuffs_gif_lzw_decode_100k,  //

    // GIF Benches
    bench_wuffs_gif_decode_1k,     //
    bench_wuffs_gif_decode_10k,    //
    bench_wuffs_gif_decode_100k,   //
    bench_wuffs_gif_decode_1000k,  //

#ifdef WUFFS_MIMIC

    // Mimic Benches
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
