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

for CC in clang gcc; do
  $CC -std=c99 -Wall -Werror crc32.c && ./a.out
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
// release/c/etc.h whitelist which parts of Wuffs to build. That file contains
// the entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__CRC32

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.h"
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

void test_wuffs_crc32_ieee_golden() {
  CHECK_FOCUS(__func__);

  struct {
    const char* filename;
    // The want values are determined by script/checksum.go.
    uint32_t want;
  } test_cases[] = {
      {
          .filename = "../../data/hat.bmp",
          .want = 0xA95A578B,
      },
      {
          .filename = "../../data/hat.gif",
          .want = 0xD9743B6A,
      },
      {
          .filename = "../../data/hat.jpeg",
          .want = 0x7F1A90CD,
      },
      {
          .filename = "../../data/hat.lossless.webp",
          .want = 0x485AA040,
      },
      {
          .filename = "../../data/hat.lossy.webp",
          .want = 0x89F53B4E,
      },
      {
          .filename = "../../data/hat.png",
          .want = 0xD5DA5C2F,
      },
      {
          .filename = "../../data/hat.tiff",
          .want = 0xBEF54503,
      },
  };

  int i;
  for (i = 0; i < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); i++) {
    wuffs_base__io_buffer src =
        ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});
    if (!read_file(&src, test_cases[i].filename)) {
      return;
    }

    int j;
    for (j = 0; j < 2; j++) {
      wuffs_crc32__ieee_hasher checksum = ((wuffs_crc32__ieee_hasher){});
      wuffs_crc32__ieee_hasher__check_wuffs_version(&checksum, sizeof checksum,
                                                    WUFFS_VERSION);

      uint32_t got = 0;
      size_t num_fragments = 0;
      size_t num_bytes = 0;
      do {
        wuffs_base__slice_u8 data = ((wuffs_base__slice_u8){
            .ptr = src.ptr + num_bytes,
            .len = src.wi - num_bytes,
        });
        size_t limit = 101 + 103 * num_fragments;
        if ((j > 0) && (data.len > limit)) {
          data.len = limit;
        }
        got = wuffs_crc32__ieee_hasher__update(&checksum, data);
        num_fragments++;
        num_bytes += data.len;
      } while (num_bytes < src.wi);

      if (got != test_cases[i].want) {
        FAIL("i=%d, j=%d, filename=\"%s\": got 0x%08" PRIX32
             ", want 0x%08" PRIX32 "\n",
             i, j, test_cases[i].filename, got, test_cases[i].want);
        return;
      }
    }
  }
}

void test_wuffs_crc32_ieee_pi() {
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
      0x00000000, 0x6DD28E9B, 0x69647A00, 0x83B58BCD, 0x16E010BE, 0xAF13912C,
      0xB6C654DC, 0x02D43F2E, 0xC60167FD, 0xDE72F5D2, 0xECB2EAA3, 0x22E1CE23,
      0x26F4BB12, 0x099FD2E0, 0x2D041A2F, 0xC14373C1, 0x61A5D6D0, 0xEB60F999,
      0x93EDF514, 0x779BB713, 0x7EC98D7A, 0x43184A97, 0x739064B9, 0xA81B2541,
      0x1CCB1037, 0x4B177527, 0xC8932C85, 0xF0C86A18, 0xE99C072F, 0xC6EA2FC5,
      0xF11D621D, 0x09483B39, 0xD20BA7B6, 0xA66136B0, 0x3F1C0D9B, 0x7D37E8CC,
      0x68AFEE60, 0xB7DA99A5, 0x55BD96C6, 0xF18E35A4, 0x5C4D8E41, 0x6B38760A,
      0x63623EDF, 0x0BB7D76F, 0x5001AC9B, 0x0A5FC5FB, 0xA76213D4, 0x0C1E135B,
      0x916718F4, 0xD0FE1B9F, 0xE4D15B60, 0xCE8A5FB4, 0x381922EB, 0xB351097C,
      0xA3003B0D, 0x64C7C28B, 0x8ED5424B, 0x6C872ADF, 0x7CBF02ED, 0x2D713AFF,
      0xA028F932, 0x3BC16241, 0xF256AB5C, 0xE69E60DA,
  };

  int i;
  for (i = 0; i < 64; i++) {
    wuffs_crc32__ieee_hasher checksum = ((wuffs_crc32__ieee_hasher){});
    wuffs_crc32__ieee_hasher__check_wuffs_version(&checksum, sizeof checksum,
                                                  WUFFS_VERSION);
    uint32_t got = wuffs_crc32__ieee_hasher__update(
        &checksum, ((wuffs_base__slice_u8){
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

// ---------------- CRC32 Benches

uint32_t global_wuffs_crc32_unused_u32;

const char* wuffs_bench_crc32_ieee(wuffs_base__io_buffer* dst,
                                   wuffs_base__io_buffer* src,
                                   uint64_t wlimit,
                                   uint64_t rlimit) {
  uint64_t len = src->wi - src->ri;
  if (rlimit) {
    len = wuffs_base__u64__min(len, rlimit);
  }
  wuffs_crc32__ieee_hasher checksum = ((wuffs_crc32__ieee_hasher){});
  wuffs_crc32__ieee_hasher__check_wuffs_version(&checksum, sizeof checksum,
                                                WUFFS_VERSION);
  global_wuffs_crc32_unused_u32 =
      wuffs_crc32__ieee_hasher__update(&checksum, ((wuffs_base__slice_u8){
                                                      .ptr = src->ptr + src->ri,
                                                      .len = len,
                                                  }));
  src->ri += len;
  return NULL;
}

void bench_wuffs_crc32_ieee_10k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(wuffs_bench_crc32_ieee, tc_src, &crc32_midsummer_gt, 0, 0,
                      1500);
}

void bench_wuffs_crc32_ieee_100k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(wuffs_bench_crc32_ieee, tc_src, &crc32_pi_gt, 0, 0, 150);
}

  // ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

void bench_mimic_crc32_ieee_10k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(mimic_bench_crc32_ieee, tc_src, &crc32_midsummer_gt, 0, 0,
                      1500);
}

void bench_mimic_crc32_ieee_100k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(mimic_bench_crc32_ieee, tc_src, &crc32_pi_gt, 0, 0, 150);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// Note that the crc32 mimic tests and benches don't work with
// WUFFS_MIMICLIB_USE_MINIZ_INSTEAD_OF_ZLIB.

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

    test_wuffs_crc32_ieee_golden,  //
    test_wuffs_crc32_ieee_pi,      //

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

    bench_wuffs_crc32_ieee_10k,   //
    bench_wuffs_crc32_ieee_100k,  //

#ifdef WUFFS_MIMIC

    bench_mimic_crc32_ieee_10k,   //
    bench_mimic_crc32_ieee_100k,  //

#endif  // WUFFS_MIMIC

    NULL,
};

int main(int argc, char** argv) {
  proc_package_name = "std/crc32";
  return test_main(argc, argv, tests, benches);
}
