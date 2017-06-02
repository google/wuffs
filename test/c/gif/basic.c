// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

/*
To manually run this test:

for cc in clang gcc; do
  $cc -std=c99 -Wall -Werror basic.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).
*/

#include "../../../gen/c/gif.c"
#include "../testlib/testlib.c"

const char* test_filename = "gif/basic.c";

void test_constructor_not_called() {
  puffs_gif_lzw_decoder dec;
  puffs_base_buf1 dst = {0};
  puffs_base_buf1 src = {0};
  puffs_gif_status status =
      puffs_gif_lzw_decoder_decode(&dec, &dst, &src, false);
  if (status != puffs_gif_error_constructor_not_called) {
    FAIL("test_constructor_not_called: status: got %d, want %d", status,
         puffs_gif_error_constructor_not_called);
  }
}

void test_bad_argument() {
  puffs_gif_lzw_decoder dec;
  puffs_gif_lzw_decoder_constructor(&dec, PUFFS_VERSION, 0);
  puffs_gif_status status =
      puffs_gif_lzw_decoder_decode(&dec, NULL, NULL, false);
  if (status != puffs_gif_error_bad_argument) {
    FAIL("test_bad_argument: status: got %d, want %d", status,
         puffs_gif_error_bad_argument);
  }
}

void test_bad_receiver() {
  puffs_base_buf1 dst = {0};
  puffs_base_buf1 src = {0};
  puffs_gif_status status =
      puffs_gif_lzw_decoder_decode(NULL, &dst, &src, false);
  if (status != puffs_gif_error_bad_receiver) {
    FAIL("test_bad_receiver: status: got %d, want %d", status,
         puffs_gif_error_bad_receiver);
  }
}

void test_puffs_version_bad() {
  puffs_gif_lzw_decoder dec;
  puffs_gif_lzw_decoder_constructor(&dec, 0, 0);  // 0 is not PUFFS_VERSION.
  if (dec.status != puffs_gif_error_bad_version) {
    FAIL("test_puffs_version_bad: status: got %d, want %d", dec.status,
         puffs_gif_error_bad_version);
    goto cleanup0;
  }
cleanup0:
  puffs_gif_lzw_decoder_destructor(&dec);
}

void test_puffs_version_good() {
  puffs_gif_lzw_decoder dec;
  puffs_gif_lzw_decoder_constructor(&dec, PUFFS_VERSION, 0);
  if (dec.magic != PUFFS_MAGIC) {
    FAIL("test_puffs_version_good: magic: got %u, want %u", dec.magic,
         PUFFS_MAGIC);
    goto cleanup0;
  }
  if (dec.f_literal_width != 8) {
    FAIL("test_puffs_version_good: f_literal_width: got %u, want %u",
         dec.f_literal_width, 8);
    goto cleanup0;
  }
cleanup0:
  puffs_gif_lzw_decoder_destructor(&dec);
}

void test_status_is_error() {
  if (puffs_gif_status_is_error(puffs_gif_status_ok)) {
    FAIL("test_status_is_error: is_error(ok) returned true");
    return;
  }
  if (!puffs_gif_status_is_error(puffs_gif_error_bad_version)) {
    FAIL("test_status_is_error: is_error(bad_version) returned false");
    return;
  }
  if (puffs_gif_status_is_error(puffs_gif_status_short_dst)) {
    FAIL("test_status_is_error: is_error(short_dst) returned true");
    return;
  }
}

void test_status_strings() {
  const char* s0 = puffs_gif_status_string(puffs_gif_status_ok);
  if (strcmp(s0, "gif: ok")) {
    FAIL("test_status_strings: got \"%s\", want \"gif: ok\"", s0);
    return;
  }
  const char* s1 = puffs_gif_status_string(puffs_gif_error_bad_version);
  if (strcmp(s1, "gif: bad version")) {
    FAIL("test_status_strings: got \"%s\", want \"gif: bad version\"", s1);
    return;
  }
  const char* s2 = puffs_gif_status_string(puffs_gif_status_short_dst);
  if (strcmp(s2, "gif: short dst")) {
    FAIL("test_status_strings: got \"%s\", want \"gif: short dst\"", s2);
    return;
  }
}

// The empty comments forces clang-format to place one element per line.
test tests[] = {
    test_constructor_not_called,  //
    test_bad_argument,            //
    test_bad_receiver,            //
    test_puffs_version_bad,       //
    test_puffs_version_good,      //
    test_status_is_error,         //
    test_status_strings,          //
    NULL,                         //
};
