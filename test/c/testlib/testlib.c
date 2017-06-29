// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

char fail_msg[65536] = {0};

#define FAIL(...) snprintf(fail_msg, sizeof(fail_msg), ##__VA_ARGS__)
#define INCR_FAIL(msg, ...) \
  msg += snprintf(msg, sizeof(fail_msg) - (msg - fail_msg), ##__VA_ARGS__)

int tests_run = 0;

typedef void (*proc)();

proc benches[];
proc tests[];

const char* proc_filename;
const char* proc_funcname = "";

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

bool bench_warm_up;
struct timeval bench_start_tv;

void bench_start() {
  gettimeofday(&bench_start_tv, NULL);
}

void bench_finish(uint64_t reps, uint64_t n_bytes) {
  struct timeval bench_finish_tv;
  gettimeofday(&bench_finish_tv, NULL);
  int64_t micros =
      (int64_t)(bench_finish_tv.tv_sec - bench_start_tv.tv_sec) * 1000000 +
      (int64_t)(bench_finish_tv.tv_usec - bench_start_tv.tv_usec);
  uint64_t nanos = 1;
  if (micros > 0) {
    nanos = (uint64_t)(micros)*1000;
  }
  uint64_t kb_per_s = n_bytes * 1000000 / nanos;
  if (bench_warm_up) {
    printf("# (warm up) %s_%s\t%8" PRIu64 ".%06" PRIu64 " seconds\n",
           proc_funcname, cc, nanos / 1000000000, (nanos % 1000000000) / 1000);
  } else {
    printf("Benchmark%s_%s\t%8" PRIu64 "\t%8" PRIu64 " ns/op\t%8d.%03d MB/s\n",
           proc_funcname, cc, reps, nanos / reps, (int)(kb_per_s / 1000),
           (int)(kb_per_s % 1000));
  }
}

int main(int argc, char** argv) {
  bool bench = false;
  int i;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-b")) {
      bench = true;
    }
  }

  int proc_reps = 1;
  proc* procs = tests;
  if (bench) {
    proc_reps = 5 + 1;  // +1 for the warm up run.
    procs = benches;
    printf(
        "# The output format, including the redundant \"Benchmark\"s, is "
        "compatible with\n# "
        "https://godoc.org/golang.org/x/perf/cmd/benchstat\n");
  }

  for (i = 0; i < proc_reps; i++) {
    bench_warm_up = i == 0;
    proc* p;
    for (p = procs; *p; p++) {
      proc_funcname = "unknown_funcname";
      (*p)();
      if (fail_msg[0]) {
        printf("%-16s%-8sFAIL %s: %s\n", proc_filename, cc, proc_funcname,
               fail_msg);
        return 1;
      }
      tests_run++;
    }
  }
  if (!bench) {
    printf("%-16s%-8sPASS (%d tests run)\n", proc_filename, cc, tests_run);
  }
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
  int j;
  for (j = -3 * 16; j <= +3 * 16; j += 16) {
    if ((j < 0) && (base < (size_t)(-j))) {
      continue;
    }
    size_t b = base + j;
    if (b >= buf->wi) {
      break;
    }
    size_t n = buf->wi - b;
    INCR_FAIL(msg, "  %06zx:", b);
    size_t k;
    for (k = 0; k < 16; k++) {
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
    for (k = 0; k < 16; k++) {
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
