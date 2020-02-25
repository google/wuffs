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

const char*  //
test_strconv_parse_number_i64() {
  CHECK_FOCUS(__func__);

  const int64_t fail = 0xDEADBEEF;

  struct {
    int64_t want;
    const char* str;
  } test_cases[] = {
      {.want = +0x0000000000000000, .str = "+0"},
      {.want = +0x0000000000000000, .str = "-0"},
      {.want = +0x0000000000000000, .str = "0"},
      {.want = +0x000000000000012C, .str = "+300"},
      {.want = +0x7FFFFFFFFFFFFFFF, .str = "+9223372036854775807"},
      {.want = +0x7FFFFFFFFFFFFFFF, .str = "9223372036854775807"},
      {.want = -0x0000000000000002, .str = "-2"},
      {.want = -0x00000000000000AB, .str = "_-_0x_AB"},
      {.want = -0x7FFFFFFFFFFFFFFF, .str = "-9223372036854775807"},
      {.want = -0x8000000000000000, .str = "-9223372036854775808"},

      {.want = fail, .str = "+ 1"},
      {.want = fail, .str = "++1"},
      {.want = fail, .str = "+-1"},
      {.want = fail, .str = "+9223372036854775808"},  // 1 << 63.
      {.want = fail, .str = "-"},
      {.want = fail, .str = "-+1"},
      {.want = fail, .str = "-0x8000000000000001"},   // -((1 << 63) + 1).
      {.want = fail, .str = "-9223372036854775809"},  // -((1 << 63) + 1).
      {.want = fail, .str = "0x8000000000000000"},    // 1 << 63.
      {.want = fail, .str = "1-"},
      {.want = fail, .str = "9223372036854775808"},  // 1 << 63.
  };

  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    wuffs_base__result_i64 r =
        wuffs_base__parse_number_i64(wuffs_base__make_slice_u8(
            (void*)test_cases[tc].str, strlen(test_cases[tc].str)));
    int64_t have = (r.status.repr == NULL) ? r.value : fail;
    if (have != test_cases[tc].want) {
      RETURN_FAIL("\"%s\": have 0x%" PRIX64 ", want 0x%" PRIX64,
                  test_cases[tc].str, have, test_cases[tc].want);
    }
  }

  return NULL;
}

const char*  //
test_strconv_parse_number_u64() {
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

  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    wuffs_base__result_u64 r =
        wuffs_base__parse_number_u64(wuffs_base__make_slice_u8(
            (void*)test_cases[tc].str, strlen(test_cases[tc].str)));
    uint64_t have = (r.status.repr == NULL) ? r.value : fail;
    if (have != test_cases[tc].want) {
      RETURN_FAIL("\"%s\": have 0x%" PRIX64 ", want 0x%" PRIX64,
                  test_cases[tc].str, have, test_cases[tc].want);
    }
  }

  return NULL;
}

// ---------------- Golden Tests

golden_test json_australian_abc_gt = {
    .src_filename = "test/data/australian-abc-local-stations.json",  //
};

golden_test json_file_sizes_gt = {
    .src_filename = "test/data/file-sizes.json",  //
};

golden_test json_github_tags_gt = {
    .src_filename = "test/data/github-tags.json",  //
};

golden_test json_nobel_prizes_gt = {
    .src_filename = "test/data/nobel-prizes.json",  //
};

// ---------------- JSON Tests

const char*  //
test_wuffs_json_decode_tokens() {
  CHECK_FOCUS(__func__);
  wuffs_json__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_json__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  const uint64_t vbc = WUFFS_BASE__TOKEN__VBC__STRUCTURE;
  const uint64_t vbd = WUFFS_BASE__TOKEN__VBD__STRUCTURE__POP |
                       WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_LIST |
                       WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_NONE;
  const uint64_t len = 1;

  return do_test__wuffs_base__token_decoder(
      wuffs_json__decoder__upcast_as__wuffs_base__token_decoder(&dec),
      "test/data/github-tags.json", 0, SIZE_MAX,
      (vbc << WUFFS_BASE__TOKEN__VALUE_BASE_CATEGORY__SHIFT) |
          (vbd << WUFFS_BASE__TOKEN__VALUE_BASE_DETAIL__SHIFT) |
          (len << WUFFS_BASE__TOKEN__LENGTH__SHIFT));
}

