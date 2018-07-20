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
  $CC -std=c99 -Wall -Werror lzw.c && ./a.out
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
// release/c/etc.h whitelist which parts of Wuffs to build. That file contains
// the entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__LZW

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.h"
#include "../testlib/testlib.c"

// ---------------- LZW Tests

bool do_test_wuffs_lzw_decode(const char* src_filename,
                              uint64_t src_size,
                              const char* want_filename,
                              uint64_t want_size,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  wuffs_base__io_buffer got =
      ((wuffs_base__io_buffer){.ptr = global_got_buffer, .len = BUFFER_SIZE});
  wuffs_base__io_buffer want =
      ((wuffs_base__io_buffer){.ptr = global_want_buffer, .len = BUFFER_SIZE});
  wuffs_base__io_buffer src =
      ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});

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

  wuffs_lzw__decoder dec = ((wuffs_lzw__decoder){});
  wuffs_lzw__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);
  wuffs_lzw__decoder__set_literal_width(&dec, literal_width);
  int num_iters = 0;
  while (true) {
    num_iters++;
    wuffs_base__io_writer got_writer = wuffs_base__io_buffer__writer(&got);
    if (wlimit) {
      set_writer_limit(&got_writer, wlimit);
    }
    wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);
    if (rlimit) {
      set_reader_limit(&src_reader, rlimit);
    }
    size_t old_wi = got.wi;
    size_t old_ri = src.ri;

    wuffs_base__status status =
        wuffs_lzw__decoder__decode(&dec, got_writer, src_reader);
    if (status == WUFFS_BASE__STATUS_OK) {
      if (src.ri != src.wi) {
        FAIL("decode returned \"ok\" but src was not exhausted");
        return false;
      }
      break;
    }
    if ((status != WUFFS_BASE__SUSPENSION_SHORT_READ) &&
        (status != WUFFS_BASE__SUSPENSION_SHORT_WRITE)) {
      FAIL("decode: got %" PRIi32 " (%s), want %" PRIi32 " (%s) or %" PRIi32
           " (%s)",
           status, wuffs_lzw__status__string(status),
           WUFFS_BASE__SUSPENSION_SHORT_READ,
           wuffs_lzw__status__string(WUFFS_BASE__SUSPENSION_SHORT_READ),
           WUFFS_BASE__SUSPENSION_SHORT_WRITE,
           wuffs_lzw__status__string(WUFFS_BASE__SUSPENSION_SHORT_WRITE));
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

bool do_bench_wuffs_lzw_decode(const char* filename, uint64_t iters_unscaled) {
  wuffs_base__io_buffer dst =
      ((wuffs_base__io_buffer){.ptr = global_got_buffer, .len = BUFFER_SIZE});
  wuffs_base__io_buffer src =
      ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});
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
  uint64_t iters = iters_unscaled * iterscale;
  for (i = 0; i < iters; i++) {
    dst.wi = 0;
    src.ri = 1;  // Skip the literal width.
    wuffs_lzw__decoder dec = ((wuffs_lzw__decoder){});
    wuffs_lzw__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);
    wuffs_base__status s =
        wuffs_lzw__decoder__decode(&dec, dst_writer, src_reader);
    if (s) {
      FAIL("decode: %" PRIi32 " (%s)", s, wuffs_lzw__status__string(s));
      return false;
    }
    n_bytes += dst.wi;
  }
  bench_finish(iters, n_bytes);
  return true;
}

void bench_wuffs_lzw_decode_20k() {
  CHECK_FOCUS(__func__);
  do_bench_wuffs_lzw_decode("../../data/bricks-gray.indexes.giflzw", 50);
}

void bench_wuffs_lzw_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_wuffs_lzw_decode("../../data/pi.txt.giflzw", 10);
}

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

    test_wuffs_lzw_decode_many_big_reads,           //
    test_wuffs_lzw_decode_many_small_writes_reads,  //
    test_wuffs_lzw_decode_bricks_dither,            //
    test_wuffs_lzw_decode_bricks_nodither,          //
    test_wuffs_lzw_decode_pi,                       //

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

    bench_wuffs_lzw_decode_20k,   //
    bench_wuffs_lzw_decode_100k,  //

    NULL,
};

int main(int argc, char** argv) {
  proc_package_name = "std/lzw";
  return test_main(argc, argv, tests, benches);
}
