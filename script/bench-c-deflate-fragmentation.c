// Copyright 2018 The Wuffs Authors.
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

// This file contains a hand-written C benchmark of different strategies for
// decoding PNG data.
//
// For a PNG image with width W and height H, the H rows can be decompressed
// one-at-a-time or all-at-once. Roughly speaking, this corresponds to H versus
// 1 call into the zlib decoder. The former (call it "fragmented dst") requires
// less scratch-space memory than the latter ("full dst"): 2 * bytes_per_row
// instead of H * bytes_per row, but the latter can be faster.
//
// The zlib-compressed data can be split into multiple IDAT chunks. Similarly,
// these chunks can be decompressed separately ("fragmented IDAT") or together
// ("full IDAT"), again providing a memory vs speed trade-off.
//
// This program reports the speed of combining the independent frag/full dst
// and frag/full IDAT techniques.
//
// For example, with gcc 7.3 (and -O3) as of April 2018:
//
// On ../test/data/hat.png (90 × 112 pixels):
// name                 time/op     relative
// FragDstFragIDAT/gcc  203µs ± 1%  1.00x
// FragDstFullIDAT/gcc  203µs ± 0%  1.00x
// FullDstFragIDAT/gcc  170µs ± 0%  1.19x
// FullDstFullIDAT/gcc  147µs ± 0%  1.38x
//
// On ../test/data/hibiscus.png (312 × 442 pixels):
// name                 time/op      relative
// FragDstFragIDAT/gcc  2.62ms ± 1%  1.00x
// FragDstFullIDAT/gcc  2.61ms ± 1%  1.00x
// FullDstFragIDAT/gcc  2.44ms ± 0%  1.07x
// FullDstFullIDAT/gcc  2.03ms ± 1%  1.29x
//
// On ../test/data/harvesters.png (1165 × 859 pixels):
// name                 time/op      relative
// FragDstFragIDAT/gcc  18.3ms ± 0%  1.00x
// FragDstFullIDAT/gcc  18.3ms ± 1%  1.00x
// FullDstFragIDAT/gcc  17.1ms ± 0%  1.07x
// FullDstFullIDAT/gcc  13.9ms ± 0%  1.32x

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../release/c/wuffs-unsupported-snapshot.h"

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

static inline uint32_t load_u32be(uint8_t* p) {
  return ((uint32_t)(p[0]) << 24) | ((uint32_t)(p[1]) << 16) |
         ((uint32_t)(p[2]) << 8) | ((uint32_t)(p[3]) << 0);
}

// Limit the input PNG image (and therefore its IDAT data) to (64 MiB - 1 byte)
// compressed, in up to 1024 IDAT chunks, and 256 MiB and 16384 × 16384 pixels
// uncompressed. This is a limitation of this program (which uses the Wuffs
// standard library), not a limitation of Wuffs per se.
#define DST_BUFFER_SIZE (256 * 1024 * 1024)
#define SRC_BUFFER_SIZE (64 * 1024 * 1024)
#define MAX_DIMENSION (16384)
#define MAX_IDAT_CHUNKS (1024)

uint8_t dst_buffer[DST_BUFFER_SIZE] = {0};
size_t dst_len = 0;
uint8_t src_buffer[SRC_BUFFER_SIZE] = {0};
size_t src_len = 0;
uint8_t idat_buffer[SRC_BUFFER_SIZE] = {0};
// The n'th IDAT chunk data (where n is a zero-based count) is in
// idat_buffer[i:j], where i = idat_splits[n+0] and j = idat_splits[n+1].
size_t idat_splits[MAX_IDAT_CHUNKS + 1] = {0};
uint32_t num_idat_chunks = 0;

uint32_t width = 0;
uint32_t height = 0;
uint64_t bytes_per_pixel = 0;
uint64_t bytes_per_row = 0;
uint64_t bytes_per_frame = 0;