const char*  //
wuffs_json_decode(wuffs_base__token_buffer* tok,
                  wuffs_base__io_buffer* src,
                  uint32_t wuffs_initialize_flags,
                  uint64_t wlimit,
                  uint64_t rlimit) {
  wuffs_json__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_json__decoder__initialize(&dec, sizeof dec, WUFFS_VERSION,
                                               wuffs_initialize_flags));

  while (true) {
    wuffs_base__token_buffer limited_tok =
        make_limited_token_writer(*tok, wlimit);
    wuffs_base__io_buffer limited_src = make_limited_reader(*src, rlimit);

    wuffs_base__status status =
        wuffs_json__decoder__decode_tokens(&dec, &limited_tok, &limited_src);

    tok->meta.wi += limited_tok.meta.wi;
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
test_wuffs_json_decode_unicode4_escapes() {
  CHECK_FOCUS(__func__);

  const uint32_t fail = 0xDEADBEEF;

  struct {
    uint32_t want;
    const char* str;
  } test_cases[] = {
      // Simple (non-surrogate) successes.
      {.want = 0x0000000A, .str = "\"\\u000a\""},
      {.want = 0x0000005C, .str = "\"\\\\u1234\""},  // U+005C is '\\'.
      {.want = 0x00001000, .str = "\"\\u10002345\""},
      {.want = 0x00001000, .str = "\"\\u1000234\""},
      {.want = 0x00001000, .str = "\"\\u100023\""},
      {.want = 0x00001000, .str = "\"\\u10002\""},
      {.want = 0x00001234, .str = "\"\\u1234\""},
      {.want = 0x0000D7FF, .str = "\"\\ud7ff\""},
      {.want = 0x0000E000, .str = "\"\\uE000\""},
      {.want = 0x0000FFFF, .str = "\"\\uFffF\""},

      // Unicode surrogate pair. U+0001F4A9 PILE OF POO is (U+D83D, U+DCA9),
      // because ((0x03D << 10) | 0x0A9) is 0xF4A9:
      //  - High surrogates are in the range U+D800 ..= U+DBFF.
      //  - Low  surrogates are in the range U+DC00 ..= U+DFFF.
      {.want = 0x0001F4A9, .str = "\"\\uD83D\\udca9\""},

      // More surrogate pairs.
      {.want = 0x00010000, .str = "\"\\uD800\\uDC00\""},
      {.want = 0x0010FFFF, .str = "\"\\uDBFF\\uDFFF\""},

      // Simple (non-surrogate) failures.
      {.want = fail, .str = "\"\\U1234\""},
      {.want = fail, .str = "\"\\u123"},
      {.want = fail, .str = "\"\\u123\""},
      {.want = fail, .str = "\"\\u123x\""},
      {.want = fail, .str = "\"u1234\""},

      // Invalid surrogate pairs.
      {.want = fail, .str = "\"\\uD800\""},         // High alone.
      {.want = fail, .str = "\"\\uD83D?udca9\""},   // High then not "\\u".
      {.want = fail, .str = "\"\\uD83D\\ud7ff\""},  // High then non-surrogate.
      {.want = fail, .str = "\"\\uD83D\\udbff\""},  // High then high.
      {.want = fail, .str = "\"\\uD83D\\ue000\""},  // High then non-surrogate.
      {.want = fail, .str = "\"\\uDC00\""},         // Low alone.
      {.want = fail, .str = "\"\\uDC00\\u0000\""},  // Low then non-surrogate.
      {.want = fail, .str = "\"\\uDC00\\ud800\""},  // Low then high.
      {.want = fail, .str = "\"\\uDC00\\udfff\""},  // Low then low.
      {.want = fail, .str = "\"\\uDFFF1234\""},     // Low alone.
  };

  wuffs_json__decoder dec;
  int tc;
  for (tc = 0; tc < WUFFS_TESTLIB_ARRAY_SIZE(test_cases); tc++) {
    CHECK_STATUS("initialize",
                 wuffs_json__decoder__initialize(
                     &dec, sizeof dec, WUFFS_VERSION,
                     WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

    wuffs_base__token_buffer tok = ((wuffs_base__token_buffer){
        .data = global_have_token_slice,
    });
    size_t n = strlen(test_cases[tc].str);
    wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
        .data = wuffs_base__make_slice_u8((void*)(test_cases[tc].str), n),
        .meta = wuffs_base__make_io_buffer_meta(n, 0, 0, true),
    });

    wuffs_json__decoder__decode_tokens(&dec, &tok, &src);

    uint32_t have = fail;
    uint64_t total_length = 0;
    size_t i;
    for (i = tok.meta.ri; i < tok.meta.wi; i++) {
      wuffs_base__token* t = &tok.data.ptr[i];
      total_length =
          wuffs_base__u64__sat_add(total_length, wuffs_base__token__length(t));

      // Set have to the first Unicode code point token.
      if ((have == fail) && ((wuffs_base__token__value_base_category(t) ==
                              WUFFS_BASE__TOKEN__VBC__UNICODE_CODE_POINT))) {
        have = wuffs_base__token__value_base_detail(t);
        if (have > 0x10FFFF) {  // This also catches "have == fail".
          RETURN_FAIL("%s: invalid Unicode code point", test_cases[tc].str);
        }

        uint64_t have_length = wuffs_base__token__length(t);
        uint64_t want_length = (have == 0x5C) ? 2 : ((have <= 0xFFFF) ? 6 : 12);
        if (have_length != want_length) {
          RETURN_FAIL("%s: token length: have %" PRIu64 ", want %" PRIu64,
                      test_cases[tc].str, have_length, want_length);
        }
      }
    }

    if (have != test_cases[tc].want) {
      RETURN_FAIL("%s: have 0x%" PRIX32 ", want 0x%" PRIX32, test_cases[tc].str,
                  have, test_cases[tc].want);
    }

    if (total_length != src.meta.ri) {
      RETURN_FAIL("%s: total length: have 0x%" PRIu64 ", want 0x%" PRIu64,
                  test_cases[tc].str, total_length, src.meta.ri);
    }
  }

  return NULL;
}

  // ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

  // No mimic tests.

#endif  // WUFFS_MIMIC

// ---------------- JSON Benches

const char*  //
bench_wuffs_json_decode_1k() {
  CHECK_FOCUS(__func__);
  return do_bench_token_decoder(
      wuffs_json_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      tcounter_src, &json_github_tags_gt, UINT64_MAX, UINT64_MAX, 10000);
}

const char*  //
bench_wuffs_json_decode_21k_formatted() {
  CHECK_FOCUS(__func__);
  return do_bench_token_decoder(
      wuffs_json_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      tcounter_src, &json_file_sizes_gt, UINT64_MAX, UINT64_MAX, 300);
}

const char*  //
bench_wuffs_json_decode_26k_compact() {
  CHECK_FOCUS(__func__);
  return do_bench_token_decoder(
      wuffs_json_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      tcounter_src, &json_australian_abc_gt, UINT64_MAX, UINT64_MAX, 250);
}

const char*  //
bench_wuffs_json_decode_217k_stringy() {
  CHECK_FOCUS(__func__);
  return do_bench_token_decoder(
      wuffs_json_decode, WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      tcounter_src, &json_nobel_prizes_gt, UINT64_MAX, UINT64_MAX, 20);
}

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
    test_strconv_parse_number_i64,  //
    test_strconv_parse_number_u64,  //

    test_wuffs_json_decode_tokens,            //
    test_wuffs_json_decode_unicode4_escapes,  //

#ifdef WUFFS_MIMIC

// No mimic tests.

#endif  // WUFFS_MIMIC

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

    bench_wuffs_json_decode_1k,             //
    bench_wuffs_json_decode_21k_formatted,  //
    bench_wuffs_json_decode_26k_compact,    //
    bench_wuffs_json_decode_217k_stringy,   //

#ifdef WUFFS_MIMIC

// No mimic benches.

#endif  // WUFFS_MIMIC

    NULL,
};

int  //
main(int argc, char** argv) {
  proc_package_name = "std/json";
  return test_main(argc, argv, tests, benches);
}
