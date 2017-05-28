// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

/*
To manually run this test:

for cc in clang gcc; do
  $cc -std=c99 -Wall -Werror lzw.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).
*/

#include "../../../gen/c/gif.c"
#include "../testlib/testlib.c"

const char* test_filename = "gif/lzw.c";

void test_lzw_decode() {
  puffs_gif_lzw_decoder dec;
  puffs_gif_lzw_decoder_constructor(&dec, PUFFS_VERSION, 0);
  puffs_gif_status status = puffs_gif_lzw_decoder_decode(&dec);
  if (status != puffs_gif_status_ok) {
    FAIL("test_lzw_decode: status: got %d, want %d", status,
         puffs_gif_status_ok);
    goto cleanup0;
  }
cleanup0:
  puffs_gif_lzw_decoder_destructor(&dec);
}

// The empty comments forces clang-format to place one element per line.
test tests[] = {
    test_lzw_decode,  //
    NULL,             //
};
