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
#include <unistd.h>

#define BUFFER_SIZE (64 * 1024 * 1024)

#define WUFFS_TESTLIB_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

uint8_t global_got_array[BUFFER_SIZE];
uint8_t global_want_array[BUFFER_SIZE];
uint8_t global_work_array[BUFFER_SIZE];
uint8_t global_src_array[BUFFER_SIZE];
uint8_t global_pixel_array[BUFFER_SIZE];

wuffs_base__slice_u8 global_got_slice = ((wuffs_base__slice_u8){
    .ptr = global_got_array,
    .len = BUFFER_SIZE,
});

wuffs_base__slice_u8 global_want_slice = ((wuffs_base__slice_u8){
    .ptr = global_want_array,
    .len = BUFFER_SIZE,
});

wuffs_base__slice_u8 global_work_slice = ((wuffs_base__slice_u8){
    .ptr = global_work_array,
    .len = BUFFER_SIZE,
});

wuffs_base__slice_u8 global_src_slice = ((wuffs_base__slice_u8){
    .ptr = global_src_array,
    .len = BUFFER_SIZE,
});

wuffs_base__slice_u8 global_pixel_slice = ((wuffs_base__slice_u8){
    .ptr = global_pixel_array,
    .len = BUFFER_SIZE,
});

char fail_msg[65536] = {0};

