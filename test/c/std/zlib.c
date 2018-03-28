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
    .src_filename = "../../data/midsummer.txt",  //
};

golden_test adler32_pi_gt = {
    .src_filename = "../../data/pi.txt",  //
};

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

// ---------------- Adler32 Tests

void test_wuffs_adler32_golden() {
  CHECK_FOCUS(__func__);

  struct {
    const char* filename;
    // The want values are determined by script/checksum.go.
    uint32_t want;
  } test_cases[] = {
      {
          .filename = "../../data/hat.bmp",
          .want = 0x3D26D034,
      },
      {
          .filename = "../../data/hat.gif",
          .want = 0x2A5EB144,
      },
      {
          .filename = "../../data/hat.jpeg",
          .want = 0x3A503B1A,
      },
      {
          .filename = "../../data/hat.lossless.webp",
          .want = 0xD059D427,
      },
      {
          .filename = "../../data/hat.lossy.webp",
          .want = 0xF1BB258D,
      },
      {
          .filename = "../../data/hat.png",
          .want = 0xDFC6C9C6,
      },
      {
          .filename = "../../data/hat.tiff",
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

void test_wuffs_adler32_pi() {
  CHECK_FOCUS(__func__);

  const char* digits =
      "3.1415926535897932384626433832795028841971693993751058209749445";
  if (strlen(digits) != 63) {
    FAIL("strlen(digits): got %d, want 63", (int)(strlen(digits)));
    return;
  }

  // The want values are determined by script/checksum.go.
  //
  // wants[i] is the checksum of the first i bytes of the digits string.
  uint32_t wants[64] = {
      0x00000001, 0x00340034, 0x00960062, 0x01290093, 0x01F000C7, 0x02E800F8,
      0x0415012D, 0x057B0166, 0x07130198, 0x08E101CE, 0x0AE40203, 0x0D1A0236,
      0x0F85026B, 0x122802A3, 0x150402DC, 0x18170313, 0x1B63034C, 0x1EE2037F,
      0x229303B1, 0x267703E4, 0x2A93041C, 0x2EE30450, 0x33690486, 0x382104B8,
      0x3D0F04EE, 0x42310522, 0x47860555, 0x4D0E0588, 0x52CE05C0, 0x58C105F3,
      0x5EE60625, 0x6542065C, 0x6BD70695, 0x72A106CA, 0x799B06FA, 0x80C7072C,
      0x882B0764, 0x8FC7079C, 0x979707D0, 0x9F980801, 0xA7D2083A, 0xB0430871,
      0xB8E508A2, 0xC1BD08D8, 0xCACE0911, 0xD4120944, 0xDD8F097D, 0xE74509B6,
      0xF12E09E9, 0xFB4E0A20, 0x05B20A55, 0x10380A86, 0x1AEE0AB6, 0x25D90AEB,
      0x30FC0B23, 0x3C510B55, 0x47D60B85, 0x53940BBE, 0x5F890BF5, 0x6BB20C29,
      0x78140C62, 0x84AA0C96, 0x91740CCA, 0x9E730CFF,
  };

  int i;
  for (i = 0; i < 64; i++) {
    wuffs_zlib__adler32 checksum;
    wuffs_zlib__adler32__initialize(&checksum, WUFFS_VERSION, 0);
    uint32_t got =
        wuffs_zlib__adler32__update(&checksum, ((wuffs_base__slice_u8){
                                                   .ptr = (uint8_t*)(digits),
                                                   .len = i,
                                               }));
    if (got != wants[i]) {
      FAIL("i=%d: got 0x%08" PRIX32 ", want 0x%08" PRIX32 "\n", i, got,
           wants[i]);
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

bool do_test_wuffs_zlib_checksum(bool ignore_checksum, bool bad_checksum) {
  wuffs_base__buf1 got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

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
    wuffs_base__writer1 got_writer = {.buf = &got};
    src.ri = 0;
    wuffs_base__reader1 src_reader = {.buf = &src};

    // Decode the src data in 1 or 2 chunks, depending on whether end_limit is
    // or isn't zero.
    int i;
    for (i = 0; i < 2; i++) {
      wuffs_zlib__status want = 0;
      if (i == 0) {
        if (end_limit == 0) {
          continue;
        }
        if (src.wi < end_limit) {
          FAIL("end_limit=%d: not enough source data", end_limit);
          return false;
        }
        uint64_t rlim = src.wi - (uint64_t)(end_limit);
        src_reader.private_impl.limit.ptr_to_len = &rlim;
        want = WUFFS_ZLIB__SUSPENSION_SHORT_READ;
      } else {
        src_reader.private_impl.limit.ptr_to_len = NULL;
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

// Note that the adler32 mimic tests and benches don't work with
// WUFFS_MIMICLIB_USE_MINIZ_INSTEAD_OF_ZLIB.

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

    // These basic tests are really testing the Wuffs compiler. They aren't
    // specific to the std/zlib code, but putting them here is as good as any
    // other place.
    test_basic_status_used_package,  //

    test_wuffs_adler32_golden,  //
    test_wuffs_adler32_pi,      //

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
