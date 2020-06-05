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
// release/c/etc.c whitelist which parts of Wuffs to build. That file contains
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
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/deflate-gzip-zlib.c"
#endif

// ---------------- Golden Tests

golden_test g_crc32_midsummer_gt = {
    .src_filename = "test/data/midsummer.txt",
};

golden_test g_crc32_pi_gt = {
    .src_filename = "test/data/pi.txt",
};

// ---------------- CRC32 Tests

const char*  //
test_wuffs_crc32_ieee_interface() {
  CHECK_FOCUS(__func__);
  wuffs_crc32__ieee_hasher h;
  CHECK_STATUS("initialize",
               wuffs_crc32__ieee_hasher__initialize(
                   &h, sizeof h, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
  return do_test__wuffs_base__hasher_u32(
      wuffs_crc32__ieee_hasher__upcast_as__wuffs_base__hasher_u32(&h),
      "test/data/hat.lossy.webp", 0, SIZE_MAX, 0x89F53B4E);
}

const char*  //
test_wuffs_crc32_ieee_golden() {
  CHECK_FOCUS(__func__);

  struct {
    const char* filename;
    // The want values are determined by script/checksum.go.
    uint32_t want;
  } test_cases[] = {
      {
          .filename = "test/data/hat.bmp",
          .want = 0xA95A578B,
      },
      {
          .filename = "test/data/hat.gif",
          .want = 0xD9743B6A,
      },
      {
          .filename = "test/data/hat.jpeg",
          .want = 0x7F1A90CD,
      },
      {
          .filename = "test/data/hat.lossless.webp",
          .want = 0x485AA040,
      },
      {
          .filename = "test/data/hat.lossy.webp",
          .want = 0x89F53B4E,
      },
      {
          .filename = "test/data/hat.png",
          .want = 0xD5DA5C2F,
      },
      {
          .filename = "test/data/hat.tiff",
          .want = 0xBEF54503,
      },
  };

  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
        .data = g_src_slice_u8,
    });
    CHECK_STRING(read_file(&src, test_cases[tc].filename));

    int j;
    for (j = 0; j < 2; j++) {
      wuffs_crc32__ieee_hasher checksum;
      CHECK_STATUS("initialize",
                   wuffs_crc32__ieee_hasher__initialize(
                       &checksum, sizeof checksum, WUFFS_VERSION,
                       WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

      uint32_t have = 0;
      size_t num_fragments = 0;
      size_t num_bytes = 0;
      do {
        wuffs_base__slice_u8 data = ((wuffs_base__slice_u8){
            .ptr = src.data.ptr + num_bytes,
            .len = src.meta.wi - num_bytes,
        });
        size_t limit = 101 + 103 * num_fragments;
        if ((j > 0) && (data.len > limit)) {
          data.len = limit;
        }
        have = wuffs_crc32__ieee_hasher__update_u32(&checksum, data);
        num_fragments++;
        num_bytes += data.len;
      } while (num_bytes < src.meta.wi);

      if (have != test_cases[tc].want) {
        RETURN_FAIL("tc=%d, j=%d, filename=\"%s\": have 0x%08" PRIX32
                    ", want 0x%08" PRIX32,
                    tc, j, test_cases[tc].filename, have, test_cases[tc].want);
      }
    }
  }
  return NULL;
}

const char*  //
do_test_xxxxx_crc32_ieee_pi(bool mimic) {
  const char* digits =
      "3."
      "141592653589793238462643383279502884197169399375105820974944592307816406"
      "2862089986280348253421170";
  if (strlen(digits) != 99) {
    RETURN_FAIL("strlen(digits): have %d, want 99", (int)(strlen(digits)));
  }

  // The want values are determined by script/checksum.go.
  //
  // wants[i] is the checksum of the first i bytes of the digits string.
  uint32_t wants[100] = {
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
      0xA028F932, 0x3BC16241, 0xF256AB5C, 0xE69E60DA, 0xEBE7C22F, 0xB1EF6496,
      0x740F578E, 0xFEAF7E51, 0x762D849E, 0xEDC1C4D4, 0x028F38BE, 0x31636BA7,
      0xBB354E18, 0xE70C7239, 0x425AFE6E, 0xB09DA8AC, 0x25D02578, 0x4343533F,
      0xACF0D063, 0x20CC1F13, 0x7E9EDAD2, 0xE5A44AA7, 0xC550F584, 0x101040DF,
      0x0BC4A511, 0x706E5A5A, 0x71CE81A3, 0xBB75E3F2, 0x3EDA6848, 0x8B8F08F2,
      0xA9384B2B, 0xB6C07F06, 0x6D644EE4, 0xCAD8CCB4, 0x3F70B461, 0x205F9F77,
      0x4D9D54B7, 0xD69454CC, 0xF8BB504D, 0xFC4E595C,
  };

  int i;
  for (i = 0; i < 100; i++) {
    uint32_t have;
    wuffs_base__slice_u8 data = ((wuffs_base__slice_u8){
        .ptr = (uint8_t*)(digits),
        .len = i,
    });

    if (mimic) {
      // A simple, slow CRC-32 IEEE implementation, 1 bit at a time.
      have = 0xFFFFFFFF;
      while (data.len--) {
        uint8_t byte = *data.ptr++;
        for (int i = 0; i < 8; i++) {
          if ((have ^ byte) & 1) {
            have = (have >> 1) ^ 0xEDB88320;
          } else {
            have = (have >> 1);
          }
          byte >>= 1;
        }
      }
      have ^= 0xFFFFFFFF;

    } else {
      wuffs_crc32__ieee_hasher checksum;
      CHECK_STATUS("initialize",
                   wuffs_crc32__ieee_hasher__initialize(
                       &checksum, sizeof checksum, WUFFS_VERSION,
                       WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
      have = wuffs_crc32__ieee_hasher__update_u32(&checksum, data);
    }

    if (have != wants[i]) {
      RETURN_FAIL("i=%d: have 0x%08" PRIX32 ", want 0x%08" PRIX32, i, have,
                  wants[i]);
    }
  }
  return NULL;
}

const char*  //
test_wuffs_crc32_ieee_pi() {
  CHECK_FOCUS(__func__);
  return do_test_xxxxx_crc32_ieee_pi(false);
}

// ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

const char*  //
test_mimic_crc32_ieee_pi() {
  CHECK_FOCUS(__func__);
  return do_test_xxxxx_crc32_ieee_pi(true);
}

#endif  // WUFFS_MIMIC

// ---------------- CRC32 Benches

uint32_t g_wuffs_crc32_unused_u32;

const char*  //
wuffs_bench_crc32_ieee(wuffs_base__io_buffer* dst,
                       wuffs_base__io_buffer* src,
                       uint32_t wuffs_initialize_flags,
                       uint64_t wlimit,
                       uint64_t rlimit) {
  uint64_t len = src->meta.wi - src->meta.ri;
  if (rlimit) {
    len = wuffs_base__u64__min(len, rlimit);
  }
  wuffs_crc32__ieee_hasher checksum;
  CHECK_STATUS("initialize", wuffs_crc32__ieee_hasher__initialize(
                                 &checksum, sizeof checksum, WUFFS_VERSION,
                                 wuffs_initialize_flags));
  g_wuffs_crc32_unused_u32 = wuffs_crc32__ieee_hasher__update_u32(
      &checksum, ((wuffs_base__slice_u8){
                     .ptr = src->data.ptr + src->meta.ri,
                     .len = len,
                 }));
  src->meta.ri += len;
  return NULL;
}

const char*  //
bench_wuffs_crc32_ieee_10k() {
  CHECK_FOCUS(__func__);
  return do_bench_io_buffers(
      wuffs_bench_crc32_ieee,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED, tcounter_src,
      &g_crc32_midsummer_gt, UINT64_MAX, UINT64_MAX, 1500);
}

const char*  //
bench_wuffs_crc32_ieee_100k() {
  CHECK_FOCUS(__func__);
  return do_bench_io_buffers(
      wuffs_bench_crc32_ieee,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED, tcounter_src,
      &g_crc32_pi_gt, UINT64_MAX, UINT64_MAX, 150);
}

// ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

const char*  //
bench_mimic_crc32_ieee_10k() {
  CHECK_FOCUS(__func__);
  return do_bench_io_buffers(mimic_bench_crc32_ieee, 0, tcounter_src,
                             &g_crc32_midsummer_gt, UINT64_MAX, UINT64_MAX,
                             1500);
}

const char*  //
bench_mimic_crc32_ieee_100k() {
  CHECK_FOCUS(__func__);
  return do_bench_io_buffers(mimic_bench_crc32_ieee, 0, tcounter_src,
                             &g_crc32_pi_gt, UINT64_MAX, UINT64_MAX, 150);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// Note that the crc32 mimic tests and benches don't work with
// WUFFS_MIMICLIB_USE_MINIZ_INSTEAD_OF_ZLIB.

proc g_tests[] = {

    test_wuffs_crc32_ieee_golden,
    test_wuffs_crc32_ieee_interface,
    test_wuffs_crc32_ieee_pi,

#ifdef WUFFS_MIMIC

    test_mimic_crc32_ieee_pi,

#endif  // WUFFS_MIMIC

    NULL,
};

proc g_benches[] = {

    bench_wuffs_crc32_ieee_10k,
    bench_wuffs_crc32_ieee_100k,

#ifdef WUFFS_MIMIC

    bench_mimic_crc32_ieee_10k,
    bench_mimic_crc32_ieee_100k,

#endif  // WUFFS_MIMIC

    NULL,
};

int  //
main(int argc, char** argv) {
  g_proc_package_name = "std/crc32";
  return test_main(argc, argv, g_tests, g_benches);
}
