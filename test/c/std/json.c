// Copyright 2020 The Wuffs Authors.
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
  $CC -std=c99 -Wall -Werror json.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).

Add the "wuffs mimic cflags" (everything after the colon below) to the C
compiler flags (after the .c file) to run the mimic tests.

To manually run the benchmarks, replace "-Wall -Werror" with "-O3" and replace
the first "./a.out" with "./a.out -bench". Combine these changes with the
"wuffs mimic cflags" to run the mimic benchmarks.
*/

// !! wuffs mimic cflags: -DWUFFS_MIMIC

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
#define WUFFS_CONFIG__MODULE__JSON

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
// No mimic library.
#endif

// ---------------- String Conversions Tests

const char* test_strconv_parse_number_u64() {
  CHECK_FOCUS(__func__);

  const uint64_t fail = 0xDEADBEEF;

  struct {
    uint64_t want;
    const char* str;
  } test_cases[] = {
      {.want = 0x0000000000000000, .str = "0"},
      {.want = 0x0000000000000000, .str = "0_"},
      {.want = 0x0000000000000000, .str = "0d0"},
      {.want = 0x0000000000000000, .str = "0x000"},
      {.want = 0x0000000000000000, .str = "_0"},
      {.want = 0x0000000000000000, .str = "__0__"},
      {.want = 0x000000000000004A, .str = "0x4A"},
      {.want = 0x000000000000004B, .str = "0x__4_B_"},
      {.want = 0x000000000000007B, .str = "123"},
      {.want = 0x000000000000007C, .str = "12_4"},
      {.want = 0x000000000000007D, .str = "_1__2________5_"},
      {.want = 0x00000000000001F4, .str = "0d500"},
      {.want = 0x00000000000001F5, .str = "0D___5_01__"},
      {.want = 0x00000000FFFFFFFF, .str = "4294967295"},
      {.want = 0x0000000100000000, .str = "4294967296"},
      {.want = 0x0123456789ABCDEF, .str = "0x0123456789ABCDEF"},
      {.want = 0x0123456789ABCDEF, .str = "0x0123456789abcdef"},
      {.want = 0xFFFFFFFFFFFFFFF9, .str = "18446744073709551609"},
      {.want = 0xFFFFFFFFFFFFFFFA, .str = "18446744073709551610"},
      {.want = 0xFFFFFFFFFFFFFFFE, .str = "0xFFFFffffFFFFfffe"},
      {.want = 0xFFFFFFFFFFFFFFFE, .str = "18446744073709551614"},
      {.want = 0xFFFFFFFFFFFFFFFF, .str = "0xFFFF_FFFF_FFFF_FFFF"},
      {.want = 0xFFFFFFFFFFFFFFFF, .str = "18446744073709551615"},

      {.want = fail, .str = " "},
      {.want = fail, .str = " 0"},
      {.want = fail, .str = " 12 "},
      {.want = fail, .str = ""},
      {.want = fail, .str = "+0"},
      {.want = fail, .str = "+1"},
      {.want = fail, .str = "-0"},
      {.want = fail, .str = "-1"},
      {.want = fail, .str = "0 "},
      {.want = fail, .str = "0_x1"},
      {.want = fail, .str = "0d___"},
      {.want = fail, .str = "0x"},
      {.want = fail, .str = "0x10000000000000000"},      // 1 << 64.
      {.want = fail, .str = "0x1_0000_0000_0000_0000"},  // 1 << 64.
      {.want = fail, .str = "1 23"},
      {.want = fail, .str = "1,23"},
      {.want = fail, .str = "1.23"},
      {.want = fail, .str = "123 "},
      {.want = fail, .str = "123456789012345678901234"},
      {.want = fail, .str = "12a3"},
      {.want = fail, .str = "18446744073709551616"},  // UINT64_MAX.
      {.want = fail, .str = "18446744073709551617"},
      {.want = fail, .str = "18446744073709551618"},
      {.want = fail, .str = "18446744073709551619"},
      {.want = fail, .str = "18446744073709551620"},
      {.want = fail, .str = "18446744073709551621"},
      {.want = fail, .str = "_"},
      {.want = fail, .str = "d"},
      {.want = fail, .str = "x"},
  };

  int i;
  for (i = 0; i < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); i++) {
    wuffs_base__result_u64 r =
        wuffs_base__parse_number_u64(wuffs_base__make_slice_u8(
            (void*)test_cases[i].str, strlen(test_cases[i].str)));
    uint64_t got = (r.status.repr == NULL) ? r.value : fail;
    if (got != test_cases[i].want) {
      RETURN_FAIL("\"%s\": got 0x%" PRIX64 ", want 0x%" PRIX64,
                  test_cases[i].str, got, test_cases[i].want);
    }
  }

  return NULL;
}

// ---------------- JSON Tests

const char* test_wuffs_json_decode_tokens() {
  CHECK_FOCUS(__func__);
  wuffs_json__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_json__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  wuffs_base__token_buffer got = ((wuffs_base__token_buffer){
      .data = global_got_token_slice,
  });
  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = global_src_slice,
  });
  CHECK_STRING(read_file(&src, "test/data/github-tags.json"));

  wuffs_base__status status =
      wuffs_json__decoder__decode_tokens(&dec, &got, &src);
  if (0) {
    uint64_t pos = 0;
    size_t i;
    for (i = got.meta.ri; i < got.meta.wi; i++) {
      wuffs_base__token* t = &got.data.ptr[i];
      uint64_t len = wuffs_base__token__length(t);
      uint64_t bc = wuffs_base__token__value_base_category(t);
      uint64_t bd = wuffs_base__token__value_base_detail(t);
      printf("i=%3zu\tp=%4" PRIu64 " (0x%03" PRIX64 ")\tl=%3" PRIu64
             "\tbc=%3" PRIu64 "\tbd=0x%06" PRIX64 "\n",
             i, pos, pos, len, bc, bd);
      pos = wuffs_base__u64__sat_add(pos, len);
    }
    CHECK_STATUS("decode_tokens", status);
  }

  return NULL;
}

  // ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

  // No mimic tests.

#endif  // WUFFS_MIMIC

  // ---------------- JSON Benches

  // No JSON benches.

  // ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

  // No mimic benches.

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

    // These strconv tests are really testing the Wuffs base library. They
    // aren't specific to the std/json code, but putting them here is as good
    // as any other place.
    test_strconv_parse_number_u64,  //

    test_wuffs_json_decode_tokens,  //

#ifdef WUFFS_MIMIC

// No mimic tests.

#endif  // WUFFS_MIMIC

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

// No JSON benches.

#ifdef WUFFS_MIMIC

// No mimic benches.

#endif  // WUFFS_MIMIC

    NULL,
};

int main(int argc, char** argv) {
  proc_package_name = "std/json";
  return test_main(argc, argv, tests, benches);
}
