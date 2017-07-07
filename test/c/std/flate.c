// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

/*
This test program is typically run indirectly, by the "puffs test" or "puffs
bench" commands. These commands take an optional "-mimic" flag to check that
Puffs' output mimics (i.e. exactly matches) other libraries' output, such as
giflib for GIF, libpng for PNG, etc.

To manually run this test:

for cc in clang gcc; do
  $cc -std=c99 -Wall -Werror flate.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).

Add the "puffs mimic cflags" (everything after the colon below) to the C
compiler flags (after the .c file) to run the mimic tests.

To manually run the benchmarks, replace "-Wall -Werror" with "-O3" and replace
"./a.out" with "./a.out -bench". Combine these changes with the "puffs mimic
cflags" to run the mimic benchmarks.
*/

// !! puffs mimic cflags: -DPUFFS_MIMIC -lz

#include "../../../gen/c/std/flate.c"
#include "../testlib/testlib.c"

const char* proc_filename = "std/flate.c";

// ---------------- Golden Tests

// The src_offset0 and src_offset1 magic numbers come from:
//
// go run script/extract-flate-offsets.go test/testdata/*.gz
//
// The empty comments forces clang-format to place one element per line.

golden_test midsummer_gt = {
    .want_filename = "../../testdata/midsummer.txt",    //
    .src_filename = "../../testdata/midsummer.txt.gz",  //
    .src_offset0 = 24,                                  //
    .src_offset1 = 5166,                                //
};

golden_test pi_gt = {
    .want_filename = "../../testdata/pi.txt",    //
    .src_filename = "../../testdata/pi.txt.gz",  //
    .src_offset0 = 17,                           //
    .src_offset1 = 48335,                        //
};

golden_test romeo_gt = {
    .want_filename = "../../testdata/romeo.txt",    //
    .src_filename = "../../testdata/romeo.txt.gz",  //
    .src_offset0 = 20,                              //
    .src_offset1 = 550,                             //
};

// ---------------- Flate Tests

const char* puffs_flate_decode(puffs_base_buf1* dst, puffs_base_buf1* src) {
  puffs_flate_decoder dec;
  puffs_flate_decoder_constructor(&dec, PUFFS_VERSION, 0);
  puffs_base_writer1 dst_writer = {.buf = dst};
  puffs_base_reader1 src_reader = {.buf = src};
  puffs_flate_status s =
      puffs_flate_decoder_decode(&dec, dst_writer, src_reader);
  puffs_flate_decoder_destructor(&dec);
  if (s) {
    return puffs_flate_status_string(s);
  }
  return NULL;
}

void test_puffs_flate_decode_midsummer() {
  proc_funcname = __func__;
  test_buf1_buf1(puffs_flate_decode, &midsummer_gt);
}

void test_puffs_flate_decode_pi() {
  proc_funcname = __func__;
  test_buf1_buf1(puffs_flate_decode, &pi_gt);
}

void test_puffs_flate_decode_romeo() {
  proc_funcname = __func__;
  test_buf1_buf1(puffs_flate_decode, &romeo_gt);
}

// ---------------- Mimic Tests

#ifdef PUFFS_MIMIC

#include "../mimiclib/flate.c"

void test_mimic_flate_decode_midsummer() {
  proc_funcname = __func__;
  test_buf1_buf1(mimic_flate_decode, &midsummer_gt);
}

void test_mimic_flate_decode_pi() {
  proc_funcname = __func__;
  test_buf1_buf1(mimic_flate_decode, &pi_gt);
}

void test_mimic_flate_decode_romeo() {
  proc_funcname = __func__;
  test_buf1_buf1(mimic_flate_decode, &romeo_gt);
}

#endif  // PUFFS_MIMIC

// ---------------- Flate Benches

void bench_puffs_flate_decode_1k() {
  proc_funcname = __func__;
  bench_buf1_buf1(puffs_flate_decode, &romeo_gt, 200000);
}

void bench_puffs_flate_decode_10k() {
  proc_funcname = __func__;
  bench_buf1_buf1(puffs_flate_decode, &midsummer_gt, 30000);
}

void bench_puffs_flate_decode_100k() {
  proc_funcname = __func__;
  bench_buf1_buf1(puffs_flate_decode, &pi_gt, 3000);
}

// ---------------- Mimic Benches

#ifdef PUFFS_MIMIC

void bench_mimic_flate_decode_1k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_flate_decode, &romeo_gt, 200000);
}

void bench_mimic_flate_decode_10k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_flate_decode, &midsummer_gt, 30000);
}

void bench_mimic_flate_decode_100k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_flate_decode, &pi_gt, 3000);
}

#endif  // PUFFS_MIMIC

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {
    // Flate Tests
    /* TODO: implement puffs_flate, then uncomment these.
    test_puffs_flate_decode_midsummer,  //
    test_puffs_flate_decode_pi,         //
    test_puffs_flate_decode_romeo,      //
    */

#ifdef PUFFS_MIMIC

    // Mimic Tests
    test_mimic_flate_decode_midsummer,  //
    test_mimic_flate_decode_pi,         //
    test_mimic_flate_decode_romeo,      //

#endif  // PUFFS_MIMIC

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {
    // Flate Benches
    bench_puffs_flate_decode_1k,    //
    bench_puffs_flate_decode_10k,   //
    bench_puffs_flate_decode_100k,  //

#ifdef PUFFS_MIMIC

    // Mimic Benches
    bench_mimic_flate_decode_1k,    //
    bench_mimic_flate_decode_10k,   //
    bench_mimic_flate_decode_100k,  //

#endif  // PUFFS_MIMIC

    NULL,
};