const char* read_stdin() {
  while (src_len < SRC_BUFFER_SIZE) {
    const int stdin_fd = 0;
    ssize_t n = read(stdin_fd, src_buffer + src_len, SRC_BUFFER_SIZE - src_len);
    if (n > 0) {
      src_len += n;
    } else if (n == 0) {
      return NULL;
    } else if (errno == EINTR) {
      // No-op.
    } else {
      return strerror(errno);
    }
  }
  return "input is too large";
}

const char* process_png_chunks(uint8_t* p, size_t n) {
  while (n > 0) {
    // Process the 8 byte chunk header.
    if (n < 8) {
      return "invalid PNG chunk";
    }
    uint32_t chunk_len = load_u32be(p + 0);
    uint32_t chunk_type = load_u32be(p + 4);
    p += 8;
    n -= 8;

    // Process the chunk payload.
    if (n < chunk_len) {
      return "short PNG chunk data";
    }
    switch (chunk_type) {
      case 0x49484452:  // "IHDR"
        if (chunk_len != 13) {
          return "invalid PNG IDAT chunk";
        }
        width = load_u32be(p + 0);
        height = load_u32be(p + 4);
        if ((width == 0) || (height == 0)) {
          return "image dimensions are too small";
        }
        if ((width > MAX_DIMENSION) || (height > MAX_DIMENSION)) {
          return "image dimensions are too large";
        }
        if (p[8] != 8) {
          return "unsupported PNG bit depth";
        }
        if (bytes_per_pixel != 0) {
          return "duplicate PNG IHDR chunk";
        }
        // Process the color type, as per the PNG spec table 11.1.
        switch (p[9]) {
          case 0:
            bytes_per_pixel = 1;
            break;
          case 2:
            bytes_per_pixel = 3;
            break;
          case 3:
            bytes_per_pixel = 1;
            break;
          case 4:
            bytes_per_pixel = 2;
            break;
          case 6:
            bytes_per_pixel = 4;
            break;
          default:
            return "unsupported PNG color type";
        }
        if (p[12] != 0) {
          return "unsupported PNG interlacing";
        }
        break;

      case 0x49444154:  // "IDAT"
        if (num_idat_chunks == MAX_IDAT_CHUNKS - 1) {
          return "too many IDAT chunks";
        }
        memcpy(idat_buffer + idat_splits[num_idat_chunks], p, chunk_len);
        idat_splits[num_idat_chunks + 1] =
            idat_splits[num_idat_chunks] + chunk_len;
        num_idat_chunks++;
        break;
    }
    p += chunk_len;
    n -= chunk_len;

    // Process (and ignore) the 4 byte chunk footer (a checksum).
    if (n < 4) {
      return "invalid PNG chunk";
    }
    p += 4;
    n -= 4;
  }
  return NULL;
}

const char* decode_once(bool frag_dst, bool frag_idat) {
  wuffs_zlib__decoder dec = ((wuffs_zlib__decoder){});
  wuffs_zlib__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);

  wuffs_base__io_buffer dst = {.ptr = dst_buffer, .len = bytes_per_frame};
  wuffs_base__io_buffer idat = {.ptr = idat_buffer,
                                .len = SRC_BUFFER_SIZE,
                                .wi = idat_splits[num_idat_chunks],
                                .closed = true};
  wuffs_base__io_writer dst_writer = wuffs_base__io_buffer__writer(&dst);
  wuffs_base__io_reader idat_reader = wuffs_base__io_buffer__reader(&idat);

  uint32_t i = 0;  // Number of dst fragments processed, if frag_dst.
  if (frag_dst) {
    dst.len = bytes_per_row;
  }

  uint32_t j = 0;  // Number of IDAT fragments processed, if frag_idat.
  if (frag_idat) {
    idat.wi = idat_splits[1];
    idat.closed = (num_idat_chunks == 1);
  }

  while (true) {
    wuffs_base__status s =
        wuffs_zlib__decoder__decode(&dec, dst_writer, idat_reader);

    if (s == WUFFS_BASE__STATUS_OK) {
      break;
    }
    if ((s == WUFFS_BASE__SUSPENSION_SHORT_WRITE) && frag_dst &&
        (i < height - 1)) {
      i++;
      dst.len = bytes_per_row * (i + 1);
      continue;
    }
    if ((s == WUFFS_BASE__SUSPENSION_SHORT_READ) && frag_idat &&
        (j < num_idat_chunks - 1)) {
      j++;
      idat.wi = idat_splits[j + 1];
      idat.closed = (num_idat_chunks == j + 1);
      continue;
    }
    return wuffs_zlib__status__string(s);
  }

  if (dst.wi != bytes_per_frame) {
    return "unexpected number of bytes decoded";
  }
  return NULL;
}

