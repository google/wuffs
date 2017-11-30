// Copyright 2017 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define BUFFER_SIZE (64 * 1024 * 1024)
#define PALETTE_BUFFER_SIZE (4 * 245)

#define WUFFS_TESTLIB_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

uint8_t global_got_buffer[BUFFER_SIZE];
uint8_t global_want_buffer[BUFFER_SIZE];
uint8_t global_src_buffer[BUFFER_SIZE];
uint8_t global_palette_buffer[PALETTE_BUFFER_SIZE];

char fail_msg[65536] = {0};

#define FAIL(...) snprintf(fail_msg, sizeof(fail_msg), ##__VA_ARGS__)
#define INCR_FAIL(msg, ...) \
  msg += snprintf(msg, sizeof(fail_msg) - (msg - fail_msg), ##__VA_ARGS__)

int tests_run = 0;

const char* proc_filename = "";
const char* proc_funcname = "";
const char* focus = "";
bool in_focus = false;

#define CHECK_FOCUS(funcname) \
  proc_funcname = funcname;   \
  in_focus = check_focus();   \
  if (!in_focus) {            \
    return;                   \
  }

bool check_focus() {
  const char* p = focus;
  if (!*p) {
    return true;
  }
  size_t n = strlen(proc_funcname);

  // On each iteration of the loop, set p and q so that p (inclusive) and q
  // (exclusive) bracket the interesting fragment of the comma-separated
  // elements of focus.
  while (true) {
    const char* r = p;
    const char* q = NULL;
    while ((*r != ',') && (*r != '\x00')) {
      if ((*r == '/') && (q == NULL)) {
        q = r;
      }
      r++;
    }
    if (q == NULL) {
      q = r;
    }
    // At this point, r points to the comma or NUL byte that ends this element
    // of the comma-separated list. q points to the first slash in that
    // element, or if there are no slashes, q equals r.
    //
    // The pointers are named so that (p <= q) && (q <= r).

    if (p == q) {
      // No-op. Skip empty focus targets, which makes it convenient to
      // copy/paste a string with a trailing comma.
    } else {
      // Strip a leading "Benchmark", if present, from the [p, q) string.
      // Idiomatic C function names look like "test_wuffs_gif_lzw_decode_pi"
      // and won't start with "Benchmark". Stripping lets us conveniently
      // copy/paste a string like "Benchmarkwuffs_gif_decode_10k/gcc" from the
      // "wuffs bench std/gif" output.
      if ((q - p >= 9) && !strncmp(p, "Benchmark", 9)) {
        p += 9;
      }

      // See if proc_funcname (with or without a "test_" or "bench_" prefix)
      // starts with the [p, q) string.
      if ((n >= q - p) && !strncmp(proc_funcname, p, q - p)) {
        return true;
      }
      const char* unprefixed_proc_funcname = NULL;
      size_t unprefixed_n = 0;
      if ((n >= q - p) && !strncmp(proc_funcname, "test_", 5)) {
        unprefixed_proc_funcname = proc_funcname + 5;
        unprefixed_n = n - 5;
      } else if ((n >= q - p) && !strncmp(proc_funcname, "bench_", 6)) {
        unprefixed_proc_funcname = proc_funcname + 6;
        unprefixed_n = n - 6;
      }
      if (unprefixed_proc_funcname && (unprefixed_n >= q - p) &&
          !strncmp(unprefixed_proc_funcname, p, q - p)) {
        return true;
      }
    }

    if (*r == '\x00') {
      break;
    }
    p = r + 1;
  }
  return false;
}

// The order matters here. Clang also defines "__GNUC__".
#if defined(__clang__)
const char* cc = "clang";
const char* cc_version = __clang_version__;
#elif defined(__GNUC__)
const char* cc = "gcc";
const char* cc_version = __VERSION__;
#elif defined(_MSC_VER)
const char* cc = "cl";
const char* cc_version = "???";
#else
const char* cc = "cc";
const char* cc_version = "???";
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
  if ((strlen(name) >= 6) && !strncmp(name, "bench_", 6)) {
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
  // Flush stdout so that "wuffs bench | tee etc" still prints its numbers as
  // soon as they are available.
  fflush(stdout);
}

typedef void (*proc)();

int test_main(int argc, char** argv, proc* tests, proc* benches) {
  bool bench = false;
  int proc_reps = 5;

  int i;
  for (i = 1; i < argc; i++) {
    const char* arg = argv[i];
    size_t arg_len = strlen(arg);
    if (!strcmp(arg, "-bench")) {
      bench = true;

    } else if ((arg_len >= 7) && !strncmp(arg, "-focus=", 7)) {
      focus = arg + 7;

    } else if ((arg_len >= 6) && !strncmp(arg, "-reps=", 6)) {
      arg += 6;
      if (!*arg) {
        fprintf(stderr, "missing -reps=N value\n");
        return 1;
      }
      char* end = NULL;
      long int n = strtol(arg, &end, 10);
      if (*end) {
        fprintf(stderr, "invalid -reps=N value\n");
        return 1;
      }
      if ((n < 0) || (1000000 < n)) {
        fprintf(stderr, "out-of-range -reps=N value\n");
        return 1;
      }
      proc_reps = n;

    } else {
      fprintf(stderr, "unknown flag \"%s\"\n", arg);
      return 1;
    }
  }

  proc* procs = tests;
  if (!bench) {
    proc_reps = 1;
  } else {
    proc_reps++;  // +1 for the warm up run.
    procs = benches;
    printf("# %s version %s\n#\n", cc, cc_version);
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
      in_focus = false;
      (*p)();
      if (!in_focus) {
        continue;
      }
      if (fail_msg[0]) {
        printf("%-16s%-8sFAIL %s: %s\n", proc_filename, cc, proc_funcname,
               fail_msg);
        return 1;
      }
      if (i == 0) {
        tests_run++;
      }
    }
  }
  if (bench) {
    printf("# %-16s%-8s(%d benchmarks run, 1+%d reps per benchmark)\n",
           proc_filename, cc, tests_run, proc_reps - 1);
  } else {
    printf("%-16s%-8sPASS (%d tests run)\n", proc_filename, cc, tests_run);
  }
  return 0;
}

