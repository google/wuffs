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
  $cc -std=c99 -Wall -Werror crc32.c && ./a.out
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
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/deflate-gzip-zlib.c"
#endif

// ---------------- Golden Tests

golden_test crc32_midsummer_gt = {
    .src_filename = "../../data/midsummer.txt",  //
};

golden_test crc32_pi_gt = {
    .src_filename = "../../data/pi.txt",  //
};

// ---------------- CRC32 Tests

void test_wuffs_crc32() {
  CHECK_FOCUS(__func__);
  // TODO: implement.
}

// ---------------- CRC32 Benches

uint32_t global_wuffs_crc32_unused_u32;

const char* wuffs_bench_crc32(wuffs_base__buf1* dst,
                              wuffs_base__buf1* src,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  // TODO: don't ignore wlimit and rlimit.
  wuffs_crc32__ieee checksum;
  wuffs_crc32__ieee__initialize(&checksum, WUFFS_VERSION, 0);
  // TODO: global_wuffs_crc32_unused_u32 =
  wuffs_crc32__ieee__update(&checksum, ((wuffs_base__slice_u8){
                                           .ptr = src->ptr + src->ri,
                                           .len = src->wi - src->ri,
                                       }));
  src->ri = src->wi;
  return NULL;
}

void bench_wuffs_crc32_10k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(wuffs_bench_crc32, tc_src, &crc32_midsummer_gt, 0, 0,
                     150000);
}

void bench_wuffs_crc32_100k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(wuffs_bench_crc32, tc_src, &crc32_pi_gt, 0, 0, 15000);
}

  // ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

void bench_mimic_crc32_10k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(mimic_bench_crc32, tc_src, &crc32_midsummer_gt, 0, 0,
                     150000);
}

void bench_mimic_crc32_100k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(mimic_bench_crc32, tc_src, &crc32_pi_gt, 0, 0, 15000);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

    test_wuffs_crc32,  //

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

    bench_wuffs_crc32_10k,   //
    bench_wuffs_crc32_100k,  //

#ifdef WUFFS_MIMIC

    bench_mimic_crc32_10k,   //
    bench_mimic_crc32_100k,  //

#endif  // WUFFS_MIMIC

    NULL,
};

int main(int argc, char** argv) {
  proc_filename = "std/crc32.c";
  return test_main(argc, argv, tests, benches);
}
