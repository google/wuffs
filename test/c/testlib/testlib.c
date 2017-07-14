// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define BUFFER_SIZE (64 * 1024 * 1024)
#define PALETTE_BUFFER_SIZE (4 * 245)

uint8_t global_got_buffer[BUFFER_SIZE];
uint8_t global_want_buffer[BUFFER_SIZE];
uint8_t global_src_buffer[BUFFER_SIZE];
uint8_t global_palette_buffer[PALETTE_BUFFER_SIZE];

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

typedef struct {
  const char* want_filename;
  const char* src_filename;
  size_t src_offset0;
  size_t src_offset1;
} golden_test;

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

  const char* name = proc_funcname;
  if (!strncmp(name, "bench_", 6)) {
    name += 6;
  }
  if (bench_warm_up) {
    printf("# (warm up) %s/%s\t%8" PRIu64 ".%06" PRIu64 " seconds\n",  //
           name, cc, nanos / 1000000000, (nanos % 1000000000) / 1000);
  } else if (!n_bytes) {
    printf("Benchmark%s/%s\t%8" PRIu64 "\t%8" PRIu64 " ns/op\n",  //
           name, cc, reps, nanos / reps);
  } else {
    printf("Benchmark%s/%s\t%8" PRIu64 "\t%8" PRIu64
           " ns/op\t%8d.%03d MB/s\n",     //
           name, cc, reps, nanos / reps,  //
           (int)(kb_per_s / 1000), (int)(kb_per_s % 1000));
  }
}

int main(int argc, char** argv) {
  bool bench = false;
  int i;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-bench")) {
      bench = true;
    } else {
      fprintf(stderr, "unknown flag \"%s\"\n", argv[i]);
      return 1;
    }
  }

  int proc_reps = 1;
  proc* procs = tests;
  if (bench) {
    proc_reps = 5 + 1;  // +1 for the warm up run.
    procs = benches;
    printf(
        "# The output format, including the \"Benchmark\" prefixes, is "
        "compatible with the\n"
        "# https://godoc.org/golang.org/x/perf/cmd/benchstat tool. To install "
        "it, first\n"
        "# install Go, then run \"go get golang.org/x/perf/cmd/benchstat\".\n");
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
  size_t i;
  for (i = 0; (i < got->wi) && (i < want->wi); i++) {
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

void proc_buf1_buf1(const char* (*codec_func)(puffs_base_buf1*,
                                              puffs_base_buf1*),
                    golden_test* gt,
                    uint64_t reps,
                    bool bench) {
  if (!codec_func) {
    FAIL("NULL codec_func");
    return;
  }
  if (!gt) {
    FAIL("NULL golden_test");
    return;
  }

  puffs_base_buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};
  puffs_base_buf1 got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  puffs_base_buf1 want = {.ptr = global_want_buffer, .len = BUFFER_SIZE};

  if (!gt->src_filename) {
    src.closed = true;
  } else if (!read_file(&src, gt->src_filename)) {
    return;
  }
  if (gt->src_offset0 || gt->src_offset1) {
    if (gt->src_offset0 > gt->src_offset1) {
      FAIL("inconsistent src_offsets");
      return;
    }
    if (gt->src_offset1 > src.wi) {
      FAIL("src_offset1 too large");
      return;
    }
    src.ri = gt->src_offset0;
    src.wi = gt->src_offset1;
  }

  if (bench) {
    bench_start();
  }
  uint64_t n_bytes = 0;
  uint64_t i;
  for (i = 0; i < reps; i++) {
    got.wi = 0;
    src.ri = gt->src_offset0;
    const char* s = codec_func(&got, &src);
    if (s) {
      FAIL("%s", s);
      return;
    }
    n_bytes += got.wi;
  }
  if (bench) {
    bench_finish(reps, n_bytes);
    return;
  }

  if (!gt->want_filename) {
    want.closed = true;
  } else if (!read_file(&want, gt->want_filename)) {
    return;
  }
  if (!buf1s_equal("", &got, &want)) {
    return;
  }
}

void bench_buf1_buf1(const char* (*codec_func)(puffs_base_buf1*,
                                               puffs_base_buf1*),
                     golden_test* gt,
                     uint64_t reps) {
  proc_buf1_buf1(codec_func, gt, reps, true);
}

void test_buf1_buf1(const char* (*codec_func)(puffs_base_buf1*,
                                              puffs_base_buf1*),
                    golden_test* gt) {
  proc_buf1_buf1(codec_func, gt, 1, false);
}

#endif  // PUFFS_BASE_HEADER_H
