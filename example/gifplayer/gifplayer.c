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

/*
gifplayer prints an ASCII representation of the GIF image read from stdin. To
play Eadweard Muybridge's iconic galloping horse animation, run:

$CC gifplayer.c && ./a.out < ../../test/data/muybridge.gif; rm -f a.out

for a C compiler $CC, such as clang or gcc.

Add the -color flag to a.out to get 24 bit color ("true color") terminal output
(in the UTF-8 format) instead of plain ASCII output. Not all terminal emulators
support true color: https://gist.github.com/XVilka/8346728
*/

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef _POSIX_TIMERS
#error "TODO: timers on non-POSIX systems"
#else
#include <time.h>

bool started = false;
struct timespec start_time;

uint64_t micros_since_start(struct timespec* now) {
  if (!started) {
    return 0;
  }
  int64_t nanos = (int64_t)(now->tv_sec - start_time.tv_sec) * 1000000000 +
                  (int64_t)(now->tv_nsec - start_time.tv_nsec);
  if (nanos < 0) {
    return 0;
  }
  return nanos / 1000;
}
#endif

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../release/c/wuffs-unsupported-snapshot.h"

// Limit the input GIF image to (64 MiB - 1 byte) compressed and 4096 × 4096
// pixels uncompressed. This is a limitation of this example program (which
// uses the Wuffs standard library), not a limitation of Wuffs per se.
//
// We keep the whole input in memory, instead of one-pass stream processing,
// because playing a looping animation requires re-winding the input.
#define SRC_BUFFER_SIZE (64 * 1024 * 1024)
#define MAX_DIMENSION (4096)

uint8_t src_buffer[SRC_BUFFER_SIZE] = {0};
size_t src_len = 0;

wuffs_base__color_u32argb* dst_buffer = NULL;
wuffs_base__color_u32argb* prev_dst_buffer = NULL;
size_t dst_len;  // Length in bytes.
uint8_t* image_buffer = NULL;
size_t image_len = 0;

uint8_t* print_buffer = NULL;
size_t print_len = 0;

bool first_play = true;
uint32_t num_loops_remaining = 0;
wuffs_base__pixel_buffer pb = ((wuffs_base__pixel_buffer){});

wuffs_base__flicks cumulative_delay_micros = 0;

// ignore_return_value suppresses errors from -Wall -Werror.
static void ignore_return_value(int ignored) {}

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

// ----

// BYTES_PER_COLOR_PIXEL is long enough to contain "\x1B[38;2;255;255;255mABC"
// plus a trailing NUL byte and a few bytes of slack. It starts with a true
// color terminal escape code. ABC is three bytes for the UTF-8 representation
// "\xE2\x96\x88" of "█", U+2588 FULL BLOCK.
#define BYTES_PER_COLOR_PIXEL 32

const char* reset_color = "\x1B[0m";

bool color_flag = false;
const int stdout_fd = 1;

static inline uint32_t load_u32le(uint8_t* p) {
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16) | ((uint32_t)(p[3]) << 24);
}

void restore_background(wuffs_base__pixel_buffer* pb,
                        wuffs_base__rect_ie_u32 bounds) {
  size_t width = wuffs_base__pixel_config__width(&pb->pixcfg);
  size_t y;
  for (y = bounds.min_incl_y; y < bounds.max_excl_y; y++) {
    size_t x;
    wuffs_base__color_u32argb* d = dst_buffer + (y * width) + bounds.min_incl_x;
    for (x = bounds.min_incl_x; x < bounds.max_excl_x; x++) {
      *d++ = 0;
    }
  }
}

void compose(wuffs_base__pixel_buffer* pb, wuffs_base__rect_ie_u32 bounds) {
  wuffs_base__slice_u8 palette = wuffs_base__pixel_buffer__palette(pb);
  if (palette.len != 1024) {
    return;
  }
  size_t width = wuffs_base__pixel_config__width(&pb->pixcfg);
  size_t y;
  for (y = bounds.min_incl_y; y < bounds.max_excl_y; y++) {
    size_t x;
    wuffs_base__color_u32argb* d = dst_buffer + (y * width) + bounds.min_incl_x;
    uint8_t* s = image_buffer + (y * width) + bounds.min_incl_x;
    for (x = bounds.min_incl_x; x < bounds.max_excl_x; x++) {
      uint32_t index = *s++;
      wuffs_base__color_u32argb c = load_u32le(palette.ptr + 4 * index);
      if (c) {
        *d = c;
      }
      d++;
    }
  }
}

