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

golden_test checksum_midsummer_gt = {
    .src_filename = "../../testdata/midsummer.txt",  //
};

golden_test checksum_pi_gt = {
    .src_filename = "../../testdata/pi.txt",  //
};

golden_test flate_256_bytes_gt = {
    .want_filename = "../../testdata/256.bytes",    //
    .src_filename = "../../testdata/256.bytes.gz",  //
    .src_offset0 = 20,                              //
    .src_offset1 = 281,                             //
};

golden_test flate_midsummer_gt = {
    .want_filename = "../../testdata/midsummer.txt",    //
    .src_filename = "../../testdata/midsummer.txt.gz",  //
    .src_offset0 = 24,                                  //
    .src_offset1 = 5166,                                //
};

golden_test flate_pi_gt = {
    .want_filename = "../../testdata/pi.txt",    //
    .src_filename = "../../testdata/pi.txt.gz",  //
    .src_offset0 = 17,                           //
    .src_offset1 = 48335,                        //
};

golden_test flate_romeo_gt = {
    .want_filename = "../../testdata/romeo.txt",    //
    .src_filename = "../../testdata/romeo.txt.gz",  //
    .src_offset0 = 20,                              //
    .src_offset1 = 550,                             //
};

golden_test gzip_midsummer_gt = {
    .want_filename = "../../testdata/midsummer.txt",    //
    .src_filename = "../../testdata/midsummer.txt.gz",  //
};

golden_test gzip_pi_gt = {
    .want_filename = "../../testdata/pi.txt",    //
    .src_filename = "../../testdata/pi.txt.gz",  //
};

golden_test zlib_midsummer_gt = {
    .want_filename = "../../testdata/midsummer.txt",      //
    .src_filename = "../../testdata/midsummer.txt.zlib",  //
};

golden_test zlib_pi_gt = {
    .want_filename = "../../testdata/pi.txt",      //
    .src_filename = "../../testdata/pi.txt.zlib",  //
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

void test_puffs_flate_decode_256_bytes() {
  proc_funcname = __func__;
  test_buf1_buf1(puffs_flate_decode, &flate_256_bytes_gt);
}

void test_puffs_flate_decode_midsummer() {
  proc_funcname = __func__;
  test_buf1_buf1(puffs_flate_decode, &flate_midsummer_gt);
}

void test_puffs_flate_decode_pi() {
  proc_funcname = __func__;
  test_buf1_buf1(puffs_flate_decode, &flate_pi_gt);
}

void test_puffs_flate_decode_romeo() {
  proc_funcname = __func__;
  test_buf1_buf1(puffs_flate_decode, &flate_romeo_gt);
}

void test_puffs_flate_decode_split_src() {
  proc_funcname = __func__;

  puffs_base_buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};
  puffs_base_buf1 got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  puffs_base_buf1 want = {.ptr = global_want_buffer, .len = BUFFER_SIZE};

  golden_test* gt = &flate_256_bytes_gt;
  if (!read_file(&src, gt->src_filename)) {
    return;
  }
  if (!read_file(&want, gt->want_filename)) {
    return;
  }

  puffs_flate_decoder dec;
  puffs_base_writer1 dst_writer = {.buf = &got};
  puffs_base_reader1 src_reader = {.buf = &src};

  int i;
  for (i = 1; i < 32; i++) {
    size_t split = gt->src_offset0 + i;
    if (split >= gt->src_offset1) {
      FAIL("i=%d: split was not an interior split", i);
      return;
    }
    got.wi = 0;

    puffs_flate_decoder_constructor(&dec, PUFFS_VERSION, 0);

    src.closed = false;
    src.ri = gt->src_offset0;
    src.wi = split;
    puffs_flate_status s0 =
        puffs_flate_decoder_decode(&dec, dst_writer, src_reader);

    src.closed = true;
    src.ri = split;
    src.wi = gt->src_offset1;
    puffs_flate_status s1 =
        puffs_flate_decoder_decode(&dec, dst_writer, src_reader);

    puffs_flate_decoder_destructor(&dec);

    if (s0 != PUFFS_FLATE_SUSPENSION_SHORT_READ) {
      FAIL("i=%d: s0: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", i, s0,
           puffs_flate_status_string(s0), PUFFS_FLATE_SUSPENSION_SHORT_READ,
           puffs_flate_status_string(PUFFS_FLATE_SUSPENSION_SHORT_READ));
      return;
    }

    if (s1 != PUFFS_FLATE_STATUS_OK) {
      FAIL("i=%d: s1: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", i, s1,
           puffs_flate_status_string(s1), PUFFS_FLATE_STATUS_OK,
           puffs_flate_status_string(PUFFS_FLATE_STATUS_OK));
      return;
    }

    char prefix[64];
    snprintf(prefix, 64, "i=%d: ", i);
    if (!buf1s_equal(prefix, &got, &want)) {
      return;
    }
  }
}

// ---------------- Mimic Tests

#ifdef PUFFS_MIMIC

#include "../mimiclib/flate.c"

void test_mimic_flate_decode_256_bytes() {
  proc_funcname = __func__;
  test_buf1_buf1(mimic_flate_decode, &flate_256_bytes_gt);
}

