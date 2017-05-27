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

void test_null_receiver() {
  puffs_gif_status status = puffs_gif_lzw_decoder_decode(NULL);
  if (status != puffs_gif_error_null_receiver) {
    FAIL("test_null_receiver: status: got %d, want %d", status,
         puffs_gif_error_null_receiver);
  }
}

void test_puffs_version_bad() {
  puffs_gif_lzw_decoder dec;
  puffs_gif_lzw_decoder_constructor(&dec, 0);  // 0 is not PUFFS_VERSION.
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
  puffs_gif_lzw_decoder_constructor(&dec, PUFFS_VERSION);
  if (dec.magic != PUFFS_MAGIC) {
    FAIL("test_puffs_version_good: magic: got %u, want %u", dec.magic,
         PUFFS_MAGIC);
    goto cleanup0;
  }
  if (dec.f_literal_width != 0) {
    FAIL("test_puffs_version_good: f_literal_width: got %u, want %u",
         dec.f_literal_width, 0);
    goto cleanup0;
  }
cleanup0:
  puffs_gif_lzw_decoder_destructor(&dec);
}

// The empty comments forces clang-format to place one element per line.
test tests[] = {
    test_null_receiver,       //
    test_puffs_version_bad,   //
    test_puffs_version_good,  //
    NULL,                     //
};
