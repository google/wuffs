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
  $cc -std=c99 -Wall -Werror zlib.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).

Add the "wuffs mimic cflags" (everything after the colon below) to the C
compiler flags (after the .c file) to run the mimic tests.

To manually run the benchmarks, replace "-Wall -Werror" with "-O3" and replace
the first "./a.out" with "./a.out -bench". Combine these changes with the
"wuffs mimic cflags" to run the mimic benchmarks.
*/

// !! wuffs mimic cflags: -DWUFFS_MIMIC -lz

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../gen/c/std/adler32.c"
#include "../../../gen/c/std/deflate.c"
#include "../../../gen/c/std/zlib.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/deflate-gzip-zlib.c"
#endif

// ---------------- Golden Tests

golden_test zlib_midsummer_gt = {
    .want_filename = "../../data/midsummer.txt",      //
    .src_filename = "../../data/midsummer.txt.zlib",  //
};

golden_test zlib_pi_gt = {
    .want_filename = "../../data/pi.txt",      //
    .src_filename = "../../data/pi.txt.zlib",  //
};

// ---------------- Basic Tests

void test_basic_status_used_package() {
  CHECK_FOCUS(__func__);
  // The function call here is from "std/zlib" but the argument is from
  // "std/deflate". The former package depends on the latter.
  const char* s0 =
      wuffs_zlib__status__string(WUFFS_DEFLATE__ERROR_BAD_DISTANCE);
  const char* t0 = "deflate: bad distance";
  if (strcmp(s0, t0)) {
    FAIL("got \"%s\", want \"%s\"", s0, t0);
    return;
  }
}

// ---------------- Zlib Tests

const char* wuffs_zlib_decode(wuffs_base__io_buffer* dst,
                              wuffs_base__io_buffer* src,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  wuffs_zlib__decoder dec;
  wuffs_zlib__decoder__initialize(&dec, WUFFS_VERSION, 0);

  while (true) {
    wuffs_base__io_writer dst_writer = wuffs_base__io_buffer__writer(dst);
    if (wlimit) {
      wuffs_base__io_writer__set_limit(&dst_writer, wlimit);
    }
    wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(src);
    if (rlimit) {
      wuffs_base__io_reader__set_limit(&src_reader, rlimit);
    }

    wuffs_zlib__status s =
        wuffs_zlib__decoder__decode(&dec, dst_writer, src_reader);

    if (s == WUFFS_ZLIB__STATUS_OK) {
      return NULL;
    }
    if ((wlimit && (s == WUFFS_ZLIB__SUSPENSION_SHORT_WRITE)) ||
        (rlimit && (s == WUFFS_ZLIB__SUSPENSION_SHORT_READ))) {
      continue;
    }
    return wuffs_zlib__status__string(s);
  }
}

bool do_test_wuffs_zlib_checksum(bool ignore_checksum, bool bad_checksum) {
  wuffs_base__io_buffer got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__io_buffer src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  if (!read_file(&src, zlib_midsummer_gt.src_filename)) {
    return false;
  }
  // Flip a bit in the zlib checksum, which is in the last 4 bytes of the file.
  if (src.wi < 4) {
    FAIL("source file was too short");
    return false;
  }
  if (bad_checksum) {
    src.ptr[src.wi - 1 - (bad_checksum & 3)] ^= 1;
  }

  int end_limit;
  for (end_limit = 0; end_limit < 10; end_limit++) {
    wuffs_zlib__decoder dec;
    wuffs_zlib__decoder__initialize(&dec, WUFFS_VERSION, 0);
    wuffs_zlib__decoder__set_ignore_checksum(&dec, ignore_checksum);
    got.wi = 0;
    wuffs_base__io_writer got_writer = wuffs_base__io_buffer__writer(&got);
    src.ri = 0;

    // Decode the src data in 1 or 2 chunks, depending on whether end_limit is
    // or isn't zero.
    int i;
    for (i = 0; i < 2; i++) {
      wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);
      wuffs_zlib__status want = 0;
      if (i == 0) {
        if (end_limit == 0) {
          continue;
        }
        if (src.wi < end_limit) {
          FAIL("end_limit=%d: not enough source data", end_limit);
          return false;
        }
        wuffs_base__io_reader__set_limit(&src_reader,
                                         src.wi - (uint64_t)(end_limit));
        want = WUFFS_ZLIB__SUSPENSION_SHORT_READ;
      } else {
        want = (bad_checksum && !ignore_checksum)
                   ? WUFFS_ZLIB__ERROR_CHECKSUM_MISMATCH
                   : WUFFS_ZLIB__STATUS_OK;
      }

      wuffs_zlib__status status =
          wuffs_zlib__decoder__decode(&dec, got_writer, src_reader);
      if (status != want) {
        FAIL("end_limit=%d: got %" PRIi32 " (%s), want %" PRIi32 " (%s)",
             end_limit, status, wuffs_zlib__status__string(status), want,
             wuffs_zlib__status__string(want));
        return false;
      }
    }
  }
  return true;
}

void test_wuffs_zlib_checksum_ignore() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_zlib_checksum(true, 1);
}

void test_wuffs_zlib_checksum_verify_bad() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_zlib_checksum(false, 1);
}

void test_wuffs_zlib_checksum_verify_good() {
  CHECK_FOCUS(__func__);
  do_test_wuffs_zlib_checksum(false, 0);
}

void test_wuffs_zlib_decode_midsummer() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_zlib_decode, &zlib_midsummer_gt, 0, 0);
}

void test_wuffs_zlib_decode_pi() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_zlib_decode, &zlib_pi_gt, 0, 0);
}

  // ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

void test_mimic_zlib_decode_midsummer() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(mimic_zlib_decode, &zlib_midsummer_gt, 0, 0);
}

void test_mimic_zlib_decode_pi() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(mimic_zlib_decode, &zlib_pi_gt, 0, 0);
}

#endif  // WUFFS_MIMIC

// ---------------- Zlib Benches

void bench_wuffs_zlib_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(wuffs_zlib_decode, tc_dst, &zlib_midsummer_gt, 0, 0,
                      300);
}

void bench_wuffs_zlib_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(wuffs_zlib_decode, tc_dst, &zlib_pi_gt, 0, 0, 30);
}

  // ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

void bench_mimic_zlib_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(mimic_zlib_decode, tc_dst, &zlib_midsummer_gt, 0, 0,
                      300);
}

void bench_mimic_zlib_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(mimic_zlib_decode, tc_dst, &zlib_pi_gt, 0, 0, 30);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

    // These basic tests are really testing the Wuffs compiler. They aren't
    // specific to the std/zlib code, but putting them here is as good as any
    // other place.
    test_basic_status_used_package,  //

    test_wuffs_zlib_checksum_ignore,       //
    test_wuffs_zlib_checksum_verify_bad,   //
    test_wuffs_zlib_checksum_verify_good,  //
    test_wuffs_zlib_decode_midsummer,      //
    test_wuffs_zlib_decode_pi,             //

#ifdef WUFFS_MIMIC

    test_mimic_zlib_decode_midsummer,  //
    test_mimic_zlib_decode_pi,         //

#endif  // WUFFS_MIMIC

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

    bench_wuffs_zlib_decode_10k,   //
    bench_wuffs_zlib_decode_100k,  //

#ifdef WUFFS_MIMIC

    bench_mimic_zlib_decode_10k,   //
    bench_mimic_zlib_decode_100k,  //

#endif  // WUFFS_MIMIC

    NULL,
};

int main(int argc, char** argv) {
  proc_package_name = "std/zlib";
  return test_main(argc, argv, tests, benches);
}
