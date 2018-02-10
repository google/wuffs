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
#include "../../../gen/c/std/deflate.c"
#include "../../../gen/c/std/zlib.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/deflate-gzip-zlib.c"
#endif

// ---------------- Golden Tests

golden_test adler32_midsummer_gt = {
    .src_filename = "../../testdata/midsummer.txt",  //
};

golden_test adler32_pi_gt = {
    .src_filename = "../../testdata/pi.txt",  //
};

golden_test zlib_midsummer_gt = {
    .want_filename = "../../testdata/midsummer.txt",      //
    .src_filename = "../../testdata/midsummer.txt.zlib",  //
};

golden_test zlib_pi_gt = {
    .want_filename = "../../testdata/pi.txt",      //
    .src_filename = "../../testdata/pi.txt.zlib",  //
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

// ---------------- Adler32 Tests

void test_wuffs_adler32() {
  CHECK_FOCUS(__func__);

  struct {
    const char* filename;
    // The want values are determined by script/adler32sum.go.
    uint32_t want;
  } test_cases[] = {
      {
          .filename = "../../testdata/hat.bmp",
          .want = 0x3D26D034,
      },
      {
          .filename = "../../testdata/hat.gif",
          .want = 0x2A5EB144,
      },
      {
          .filename = "../../testdata/hat.jpeg",
          .want = 0x3A503B1A,
      },
      {
          .filename = "../../testdata/hat.lossless.webp",
          .want = 0xD059D427,
      },
      {
          .filename = "../../testdata/hat.lossy.webp",
          .want = 0xF1BB258D,
      },
      {
          .filename = "../../testdata/hat.png",
          .want = 0xDFC6C9C6,
      },
      {
          .filename = "../../testdata/hat.tiff",
          .want = 0xBDC011E9,
      },
  };

  int i;
  for (i = 0; i < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); i++) {
    wuffs_base__buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};
    if (!read_file(&src, test_cases[i].filename)) {
      return;
    }
    wuffs_zlib__adler32 checksum;
    wuffs_zlib__adler32__initialize(&checksum, WUFFS_VERSION, 0);
    uint32_t got =
        wuffs_zlib__adler32__update(&checksum, ((wuffs_base__slice_u8){
                                                   .ptr = src.ptr + src.ri,
                                                   .len = src.wi - src.ri,
                                               }));
    if (got != test_cases[i].want) {
      FAIL("i=%d, filename=\"%s\": got 0x%08" PRIX32 ", want 0x%08" PRIX32 "\n",
           i, test_cases[i].filename, got, test_cases[i].want);
      return;
    }
  }
}

// ---------------- Zlib Tests