void test_mimic_flate_decode_midsummer() {
  proc_funcname = __func__;
  test_buf1_buf1(mimic_flate_decode, &flate_midsummer_gt);
}

void test_mimic_flate_decode_pi() {
  proc_funcname = __func__;
  test_buf1_buf1(mimic_flate_decode, &flate_pi_gt);
}

void test_mimic_flate_decode_romeo() {
  proc_funcname = __func__;
  test_buf1_buf1(mimic_flate_decode, &flate_romeo_gt);
}

void test_mimic_gzip_decode_midsummer() {
  proc_funcname = __func__;
  test_buf1_buf1(mimic_gzip_decode, &gzip_midsummer_gt);
}

void test_mimic_gzip_decode_pi() {
  proc_funcname = __func__;
  test_buf1_buf1(mimic_gzip_decode, &gzip_pi_gt);
}

void test_mimic_zlib_decode_midsummer() {
  proc_funcname = __func__;
  test_buf1_buf1(mimic_zlib_decode, &zlib_midsummer_gt);
}

void test_mimic_zlib_decode_pi() {
  proc_funcname = __func__;
  test_buf1_buf1(mimic_zlib_decode, &zlib_pi_gt);
}

#endif  // PUFFS_MIMIC

// ---------------- Flate Benches

void bench_puffs_flate_decode_1k() {
  proc_funcname = __func__;
  bench_buf1_buf1(puffs_flate_decode, tc_dst, &flate_romeo_gt, 200000);
}

void bench_puffs_flate_decode_10k() {
  proc_funcname = __func__;
  bench_buf1_buf1(puffs_flate_decode, tc_dst, &flate_midsummer_gt, 30000);
}

void bench_puffs_flate_decode_100k() {
  proc_funcname = __func__;
  bench_buf1_buf1(puffs_flate_decode, tc_dst, &flate_pi_gt, 3000);
}

// ---------------- Mimic Benches

#ifdef PUFFS_MIMIC

void bench_mimic_adler32_10k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_adler32, tc_src, &checksum_midsummer_gt, 30000);
}

void bench_mimic_adler32_100k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_adler32, tc_src, &checksum_pi_gt, 3000);
}

void bench_mimic_crc32_10k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_crc32, tc_src, &checksum_midsummer_gt, 30000);
}

void bench_mimic_crc32_100k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_crc32, tc_src, &checksum_pi_gt, 3000);
}

void bench_mimic_flate_decode_1k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_flate_decode, tc_dst, &flate_romeo_gt, 200000);
}

void bench_mimic_flate_decode_10k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_flate_decode, tc_dst, &flate_midsummer_gt, 30000);
}

void bench_mimic_flate_decode_100k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_flate_decode, tc_dst, &flate_pi_gt, 3000);
}

void bench_mimic_gzip_decode_10k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_gzip_decode, tc_dst, &gzip_midsummer_gt, 30000);
}

void bench_mimic_gzip_decode_100k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_gzip_decode, tc_dst, &gzip_pi_gt, 3000);
}

void bench_mimic_zlib_decode_10k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_zlib_decode, tc_dst, &zlib_midsummer_gt, 30000);
}

void bench_mimic_zlib_decode_100k() {
  proc_funcname = __func__;
  bench_buf1_buf1(mimic_zlib_decode, tc_dst, &zlib_pi_gt, 3000);
}

#endif  // PUFFS_MIMIC

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {
    // Flate Tests
    test_puffs_flate_decode_256_bytes,  //
    /* TODO: implement puffs_flate, then uncomment these.
    test_puffs_flate_decode_midsummer,  //
    test_puffs_flate_decode_pi,         //
    test_puffs_flate_decode_romeo,      //
    */
    test_puffs_flate_decode_split_src,  //

#ifdef PUFFS_MIMIC

    // Mimic Tests
    test_mimic_flate_decode_256_bytes,  //
    test_mimic_flate_decode_midsummer,  //
    test_mimic_flate_decode_pi,         //
    test_mimic_flate_decode_romeo,      //
    test_mimic_gzip_decode_midsummer,   //
    test_mimic_gzip_decode_pi,          //
    test_mimic_zlib_decode_midsummer,   //
    test_mimic_zlib_decode_pi,          //

#endif  // PUFFS_MIMIC

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {
    // Flate Benches
    /* TODO: implement puffs_flate, then uncomment these.
    bench_puffs_flate_decode_1k,    //
    bench_puffs_flate_decode_10k,   //
    bench_puffs_flate_decode_100k,  //
    */

#ifdef PUFFS_MIMIC

    // Mimic Benches
    bench_mimic_adler32_10k,        //
    bench_mimic_adler32_100k,       //
    bench_mimic_crc32_10k,          //
    bench_mimic_crc32_100k,         //
    bench_mimic_flate_decode_1k,    //
    bench_mimic_flate_decode_10k,   //
    bench_mimic_flate_decode_100k,  //
    bench_mimic_gzip_decode_10k,    //
    bench_mimic_gzip_decode_100k,   //
    bench_mimic_zlib_decode_10k,    //
    bench_mimic_zlib_decode_100k,   //

#endif  // PUFFS_MIMIC

    NULL,
};
