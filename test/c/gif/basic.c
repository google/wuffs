// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

/*
To manually run this test:

for cc in clang gcc; do
  printf "%-8s" "$cc:"
  $cc -std=c99 -Wall -Werror basic.c && ./a.out
  rm -f a.out
done

It should print:
clang:  PASS (N tests run)
gcc:    PASS (N tests run)
for some value of N.
*/

#include "../testlib/testlib.c"

#include "../../../gen/c/gif.c"

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