const char* decode(bool frag_dst, bool frag_idat) {
  int reps;
  if (bytes_per_frame < 100000) {
    reps = 1000;
  } else if (bytes_per_frame < 1000000) {
    reps = 100;
  } else if (bytes_per_frame < 10000000) {
    reps = 10;
  } else {
    reps = 1;
  }

  struct timeval bench_start_tv;
  gettimeofday(&bench_start_tv, NULL);

  int i;
  for (i = 0; i < reps; i++) {
    const char* msg = decode_once(frag_dst, frag_idat);
    if (msg) {
      return msg;
    }
  }

  struct timeval bench_finish_tv;
  gettimeofday(&bench_finish_tv, NULL);
  int64_t micros =
      (int64_t)(bench_finish_tv.tv_sec - bench_start_tv.tv_sec) * 1000000 +
      (int64_t)(bench_finish_tv.tv_usec - bench_start_tv.tv_usec);
  uint64_t nanos = 1;
  if (micros > 0) {
    nanos = (uint64_t)(micros)*1000;
  }

  printf("Benchmark%sDst%sIDAT/%s\t%8d\t%8" PRIu64 " ns/op\n",
         frag_dst ? "Frag" : "Full",   //
         frag_idat ? "Frag" : "Full",  //
         cc, reps, nanos / reps);

  return NULL;
}

int fail(const char* msg) {
  const int stderr_fd = 2;
  write(stderr_fd, msg, strnlen(msg, 4095));
  write(stderr_fd, "\n", 1);
  return 1;
}

int main(int argc, char** argv) {
  const char* msg = read_stdin();
  if (msg) {
    return fail(msg);
  }
  if ((src_len < 8) || strncmp(src_buffer, "\x89PNG\x0D\x0A\x1A\x0A", 8)) {
    return fail("invalid PNG");
  }
  msg = process_png_chunks(src_buffer + 8, src_len - 8);
  if (msg) {
    return fail(msg);
  }
  if (bytes_per_pixel == 0) {
    return fail("missing PNG IHDR chunk");
  }
  if (num_idat_chunks == 0) {
    return fail("missing PNG IDAT chunk");
  }
  // The +1 here is for the per-row filter byte.
  bytes_per_row = (uint64_t)width * bytes_per_pixel + 1;
  bytes_per_frame = (uint64_t)height * bytes_per_row;
  if (bytes_per_frame > DST_BUFFER_SIZE) {
    return fail("decompressed data is too large");
  }

  printf("# %s version %s\n#\n", cc, cc_version);
  printf(
      "# The output format, including the \"Benchmark\" prefixes, is "
      "compatible with the\n"
      "# https://godoc.org/golang.org/x/perf/cmd/benchstat tool. To install "
      "it, first\n"
      "# install Go, then run \"go get golang.org/x/perf/cmd/benchstat\".\n");

  int i;
  for (i = 0; i < 5; i++) {
    msg = decode(true, true);
    if (msg) {
      return fail(msg);
    }
    msg = decode(true, false);
    if (msg) {
      return fail(msg);
    }
    msg = decode(false, true);
    if (msg) {
      return fail(msg);
    }
    msg = decode(false, false);
    if (msg) {
      return fail(msg);
    }
  }

  return 0;
}