size_t print_ascii_art(wuffs_base__pixel_buffer* pb) {
  uint32_t width = wuffs_base__pixel_config__width(&pb->pixcfg);
  uint32_t height = wuffs_base__pixel_config__height(&pb->pixcfg);

  wuffs_base__color_u32argb* d = dst_buffer;
  uint8_t* p = print_buffer;
  *p++ = '\n';
  uint32_t y;
  for (y = 0; y < height; y++) {
    uint32_t x;
    for (x = 0; x < width; x++) {
      wuffs_base__color_u32argb c = *d++;
      // Convert to grayscale via the formula
      //  Y = (0.299 * R) + (0.587 * G) + (0.114 * B)
      // translated into fixed point arithmetic.
      uint32_t b = 0xFF & (c >> 0);
      uint32_t g = 0xFF & (c >> 8);
      uint32_t r = 0xFF & (c >> 16);
      uint32_t y = ((19595 * r) + (38470 * g) + (7471 * b) + (1 << 15)) >> 16;
      *p++ = "-:=+IOX@"[(y & 0xFF) >> 5];
    }
    *p++ = '\n';
  }
  return p - print_buffer;
}

size_t print_color_art(wuffs_base__pixel_buffer* pb) {
  uint32_t width = wuffs_base__pixel_config__width(&pb->pixcfg);
  uint32_t height = wuffs_base__pixel_config__height(&pb->pixcfg);

  wuffs_base__color_u32argb* d = dst_buffer;
  uint8_t* p = print_buffer;
  *p++ = '\n';
  p += sprintf((char*)p, "%s", reset_color);
  uint32_t y;
  for (y = 0; y < height; y++) {
    uint32_t x;
    for (x = 0; x < width; x++) {
      wuffs_base__color_u32argb c = *d++;
      int b = 0xFF & (c >> 0);
      int g = 0xFF & (c >> 8);
      int r = 0xFF & (c >> 16);
      // "\xE2\x96\x88" is U+2588 FULL BLOCK. Before that is a true color
      // terminal escape code.
      p += sprintf((char*)p, "\x1B[38;2;%d;%d;%dm\xE2\x96\x88", r, g, b);
    }
    *p++ = '\n';
  }
  p += sprintf((char*)p, "%s", reset_color);
  return p - print_buffer;
}

// ----

const char* allocate(wuffs_base__pixel_config* pc) {
  image_len = wuffs_base__pixel_config__pixbuf_size(pc);
  if (image_len > (SIZE_MAX / sizeof(wuffs_base__color_u32argb))) {
    return "could not allocate dst buffer";
  }

  dst_len = image_len * sizeof(wuffs_base__color_u32argb);
  dst_buffer = (wuffs_base__color_u32argb*)malloc(dst_len);
  if (!dst_buffer) {
    return "could not allocate dst buffer";
  }

  prev_dst_buffer = (wuffs_base__color_u32argb*)malloc(dst_len);
  if (!prev_dst_buffer) {
    free(dst_buffer);
    dst_buffer = NULL;
    return "could not allocate prev-dst buffer";
  }

  image_buffer = malloc(image_len);
  if (!image_buffer) {
    free(prev_dst_buffer);
    prev_dst_buffer = NULL;
    free(dst_buffer);
    dst_buffer = NULL;
    return "could not allocate image buffer";
  }

  uint32_t width = wuffs_base__pixel_config__width(pc);
  uint32_t height = wuffs_base__pixel_config__height(pc);
  uint64_t plen = 1 + ((uint64_t)(width) + 1) * (uint64_t)(height);
  uint64_t bytes_per_print_pixel = color_flag ? BYTES_PER_COLOR_PIXEL : 1;
  if (plen <= ((uint64_t)SIZE_MAX) / bytes_per_print_pixel) {
    print_len = (size_t)(plen * bytes_per_print_pixel);
    print_buffer = malloc(print_len);
  }
  if (!print_buffer) {
    free(image_buffer);
    image_buffer = NULL;
    free(prev_dst_buffer);
    prev_dst_buffer = NULL;
    free(dst_buffer);
    dst_buffer = NULL;
    return "could not allocate print buffer";
  }

  return NULL;
}

