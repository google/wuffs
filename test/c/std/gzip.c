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

// ----------------

/*
This test program is typically run indirectly, by the "wuffs test" or "wuffs
bench" commands. These commands take an optional "-mimic" flag to check that
Wuffs' output mimics (i.e. exactly matches) other libraries' output, such as
giflib for GIF, libpng for PNG, etc.

To manually run this test:

for CC in clang gcc; do
  $CC -std=c99 -Wall -Werror gzip.c && ./a.out
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

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c whitelist which parts of Wuffs to build. That file contains
// the entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__CRC32
#define WUFFS_CONFIG__MODULE__DEFLATE
#define WUFFS_CONFIG__MODULE__GZIP

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/deflate-gzip-zlib.c"
#endif

// ---------------- Golden Tests

golden_test g_gzip_midsummer_gt = {
    .want_filename = "test/data/midsummer.txt",
    .src_filename = "test/data/midsummer.txt.gz",
};

golden_test g_gzip_pi_gt = {
    .want_filename = "test/data/pi.txt",
    .src_filename = "test/data/pi.txt.gz",
};

// ---------------- Gzip Tests

const char*  //
test_wuffs_gzip_decode_interface() {
  CHECK_FOCUS(__func__);
  wuffs_gzip__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_gzip__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
  return do_test__wuffs_base__io_transformer(
      wuffs_gzip__decoder__upcast_as__wuffs_base__io_transformer(&dec),
      "test/data/romeo.txt.gz", 0, SIZE_MAX, 942, 0x0A);
}

const char*  //
wuffs_gzip_decode(wuffs_base__io_buffer* dst,
                  wuffs_base__io_buffer* src,
                  uint32_t wuffs_initialize_flags,
                  uint64_t wlimit,
                  uint64_t rlimit) {
  wuffs_gzip__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_gzip__decoder__initialize(&dec, sizeof dec, WUFFS_VERSION,
                                               wuffs_initialize_flags));

  while (true) {
    wuffs_base__io_buffer limited_dst = make_limited_writer(*dst, wlimit);
    wuffs_base__io_buffer limited_src = make_limited_reader(*src, rlimit);

    wuffs_base__status status = wuffs_gzip__decoder__transform_io(
        &dec, &limited_dst, &limited_src, g_work_slice_u8);

    dst->meta.wi += limited_dst.meta.wi;
    src->meta.ri += limited_src.meta.ri;

    if (((wlimit < UINT64_MAX) &&
         (status.repr == wuffs_base__suspension__short_write)) ||
        ((rlimit < UINT64_MAX) &&
         (status.repr == wuffs_base__suspension__short_read))) {
      continue;
    }
    return status.repr;
  }
}

const char*  //
do_test_wuffs_gzip_checksum(bool ignore_checksum, uint32_t bad_checksum) {
  wuffs_base__io_buffer have = ((wuffs_base__io_buffer){
      .data = g_have_slice_u8,
  });
  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = g_src_slice_u8,
  });

  CHECK_STRING(read_file(&src, g_gzip_midsummer_gt.src_filename));

  // Flip a bit in the gzip checksum, which is in the last 8 bytes of the file.
  if (src.meta.wi < 8) {
    RETURN_FAIL("source file was too short");
  }
  if (bad_checksum) {
    src.data.ptr[src.meta.wi - 1 - (bad_checksum & 7)] ^= 1;
  }

  int end_limit;  // The rlimit, relative to the end of the data.
  for (end_limit = 0; end_limit < 10; end_limit++) {
    wuffs_gzip__decoder dec;
    CHECK_STATUS("initialize",
                 wuffs_gzip__decoder__initialize(
                     &dec, sizeof dec, WUFFS_VERSION,
                     WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
    wuffs_gzip__decoder__set_ignore_checksum(&dec, ignore_checksum);
    have.meta.wi = 0;
    src.meta.ri = 0;

    // Decode the src data in 1 or 2 chunks, depending on whether end_limit is
    // or isn't zero.
    int i;
    for (i = 0; i < 2; i++) {
      uint64_t rlimit = UINT64_MAX;
      const char* want_z = NULL;
      if (i == 0) {
        if (end_limit == 0) {
          continue;
        }
        if (src.meta.wi < end_limit) {
          RETURN_FAIL("end_limit=%d: not enough source data", end_limit);
        }
        rlimit = src.meta.wi - (uint64_t)(end_limit);
        want_z = wuffs_base__suspension__short_read;
      } else {
        want_z = (bad_checksum && !ignore_checksum)
                     ? wuffs_gzip__error__bad_checksum
                     : NULL;
      }

      wuffs_base__io_buffer limited_src = make_limited_reader(src, rlimit);
      wuffs_base__status have_z = wuffs_gzip__decoder__transform_io(
          &dec, &have, &limited_src, g_work_slice_u8);
      src.meta.ri += limited_src.meta.ri;
      if (have_z.repr != want_z) {
        RETURN_FAIL("end_limit=%d: have \"%s\", want \"%s\"", end_limit,
                    have_z.repr, want_z);
      }
    }
  }
  return NULL;
}

const char*  //
test_wuffs_gzip_checksum_ignore() {
  CHECK_FOCUS(__func__);
  return do_test_wuffs_gzip_checksum(true, 8 | 0);
}

const char*  //
test_wuffs_gzip_checksum_verify_bad0() {
  CHECK_FOCUS(__func__);
  return do_test_wuffs_gzip_checksum(false, 8 | 0);
}

const char*  //
test_wuffs_gzip_checksum_verify_bad7() {
  CHECK_FOCUS(__func__);
  return do_test_wuffs_gzip_checksum(false, 8 | 7);
}

const char*  //
test_wuffs_gzip_checksum_verify_good() {
  CHECK_FOCUS(__func__);
  return do_test_wuffs_gzip_checksum(false, 0);
}

const char*  //
test_wuffs_gzip_decode_midsummer() {
  CHECK_FOCUS(__func__);
  return do_test_io_buffers(wuffs_gzip_decode, &g_gzip_midsummer_gt, UINT64_MAX,
                            UINT64_MAX);
}

const char*  //
test_wuffs_gzip_decode_pi() {
  CHECK_FOCUS(__func__);
  return do_test_io_buffers(wuffs_gzip_decode, &g_gzip_pi_gt, UINT64_MAX,
                            UINT64_MAX);
}

// ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

const char*  //
test_mimic_gzip_decode_midsummer() {
  CHECK_FOCUS(__func__);
  return do_test_io_buffers(mimic_gzip_decode, &g_gzip_midsummer_gt, UINT64_MAX,
                            UINT64_MAX);
}

const char*  //
test_mimic_gzip_decode_pi() {
  CHECK_FOCUS(__func__);
  return do_test_io_buffers(mimic_gzip_decode, &g_gzip_pi_gt, UINT64_MAX,
                            UINT64_MAX);
}

#endif  // WUFFS_MIMIC

// ---------------- Gzip Benches

const char*  //
bench_wuffs_gzip_decode_10k() {
  CHECK_FOCUS(__func__);
  return do_bench_io_buffers(
      wuffs_gzip_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      tcounter_dst, &g_gzip_midsummer_gt, UINT64_MAX, UINT64_MAX, 300);
}

const char*  //
bench_wuffs_gzip_decode_100k() {
  CHECK_FOCUS(__func__);
  return do_bench_io_buffers(
      wuffs_gzip_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      tcounter_dst, &g_gzip_pi_gt, UINT64_MAX, UINT64_MAX, 30);
}

// ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

const char*  //
bench_mimic_gzip_decode_10k() {
  CHECK_FOCUS(__func__);
  return do_bench_io_buffers(mimic_gzip_decode, 0, tcounter_dst,
                             &g_gzip_midsummer_gt, UINT64_MAX, UINT64_MAX, 300);
}

const char*  //
bench_mimic_gzip_decode_100k() {
  CHECK_FOCUS(__func__);
  return do_bench_io_buffers(mimic_gzip_decode, 0, tcounter_dst, &g_gzip_pi_gt,
                             UINT64_MAX, UINT64_MAX, 30);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// Note that the gzip mimic tests and benches don't work with
// WUFFS_MIMICLIB_USE_MINIZ_INSTEAD_OF_ZLIB.

proc g_tests[] = {

    test_wuffs_gzip_checksum_ignore,
    test_wuffs_gzip_checksum_verify_bad0,
    test_wuffs_gzip_checksum_verify_bad7,
    test_wuffs_gzip_checksum_verify_good,
    test_wuffs_gzip_decode_interface,
    test_wuffs_gzip_decode_midsummer,
    test_wuffs_gzip_decode_pi,

#ifdef WUFFS_MIMIC

    test_mimic_gzip_decode_midsummer,
    test_mimic_gzip_decode_pi,

#endif  // WUFFS_MIMIC

    NULL,
};

proc g_benches[] = {

    bench_wuffs_gzip_decode_10k,
    bench_wuffs_gzip_decode_100k,

#ifdef WUFFS_MIMIC

    bench_mimic_gzip_decode_10k,
    bench_mimic_gzip_decode_100k,

#endif  // WUFFS_MIMIC

    NULL,
};

int  //
main(int argc, char** argv) {
  g_proc_package_name = "std/gzip";
  return test_main(argc, argv, g_tests, g_benches);
}
