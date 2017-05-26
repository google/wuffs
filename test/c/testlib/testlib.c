// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include <stdio.h>

char fail_msg[65536] = {0};

#define FAIL(...) snprintf(fail_msg, sizeof fail_msg, ##__VA_ARGS__)

int tests_run = 0;

typedef void (*test)();

test tests[];

int main(int argc, char** argv) {
  for (test* t = tests; *t; t++) {
    (*t)();
    if (fail_msg[0]) {
      printf("FAIL %s\n", fail_msg);
      return 1;
    }
    tests_run++;
  }
  printf("PASS (%d tests run)\n", tests_run);
  return 0;
}