const char* play() {
  wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
  wuffs_base__status z =
      wuffs_gif__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);
  if (z) {
    return z;
  }

  wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
      .ptr = src_buffer, .len = src_len, .wi = src_len, .closed = true});
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);

  if (first_play) {
    first_play = false;
    wuffs_base__image_config ic = ((wuffs_base__image_config){});
    z = wuffs_gif__decoder__decode_image_config(&dec, &ic, src_reader);
    if (z) {
      return z;
    }
    if (!wuffs_base__image_config__is_valid(&ic)) {
      return "invalid image configuration";
    }
    uint32_t width = wuffs_base__pixel_config__width(&ic.pixcfg);
    uint32_t height = wuffs_base__pixel_config__height(&ic.pixcfg);
    if ((width > MAX_DIMENSION) || (height > MAX_DIMENSION)) {
      return "image dimensions are too large";
    }
    const char* msg = allocate(&ic.pixcfg);
    if (msg) {
      return msg;
    }
    z = wuffs_base__pixel_buffer__set_from_slice(
        &pb, &ic.pixcfg,
        ((wuffs_base__slice_u8){.ptr = image_buffer, .len = image_len}));
    if (z) {
      return z;
    }
    memset(image_buffer, 0, image_len);
    memset(dst_buffer, 0, dst_len);
    num_loops_remaining = wuffs_base__image_config__num_loops(&ic);
  }

  while (1) {
    wuffs_base__frame_config fc = ((wuffs_base__frame_config){});
    wuffs_base__status z =
        wuffs_gif__decoder__decode_frame_config(&dec, &fc, src_reader);
    if (z) {
      if (z == wuffs_base__suspension__end_of_data) {
        break;
      }
      return z;
    }

    switch (wuffs_base__frame_config__disposal(&fc)) {
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS: {
        memcpy(prev_dst_buffer, dst_buffer, dst_len);
        break;
      }
    }

    z = wuffs_gif__decoder__decode_frame(&dec, &pb, 0, 0, src_reader,
                                         ((wuffs_base__slice_u8){}));
    if (z) {
      if (z == wuffs_base__suspension__end_of_data) {
        break;
      }
      return z;
    }

    compose(&pb, wuffs_base__frame_config__bounds(&fc));

    size_t n = color_flag ? print_color_art(&pb) : print_ascii_art(&pb);

    switch (wuffs_base__frame_config__disposal(&fc)) {
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_BACKGROUND: {
        restore_background(&pb, wuffs_base__frame_config__bounds(&fc));
        break;
      }
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS: {
        wuffs_base__color_u32argb* swap = dst_buffer;
        dst_buffer = prev_dst_buffer;
        prev_dst_buffer = swap;
        break;
      }
    }

#ifdef _POSIX_TIMERS
    if (started) {
      struct timespec now;
      if (clock_gettime(CLOCK_MONOTONIC, &now)) {
        return strerror(errno);
      }
      uint64_t elapsed_micros = micros_since_start(&now);
      if (cumulative_delay_micros > elapsed_micros) {
        usleep(cumulative_delay_micros - elapsed_micros);
      }

    } else {
      if (clock_gettime(CLOCK_MONOTONIC, &start_time)) {
        return strerror(errno);
      }
      started = true;
    }
#endif

    ignore_return_value(write(stdout_fd, print_buffer, n));

    cumulative_delay_micros +=
        (1000 * wuffs_base__frame_config__duration(&fc)) /
        WUFFS_BASE__FLICKS_PER_MILLISECOND;

    // TODO: should a zero duration mean to show this frame forever?
  }
  return NULL;
}

int fail(const char* msg) {
  const int stderr_fd = 2;
  ignore_return_value(write(stderr_fd, msg, strnlen(msg, 4095)));
  ignore_return_value(write(stderr_fd, "\n", 1));
  return 1;
}

int main(int argc, char** argv) {
  int i;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-color")) {
      color_flag = true;
    }
  }

  const char* msg = read_stdin();
  if (msg) {
    return fail(msg);
  }
  while (true) {
    msg = play();
    if (msg) {
      return fail(msg);
    }
    if (num_loops_remaining == 0) {
      continue;
    }
    num_loops_remaining--;
    if (num_loops_remaining == 0) {
      break;
    }
  }
  return 0;
}
