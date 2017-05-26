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

#include "../testlib/testlib.c"

#include "../../../gen/c/gif.c"

const char* test_filename = "gif/basic.c";

void test_default_literal_width() {
  puffs_gif_lzw_decoder dec;
  puffs_gif_lzw_decoder_constructor(&dec);
  if (dec.f_literal_width != 0) {
    FAIL("test_default_literal_width: got %d, want %d", dec.f_literal_width, 0);
    puffs_gif_lzw_decoder_destructor(&dec);
    return;
  }
  puffs_gif_lzw_decoder_destructor(&dec);
}

test tests[] = {
    test_default_literal_width, NULL,
};
