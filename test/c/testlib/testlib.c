// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include <stdio.h>

char fail_msg[65536] = {0};

#define FAIL(...) snprintf(fail_msg, sizeof fail_msg, ##__VA_ARGS__)

int tests_run = 0;

typedef void (*test)();

test tests[];

const char* test_filename;

int main(int argc, char** argv) {
// The order matters here. Clang also defines "__GNUC__".
#if defined(__clang__)
  const char* cc = "clang";
#elif defined(__GNUC__)
  const char* cc = "gcc";
#elif defined(_MSC_VER)
  const char* cc = "cl";
#else
  const char* cc = "cc";
#endif

  for (test* t = tests; *t; t++) {
    (*t)();
    if (fail_msg[0]) {
      printf("%-16s%-8sFAIL %s\n", test_filename, cc, fail_msg);
      return 1;
    }
    tests_run++;
  }
  printf("%-16s%-8sPASS (%d tests run)\n", test_filename, cc, tests_run);
  return 0;
}