const char* wuffs_zlib_decode(wuffs_base__buf1* dst,
                              wuffs_base__buf1* src,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  wuffs_zlib__decoder dec;
  wuffs_zlib__decoder__initialize(&dec, WUFFS_VERSION, 0);

  uint64_t wlim = 0;
  uint64_t rlim = 0;
  while (true) {
    wuffs_base__writer1 dst_writer = {.buf = dst};
    if (wlimit) {
      wlim = wlimit;
      dst_writer.private_impl.limit.ptr_to_len = &wlim;
    }
    wuffs_base__reader1 src_reader = {.buf = src};
    if (rlimit) {
      rlim = rlimit;
      src_reader.private_impl.limit.ptr_to_len = &rlim;
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

void test_wuffs_zlib_checksum_mismatch() {
  CHECK_FOCUS(__func__);

  wuffs_base__buf1 got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  if (!read_file(&src, zlib_midsummer_gt.src_filename)) {
    return;
  }
  if (src.wi == 0) {
    FAIL("source file was empty");
    return;
  }
  // Flip a bit in the zlib checksum, which comes at the end of the file.
  src.ptr[src.wi - 1] ^= 1;

  wuffs_zlib__decoder dec;
  wuffs_zlib__decoder__initialize(&dec, WUFFS_VERSION, 0);
  wuffs_base__writer1 got_writer = {.buf = &got};
  wuffs_base__reader1 src_reader = {.buf = &src};

  wuffs_zlib__status status =
      wuffs_zlib__decoder__decode(&dec, got_writer, src_reader);
  if (status != WUFFS_ZLIB__ERROR_CHECKSUM_MISMATCH) {
    FAIL("decode: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", status,
         wuffs_zlib__status__string(status),
         WUFFS_ZLIB__ERROR_CHECKSUM_MISMATCH,
         wuffs_zlib__status__string(WUFFS_ZLIB__ERROR_CHECKSUM_MISMATCH));
    return;
  }
}

void test_wuffs_zlib_decode_midsummer() {
  CHECK_FOCUS(__func__);
  do_test_buf1_buf1(wuffs_zlib_decode, &zlib_midsummer_gt, 0, 0);
}

void test_wuffs_zlib_decode_pi() {
  CHECK_FOCUS(__func__);
  do_test_buf1_buf1(wuffs_zlib_decode, &zlib_pi_gt, 0, 0);
}

// ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

void test_mimic_zlib_decode_midsummer() {
  CHECK_FOCUS(__func__);
  do_test_buf1_buf1(mimic_zlib_decode, &zlib_midsummer_gt, 0, 0);
}

void test_mimic_zlib_decode_pi() {
  CHECK_FOCUS(__func__);
  do_test_buf1_buf1(mimic_zlib_decode, &zlib_pi_gt, 0, 0);
}

#endif  // WUFFS_MIMIC

// ---------------- Adler32 Benches

uint32_t global_wuffs_zlib_unused_u32;

const char* wuffs_bench_adler32(wuffs_base__buf1* dst,
                                wuffs_base__buf1* src,
                                uint64_t wlimit,
                                uint64_t rlimit) {
  // TODO: don't ignore wlimit and rlimit.
  wuffs_zlib__adler32 checksum;
  wuffs_zlib__adler32__initialize(&checksum, WUFFS_VERSION, 0);
  global_wuffs_zlib_unused_u32 =
      wuffs_zlib__adler32__update(&checksum, ((wuffs_base__slice_u8){
                                                 .ptr = src->ptr + src->ri,
                                                 .len = src->wi - src->ri,
                                             }));
  src->ri = src->wi;
  return NULL;
}

void bench_wuffs_adler32_10k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(wuffs_bench_adler32, tc_src, &adler32_midsummer_gt, 0, 0,
                     150000);
}

void bench_wuffs_adler32_100k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(wuffs_bench_adler32, tc_src, &adler32_pi_gt, 0, 0, 15000);
}

// ---------------- Zlib Benches

void bench_wuffs_zlib_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(wuffs_zlib_decode, tc_dst, &zlib_midsummer_gt, 0, 0,
                     30000);
}

void bench_wuffs_zlib_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(wuffs_zlib_decode, tc_dst, &zlib_pi_gt, 0, 0, 3000);
}

// ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

void bench_mimic_adler32_10k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(mimic_bench_adler32, tc_src, &adler32_midsummer_gt, 0, 0,
                     150000);
}

void bench_mimic_adler32_100k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(mimic_bench_adler32, tc_src, &adler32_pi_gt, 0, 0, 15000);
}

void bench_mimic_zlib_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(mimic_zlib_decode, tc_dst, &zlib_midsummer_gt, 0, 0,
                     30000);
}

void bench_mimic_zlib_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_buf1_buf1(mimic_zlib_decode, tc_dst, &zlib_pi_gt, 0, 0, 3000);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

    // These basic tests are really testing the Wuffs compiler. They aren't
    // specific to the std/zlib code, but putting them here is as good as any
    // other place.
    test_basic_status_used_package,  //

    test_wuffs_adler32,  //

    test_wuffs_zlib_checksum_mismatch,  //
    test_wuffs_zlib_decode_midsummer,   //
    test_wuffs_zlib_decode_pi,          //

#ifdef WUFFS_MIMIC

    test_mimic_zlib_decode_midsummer,  //
    test_mimic_zlib_decode_pi,         //

#endif  // WUFFS_MIMIC

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

    bench_wuffs_adler32_10k,   //
    bench_wuffs_adler32_100k,  //

    bench_wuffs_zlib_decode_10k,   //
    bench_wuffs_zlib_decode_100k,  //

#ifdef WUFFS_MIMIC

    bench_mimic_adler32_10k,   //
    bench_mimic_adler32_100k,  //

    bench_mimic_zlib_decode_10k,   //
    bench_mimic_zlib_decode_100k,  //

#endif  // WUFFS_MIMIC

    NULL,
};

int main(int argc, char** argv) {
  proc_filename = "std/zlib.c";
  return test_main(argc, argv, tests, benches);
}
