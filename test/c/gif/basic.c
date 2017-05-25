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
clang:  PASS
gcc:    PASS
*/

#include <stdio.h>

#include "../../../gen/c/gif.c"

int main(int argc, char** argv) {
  printf("PASS\n");
  return 0;
}
