// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include <errno.h>
#include <stdio.h>
#include <string.h>

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

#ifdef PUFFS_BASE_HEADER_H
// PUFFS_BASE_HEADER_H is where puffs_base_buf1 is defined.
bool read_file(puffs_base_buf1* dst, const char* path) {
  FILE* f = fopen(path, "r");
  if (!f) {
    FAIL("read_file(\"%s\"): %s (errno=%d)", path, strerror(errno), errno);
    return false;
  }
  uint8_t* ptr = dst->ptr + dst->wi;
  size_t len = dst->len - dst->wi;
  while (true) {
    size_t n = fread(ptr, 1, len, f);
    ptr += n;
    len -= n;
    dst->wi += n;
    if (feof(f)) {
      break;
    }
    int err = ferror(f);
    if (err == EINTR) {
      clearerr(f);
      continue;
    }
    if (err) {
      FAIL("read_file(\"%s\"): %s (errno=%d)", path, strerror(err), err);
    } else {
      FAIL("read_file(\"%s\"): EOF not reached", path);
    }
    fclose(f);
    return false;
  }
  fclose(f);
  return true;
}
#endif  // PUFFS_BASE_HEADER_H
