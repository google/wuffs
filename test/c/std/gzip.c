// Copyright 2018 The Wuffs Authors.
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
  $cc -std=c99 -Wall -Werror gzip.c && ./a.out
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
#include "../../../gen/c/std/crc32.c"
#include "../../../gen/c/std/deflate.c"
#include "../../../gen/c/std/gzip.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/deflate-gzip-zlib.c"
#endif

// ---------------- Golden Tests

golden_test gzip_midsummer_gt = {
    .want_filename = "../../data/midsummer.txt",    //
    .src_filename = "../../data/midsummer.txt.gz",  //
};

golden_test gzip_pi_gt = {
    .want_filename = "../../data/pi.txt",    //
    .src_filename = "../../data/pi.txt.gz",  //
};

// ---------------- Gzip Tests

const char* wuffs_gzip_decode(wuffs_base__buf1* dst,
                              wuffs_base__buf1* src,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  // TODO.
  return NULL;
}

void test_wuffs_gzip_decode_midsummer() {
  CHECK_FOCUS(__func__);
  do_test_buf1_buf1(wuffs_gzip_decode, &gzip_midsummer_gt, 0, 0);
}

void test_wuffs_gzip_decode_pi() {
  CHECK_FOCUS(__func__);
  do_test_buf1_buf1(wuffs_gzip_decode, &gzip_pi_gt, 0, 0);
}

  // ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

void test_mimic_gzip_decode_midsummer() {
  CHECK_FOCUS(__func__);
  do_test_buf1_buf1(mimic_gzip_decode, &gzip_midsummer_gt, 0, 0);
}

void test_mimic_gzip_decode_pi() {
  CHECK_FOCUS(__func__);
  do_test_buf1_buf1(mimic_gzip_decode, &gzip_pi_gt, 0, 0);
}

#endif  // WUFFS_MIMIC

// ---------------- Gzip Benches

void bench_wuffs_gzip_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(wuffs_gzip_decode, tc_dst, &gzip_midsummer_gt, 0, 0,
                     30000);
}

void bench_wuffs_gzip_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(wuffs_gzip_decode, tc_dst, &gzip_pi_gt, 0, 0, 3000);
}

  // ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

void bench_mimic_gzip_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(mimic_gzip_decode, tc_dst, &gzip_midsummer_gt, 0, 0,
                     30000);
}

void bench_mimic_gzip_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(mimic_gzip_decode, tc_dst, &gzip_pi_gt, 0, 0, 3000);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// Note that the gzip mimic tests and benches don't work with
// WUFFS_MIMICLIB_USE_MINIZ_INSTEAD_OF_ZLIB.

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

// test_wuffs_gzip_decode_midsummer,  // TODO: uncomment.
// test_wuffs_gzip_decode_pi,         // TODO: uncomment.

#ifdef WUFFS_MIMIC

    test_mimic_gzip_decode_midsummer,  //
    test_mimic_gzip_decode_pi,         //

#endif  // WUFFS_MIMIC

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

    bench_wuffs_gzip_decode_10k,   //
    bench_wuffs_gzip_decode_100k,  //

#ifdef WUFFS_MIMIC

    bench_mimic_gzip_decode_10k,   //
    bench_mimic_gzip_decode_100k,  //

#endif  // WUFFS_MIMIC

    NULL,
};

int main(int argc, char** argv) {
  proc_filename = "std/gzip.c";
  return test_main(argc, argv, tests, benches);
}