#define RETURN_FAIL(...)                               \
  snprintf(fail_msg, sizeof(fail_msg), ##__VA_ARGS__); \
  return fail_msg
#define INCR_FAIL(msg, ...) \
  msg += snprintf(msg, sizeof(fail_msg) - (msg - fail_msg), ##__VA_ARGS__)

int tests_run = 0;

uint64_t iterscale = 100;

const char* proc_package_name = "unknown_package_name";
const char* proc_func_name = "unknown_func_name";
const char* focus = "";
bool in_focus = false;

#define CHECK_FOCUS(func_name) \
  proc_func_name = func_name;  \
  in_focus = check_focus();    \
  if (!in_focus) {             \
    return NULL;               \
  }

bool check_focus() {
  const char* p = focus;
  if (!*p) {
    return true;
  }
  size_t n = strlen(proc_func_name);

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

      // See if proc_func_name (with or without a "test_" or "bench_" prefix)
      // starts with the [p, q) string.
      if ((n >= q - p) && !strncmp(proc_func_name, p, q - p)) {
        return true;
      }
      const char* unprefixed_proc_func_name = NULL;
      size_t unprefixed_n = 0;
      if ((n >= q - p) && !strncmp(proc_func_name, "test_", 5)) {
        unprefixed_proc_func_name = proc_func_name + 5;
        unprefixed_n = n - 5;
      } else if ((n >= q - p) && !strncmp(proc_func_name, "bench_", 6)) {
        unprefixed_proc_func_name = proc_func_name + 6;
        unprefixed_n = n - 6;
      }
      if (unprefixed_proc_func_name && (unprefixed_n >= q - p) &&
          !strncmp(unprefixed_proc_func_name, p, q - p)) {
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

// https://www.guyrutenberg.com/2008/12/20/expanding-macros-into-string-constants-in-c/
#define WUFFS_TESTLIB_QUOTE_EXPAND(x) #x
#define WUFFS_TESTLIB_QUOTE(x) WUFFS_TESTLIB_QUOTE_EXPAND(x)

// The order matters here. Clang also defines "__GNUC__".
#if defined(__clang__)
const char* cc = "clang" WUFFS_TESTLIB_QUOTE(__clang_major__);
const char* cc_version = __clang_version__;
#elif defined(__GNUC__)
const char* cc = "gcc" WUFFS_TESTLIB_QUOTE(__GNUC__);
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

void bench_finish(uint64_t iters, uint64_t n_bytes) {
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

  const char* name = proc_func_name;
  if ((strlen(name) >= 6) && !strncmp(name, "bench_", 6)) {
    name += 6;
  }
  if (bench_warm_up) {
    printf("# (warm up) %s/%s\t%8" PRIu64 ".%06" PRIu64 " seconds\n",  //
           name, cc, nanos / 1000000000, (nanos % 1000000000) / 1000);
  } else if (!n_bytes) {
    printf("Benchmark%s/%s\t%8" PRIu64 "\t%8" PRIu64 " ns/op\n",  //
           name, cc, iters, nanos / iters);
  } else {
    printf("Benchmark%s/%s\t%8" PRIu64 "\t%8" PRIu64
           " ns/op\t%8d.%03d MB/s\n",       //
           name, cc, iters, nanos / iters,  //
           (int)(kb_per_s / 1000), (int)(kb_per_s % 1000));
  }
  // Flush stdout so that "wuffs bench | tee etc" still prints its numbers as
  // soon as they are available.
  fflush(stdout);
}

const char* chdir_to_the_wuffs_root_directory() {
  // Chdir to the Wuffs root directory, assuming that we're starting from
  // somewhere in the Wuffs repository, so we can find the root directory by
  // running chdir("..") a number of times.
  int n;
  for (n = 0; n < 64; n++) {
    if (access("wuffs-root-directory.txt", F_OK) == 0) {
      return NULL;
    }

    // If we're at the root "/", chdir("..") won't change anything.
    char cwd_buffer[4];
    char* cwd = getcwd(cwd_buffer, 4);
    if ((cwd != NULL) && (cwd[0] == '/') && (cwd[1] == '\x00')) {
      break;
    }

    if (chdir("..")) {
      return "could not chdir(\"..\")";
    }
  }
  return "could not find Wuffs root directory; chdir there before running this "
         "program";
}

typedef const char* (*proc)();

int test_main(int argc, char** argv, proc* tests, proc* benches) {
  const char* status = chdir_to_the_wuffs_root_directory();
  if (status) {
    fprintf(stderr, "%s\n", status);
    return 1;
  }

  bool bench = false;
  int reps = 5;

  int i;
  for (i = 1; i < argc; i++) {
    const char* arg = argv[i];
    size_t arg_len = strlen(arg);
    if (!strcmp(arg, "-bench")) {
      bench = true;

    } else if ((arg_len >= 7) && !strncmp(arg, "-focus=", 7)) {
      focus = arg + 7;

    } else if ((arg_len >= 11) && !strncmp(arg, "-iterscale=", 11)) {
      arg += 11;
      if (!*arg) {
        fprintf(stderr, "missing -iterscale=N value\n");
        return 1;
      }
      char* end = NULL;
      long int n = strtol(arg, &end, 10);
      if (*end) {
        fprintf(stderr, "invalid -iterscale=N value\n");
        return 1;
      }
      if ((n < 0) || (1000000 < n)) {
        fprintf(stderr, "out-of-range -iterscale=N value\n");
        return 1;
      }
      iterscale = n;

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
      reps = n;

    } else {
      fprintf(stderr, "unknown flag \"%s\"\n", arg);
      return 1;
    }
  }

  proc* procs = tests;
  if (!bench) {
    reps = 1;
  } else {
    reps++;  // +1 for the warm up run.
    procs = benches;
    printf("# %s\n# %s version %s\n#\n", proc_package_name, cc, cc_version);
    printf(
        "# The output format, including the \"Benchmark\" prefixes, is "
        "compatible with the\n"
        "# https://godoc.org/golang.org/x/perf/cmd/benchstat tool. To install "
        "it, first\n"
        "# install Go, then run \"go get golang.org/x/perf/cmd/benchstat\".\n");
  }

  for (i = 0; i < reps; i++) {
    bench_warm_up = i == 0;
    proc* p;
    for (p = procs; *p; p++) {
      proc_func_name = "unknown_func_name";
      fail_msg[0] = 0;
      in_focus = false;
      const char* status = (*p)();
      if (!in_focus) {
        continue;
      }
      if (status) {
        printf("%-16s%-8sFAIL %s: %s\n", proc_package_name, cc, proc_func_name,
               status);
        return 1;
      }
      if (i == 0) {
        tests_run++;
      }
    }
    if (i != 0) {
      continue;
    }
    if (bench) {
      printf("# %d benchmarks, 1+%d reps per benchmark, iterscale=%d\n",
             tests_run, reps - 1, (int)(iterscale));
    } else {
      printf("%-16s%-8sPASS (%d tests)\n", proc_package_name, cc, tests_run);
    }
  }
  return 0;
}

// WUFFS_INCLUDE_GUARD is where wuffs_base__foo_bar are defined.
#ifdef WUFFS_INCLUDE_GUARD

wuffs_base__rect_ie_u32 make_rect_ie_u32(uint32_t x0,
                                         uint32_t y0,
                                         uint32_t x1,
                                         uint32_t y1) {
  wuffs_base__rect_ie_u32 ret;
  ret.min_incl_x = x0;
  ret.min_incl_y = y0;
  ret.max_excl_x = x1;
  ret.max_excl_y = y1;
  return ret;
}

void set_reader_limit(wuffs_base__io_reader* o, uint64_t limit) {
  if (o && o->private_impl.buf) {
    uint8_t* p = o->private_impl.buf->data.ptr + o->private_impl.buf->meta.ri;
    uint8_t* q = o->private_impl.buf->data.ptr + o->private_impl.buf->meta.wi;
    if (!o->private_impl.mark) {
      o->private_impl.mark = p;
      o->private_impl.limit = q;
    }
    if ((o->private_impl.limit - p) > limit) {
      o->private_impl.limit = p + limit;
    }
  }
}

void set_writer_limit(wuffs_base__io_writer* o, uint64_t limit) {
  if (o && o->private_impl.buf) {
    uint8_t* p = o->private_impl.buf->data.ptr + o->private_impl.buf->meta.wi;
    uint8_t* q = o->private_impl.buf->data.ptr + o->private_impl.buf->data.len;
    if (!o->private_impl.mark) {
      o->private_impl.mark = p;
      o->private_impl.limit = q;
    }
    if ((o->private_impl.limit - p) > limit) {
      o->private_impl.limit = p + limit;
    }
  }
}

// TODO: we shouldn't need to pass the rect. Instead, pass a subset pixbuf.
const char* copy_to_io_buffer_from_pixel_buffer(wuffs_base__io_buffer* dst,
                                                wuffs_base__pixel_buffer* src,
                                                wuffs_base__rect_ie_u32 r) {
  if (!src) {
    return "copy_to_io_buffer_from_pixel_buffer: NULL src";
  }

  wuffs_base__pixel_format pixfmt =
      wuffs_base__pixel_config__pixel_format(&src->pixcfg);
  if (wuffs_base__pixel_format__is_planar(pixfmt)) {
    // If we want to support planar pixel buffers, in the future, be concious
    // of pixel subsampling.
    return "copy_to_io_buffer_from_pixel_buffer: cannot copy from planar src";
  }
  uint32_t bits_per_pixel = wuffs_base__pixel_format__bits_per_pixel(pixfmt);
  if (bits_per_pixel == 0) {
    return "copy_to_io_buffer_from_pixel_buffer: invalid bits_per_pixel";
  } else if ((bits_per_pixel % 8) != 0) {
    return "copy_to_io_buffer_from_pixel_buffer: cannot copy fractional bytes";
  }
  size_t bytes_per_pixel = bits_per_pixel / 8;

  uint32_t p;
  for (p = 0; p < 1; p++) {
    wuffs_base__table_u8 tab = wuffs_base__pixel_buffer__plane(src, p);
    uint32_t y;
    for (y = r.min_incl_y; y < r.max_excl_y; y++) {
      wuffs_base__slice_u8 row = wuffs_base__table_u8__row(tab, y);
      if ((r.min_incl_x >= r.max_excl_x) ||
          (r.max_excl_x > (row.len / bytes_per_pixel))) {
        break;
      }

      size_t n = r.max_excl_x - r.min_incl_x;
      if (n > (SIZE_MAX / bytes_per_pixel)) {
        return "copy_to_io_buffer_from_pixel_buffer: n is too large";
      }
      n *= bytes_per_pixel;

      if (n > (dst->data.len - dst->meta.wi)) {
        return "copy_to_io_buffer_from_pixel_buffer: dst buffer is too small";
      }
      memmove(dst->data.ptr + dst->meta.wi, row.ptr + r.min_incl_x, n);
      dst->meta.wi += n;
    }
  }
  return NULL;
}

const char* read_file(wuffs_base__io_buffer* dst, const char* path) {
  if (!dst || !path) {
    RETURN_FAIL("read_file: NULL argument");
  }
  if (dst->meta.closed) {
    RETURN_FAIL("read_file: dst buffer closed for writes");
  }
  FILE* f = fopen(path, "r");
  if (!f) {
    RETURN_FAIL("read_file(\"%s\"): %s (errno=%d)", path, strerror(errno),
                errno);
  }

  uint8_t dummy[1];
  uint8_t* ptr = dst->data.ptr + dst->meta.wi;
  size_t len = dst->data.len - dst->meta.wi;
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
      dst->meta.wi += n;
    } else if (n) {
      fclose(f);
      RETURN_FAIL("read_file(\"%s\"): EOF not reached", path);
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
    fclose(f);
    RETURN_FAIL("read_file(\"%s\"): %s (errno=%d)", path, strerror(err), err);
  }
  fclose(f);
  dst->meta.pos = 0;
  dst->meta.closed = true;
  return NULL;
}

char* hex_dump(char* msg, wuffs_base__io_buffer* buf, size_t i) {
  if (!msg || !buf) {
    RETURN_FAIL("hex_dump: NULL argument");
  }
  if (buf->meta.wi == 0) {
    return msg;
  }
  size_t base = i - (i & 15);
  int j;
  for (j = -3 * 16; j <= +3 * 16; j += 16) {
    if ((j < 0) && (base < (size_t)(-j))) {
      continue;
    }
    size_t b = base + j;
    if (b >= buf->meta.wi) {
      break;
    }
    size_t n = buf->meta.wi - b;
    INCR_FAIL(msg, "  %06zx:", b);
    size_t k;
    for (k = 0; k < 16; k++) {
      if (k % 2 == 0) {
        INCR_FAIL(msg, " ");
      }
      if (k < n) {
        INCR_FAIL(msg, "%02x", buf->data.ptr[b + k]);
      } else {
        INCR_FAIL(msg, "  ");
      }
    }
    INCR_FAIL(msg, "  ");
    for (k = 0; k < 16; k++) {
      char c = ' ';
      if (k < n) {
        c = buf->data.ptr[b + k];
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

const char* check_io_buffers_equal(const char* prefix,
                                   wuffs_base__io_buffer* got,
                                   wuffs_base__io_buffer* want) {
  if (!got || !want) {
    RETURN_FAIL("%sio_buffers_equal: NULL argument", prefix);
  }
  char* msg = fail_msg;
  size_t i;
  size_t n = got->meta.wi < want->meta.wi ? got->meta.wi : want->meta.wi;
  for (i = 0; i < n; i++) {
    if (got->data.ptr[i] != want->data.ptr[i]) {
      break;
    }
  }
  if (got->meta.wi != want->meta.wi) {
    INCR_FAIL(msg, "%sio_buffers_equal: wi: got %zu, want %zu.\n", prefix,
              got->meta.wi, want->meta.wi);
  } else if (i < got->meta.wi) {
    INCR_FAIL(msg, "%sio_buffers_equal: wi=%zu:\n", prefix, n);
  } else {
    return NULL;
  }
  INCR_FAIL(msg, "contents differ at byte %zu (in hex: 0x%06zx):\n", i, i);
  msg = hex_dump(msg, got, i);
  INCR_FAIL(msg, "excerpts of got (above) versus want (below):\n");
  msg = hex_dump(msg, want, i);
  return fail_msg;
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

const char* proc_io_buffers(const char* (*codec_func)(wuffs_base__io_buffer*,
                                                      wuffs_base__io_buffer*,
                                                      uint32_t,
                                                      uint64_t,
                                                      uint64_t),
                            uint32_t wuffs_initialize_flags,
                            throughput_counter tc,
                            golden_test* gt,
                            uint64_t wlimit,
                            uint64_t rlimit,
                            uint64_t iters,
                            bool bench) {
  if (!codec_func) {
    RETURN_FAIL("NULL codec_func");
  }
  if (!gt) {
    RETURN_FAIL("NULL golden_test");
  }

  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .data = global_src_slice,
  });
  wuffs_base__io_buffer got = ((wuffs_base__io_buffer){
      .data = global_got_slice,
  });
  wuffs_base__io_buffer want = ((wuffs_base__io_buffer){
      .data = global_want_slice,
  });

  if (!gt->src_filename) {
    src.meta.closed = true;
  } else {
    const char* status = read_file(&src, gt->src_filename);
    if (status) {
      return status;
    }
  }
  if (gt->src_offset0 || gt->src_offset1) {
    if (gt->src_offset0 > gt->src_offset1) {
      RETURN_FAIL("inconsistent src_offsets");
    }
    if (gt->src_offset1 > src.meta.wi) {
      RETURN_FAIL("src_offset1 too large");
    }
    src.meta.ri = gt->src_offset0;
    src.meta.wi = gt->src_offset1;
  }

  if (bench) {
    bench_start();
  }
  uint64_t n_bytes = 0;
  uint64_t i;
  for (i = 0; i < iters; i++) {
    got.meta.wi = 0;
    src.meta.ri = gt->src_offset0;
    const char* status =
        codec_func(&got, &src, wuffs_initialize_flags, wlimit, rlimit);
    if (status) {
      return status;
    }
    switch (tc) {
      case tc_neither:
        break;
      case tc_dst:
        n_bytes += got.meta.wi;
        break;
      case tc_src:
        n_bytes += src.meta.ri - gt->src_offset0;
        break;
    }
  }
  if (bench) {
    bench_finish(iters, n_bytes);
    return NULL;
  }

  if (!gt->want_filename) {
    want.meta.closed = true;
  } else {
    const char* status = read_file(&want, gt->want_filename);
    if (status) {
      return status;
    }
  }
  return check_io_buffers_equal("", &got, &want);
}

const char* do_bench_io_buffers(
    const char* (*codec_func)(wuffs_base__io_buffer*,
                              wuffs_base__io_buffer*,
                              uint32_t,
                              uint64_t,
                              uint64_t),
    uint32_t wuffs_initialize_flags,
    throughput_counter tc,
    golden_test* gt,
    uint64_t wlimit,
    uint64_t rlimit,
    uint64_t iters_unscaled) {
  return proc_io_buffers(codec_func, wuffs_initialize_flags, tc, gt, wlimit,
                         rlimit, iters_unscaled * iterscale, true);
}

const char* do_test_io_buffers(const char* (*codec_func)(wuffs_base__io_buffer*,
                                                         wuffs_base__io_buffer*,
                                                         uint32_t,
                                                         uint64_t,
                                                         uint64_t),
                               golden_test* gt,
                               uint64_t wlimit,
                               uint64_t rlimit) {
  return proc_io_buffers(codec_func,
                         WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
                         tc_neither, gt, wlimit, rlimit, 1, false);
}

#endif  // WUFFS_INCLUDE_GUARD
