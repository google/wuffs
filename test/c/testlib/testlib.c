// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include <errno.h>
#include <stdio.h>
#include <string.h>

char fail_msg[65536] = {0};

#define FAIL(...) snprintf(fail_msg, sizeof(fail_msg), ##__VA_ARGS__)
#define INCR_FAIL(msg, ...) \
  msg += snprintf(msg, sizeof(fail_msg) - (msg - fail_msg), ##__VA_ARGS__)

int tests_run = 0;

typedef void (*test)();

test tests[];

const char* test_filename;
const char* test_funcname = "";

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
    test_funcname = "unknown_test_funcname";
    (*t)();
    if (fail_msg[0]) {
      printf("%-16s%-8sFAIL %s: %s\n", test_filename, cc, test_funcname,
             fail_msg);
      return 1;
    }
    tests_run++;
  }
  printf("%-16s%-8sPASS (%d tests run)\n", test_filename, cc, tests_run);
  return 0;
}

// PUFFS_BASE_HEADER_H is where puffs_base_buf1 is defined.
#ifdef PUFFS_BASE_HEADER_H

bool read_file(puffs_base_buf1* dst, const char* path) {
  if (!dst || !path) {
    FAIL("read_file: NULL argument");
    return false;
  }
  if (dst->closed) {
    FAIL("read_file: dst buffer closed for writes");
    return false;
  }
  FILE* f = fopen(path, "r");
  if (!f) {
    FAIL("read_file(\"%s\"): %s (errno=%d)", path, strerror(errno), errno);
    return false;
  }

  uint8_t dummy[1];
  uint8_t* ptr = dst->ptr + dst->wi;
  size_t len = dst->len - dst->wi;
  while (true) {
    if (!len) {
      // We have read all that dst can hold. Check that we have read the full
      // file by trying to read one more byte, which should fail with EOF.
      ptr = dummy;
      len = 1;
    }
    size_t n = fread(ptr, 1, len, f);
    if (ptr != dummy) {
      ptr += n;
      len -= n;
      dst->wi += n;
    } else if (n) {
      FAIL("read_file(\"%s\"): EOF not reached", path);
      fclose(f);
      return false;
    }
    if (feof(f)) {
      break;
    }
    int err = ferror(f);
    if (!err) {
      continue;
    }
    if (err == EINTR) {
      clearerr(f);
      continue;
    }
    FAIL("read_file(\"%s\"): %s (errno=%d)", path, strerror(err), err);
    fclose(f);
    return false;
  }
  fclose(f);
  dst->closed = true;
  return true;
}

char* hex_dump(char* msg, puffs_base_buf1* buf, size_t i) {
  if (!msg || !buf) {
    FAIL("hex_dump: NULL argument");
    return false;
  }
  if (buf->wi == 0) {
    return msg;
  }
  size_t base = i - (i & 15);
  for (int j = -3 * 16; j <= +3 * 16; j += 16) {
    if ((j < 0) && (base < (size_t)(-j))) {
      continue;
    }
    size_t b = base + j;
    if (b >= buf->wi) {
      break;
    }
    size_t n = buf->wi - b;
    INCR_FAIL(msg, "  %06zx:", b);
    for (size_t k = 0; k < 16; k++) {
      if (k % 2 == 0) {
        INCR_FAIL(msg, " ");
      }
      if (k < n) {
        INCR_FAIL(msg, "%02x", buf->ptr[b + k]);
      } else {
        INCR_FAIL(msg, "  ");
      }
    }
    INCR_FAIL(msg, "  ");
    for (size_t k = 0; k < 16; k++) {
      char c = ' ';
      if (k < n) {
        c = buf->ptr[b + k];
        if ((c < 0x20) || (0x7F < c)) {
          c = '.';
        }
      }
      INCR_FAIL(msg, "%c", c);
    }
    INCR_FAIL(msg, "\n");
    if (n < 16) {
      break;
    }
  }
  return msg;
}

bool buf1s_equal(const char* prefix,
                 puffs_base_buf1* got,
                 puffs_base_buf1* want) {
  if (!got || !want) {
    FAIL("%sbuf1s_equal: NULL argument", prefix);
    return false;
  }
  char* msg = fail_msg;
  size_t i = 0;
  for (; (i < got->wi) && (i < want->wi); i++) {
    if (got->ptr[i] != want->ptr[i]) {
      break;
    }
  }
  if (got->wi != want->wi) {
    INCR_FAIL(msg, "%sbuf1s_equal: wi: got %zu, want %zu.\n", prefix, got->wi,
              want->wi);
  } else if (i < got->wi) {
    INCR_FAIL(msg, "%sbuf1s_equal:\n", prefix);
  } else {
    return true;
  }
  INCR_FAIL(msg, "contents differ at byte %zu (in hex: 0x%06zx):\n", i, i);
  msg = hex_dump(msg, got, i);
  INCR_FAIL(msg, "excerpts of got (above) versus want (below):\n");
  msg = hex_dump(msg, want, i);
  return false;
}

#endif  // PUFFS_BASE_HEADER_H