// WUFFS_BASE_HEADER_H is where wuffs_base__buf1 is defined.
#ifdef WUFFS_BASE_HEADER_H

bool read_file(wuffs_base__buf1* dst, const char* path) {
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

char* hex_dump(char* msg, wuffs_base__buf1* buf, size_t i) {
  if (!msg || !buf) {
    FAIL("hex_dump: NULL argument");
    return NULL;
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
        if ((c < 0x20) || (0x7F <= c)) {
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
                 wuffs_base__buf1* got,
                 wuffs_base__buf1* want) {
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

// throughput_counter is whether to count dst or src bytes, or neither, when
// calculating a benchmark's MB/s throughput number.
//
// Decoders typically use tc_dst. Encoders and hashes typically use tc_src.
typedef enum {
  tc_neither = 0,
  tc_dst = 1,
  tc_src = 2,
} throughput_counter;

bool proc_buf1_buf1(const char* (*codec_func)(wuffs_base__buf1*,
                                              wuffs_base__buf1*,
                                              uint64_t,
                                              uint64_t),
                    throughput_counter tc,
                    golden_test* gt,
                    uint64_t wlimit,
                    uint64_t rlimit,
                    uint64_t reps,
                    bool bench) {
  if (!codec_func) {
    FAIL("NULL codec_func");
    return false;
  }
  if (!gt) {
    FAIL("NULL golden_test");
    return false;
  }

  wuffs_base__buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};
  wuffs_base__buf1 got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  wuffs_base__buf1 want = {.ptr = global_want_buffer, .len = BUFFER_SIZE};

  if (!gt->src_filename) {
    src.closed = true;
  } else if (!read_file(&src, gt->src_filename)) {
    return false;
  }
  if (gt->src_offset0 || gt->src_offset1) {
    if (gt->src_offset0 > gt->src_offset1) {
      FAIL("inconsistent src_offsets");
      return false;
    }
    if (gt->src_offset1 > src.wi) {
      FAIL("src_offset1 too large");
      return false;
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
    const char* s = codec_func(&got, &src, wlimit, rlimit);
    if (s) {
      FAIL("%s", s);
      return false;
    }
    switch (tc) {
      case tc_neither:
        break;
      case tc_dst:
        n_bytes += got.wi;
        break;
      case tc_src:
        n_bytes += src.ri - gt->src_offset0;
        break;
    }
  }
  if (bench) {
    bench_finish(reps, n_bytes);
    return true;
  }

  if (!gt->want_filename) {
    want.closed = true;
  } else if (!read_file(&want, gt->want_filename)) {
    return false;
  }
  if (!buf1s_equal("", &got, &want)) {
    return false;
  }
  return true;
}

bool do_bench_buf1_buf1(const char* (*codec_func)(wuffs_base__buf1*,
                                                  wuffs_base__buf1*,
                                                  uint64_t,
                                                  uint64_t),
                        throughput_counter tc,
                        golden_test* gt,
                        uint64_t wlimit,
                        uint64_t rlimit,
                        uint64_t reps) {
  return proc_buf1_buf1(codec_func, tc, gt, wlimit, rlimit, reps, true);
}

bool do_test_buf1_buf1(const char* (*codec_func)(wuffs_base__buf1*,
                                                 wuffs_base__buf1*,
                                                 uint64_t,
                                                 uint64_t),
                       golden_test* gt,
                       uint64_t wlimit,
                       uint64_t rlimit) {
  return proc_buf1_buf1(codec_func, tc_neither, gt, wlimit, rlimit, 1, false);
}

#endif  // WUFFS_BASE_HEADER_H
