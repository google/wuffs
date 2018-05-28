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

$cc gifplayer.c && ./a.out < ../../test/data/muybridge.gif; rm -f a.out

for a C compiler $cc, such as clang or gcc.

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

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../gen/c/std/gif.c"

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
uint8_t* dst_buffer = NULL;
size_t dst_len = 0;
uint8_t* print_buffer = NULL;
size_t print_len = 0;

bool seen_num_loops = false;
uint32_t num_loops_remaining = 0;

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

// BYTES_PER_COLOR_PIXEL is long enough to contain "?\x1B[38;2;255;255;255mABC"
// plus a few bytes of slack. The first byte is the overall length - a Pascal
// style string. After that is a true color terminal escape code. ABC is three
// bytes for the UTF-8 representation "\xE2\x96\x88" of "█", U+2588 FULL BLOCK.
#define BYTES_PER_COLOR_PIXEL 32

const char* full_block_pascal_string = "\x03\xE2\x96\x88";
const char* reset_color_pascal_string = "\x04\x1B[0m";

uint8_t* append_pascal_string(uint8_t* dst, const char* s) {
  uint8_t len = *s++;
  while (len--) {
    *dst++ = *s++;
  }
  return dst;
}

bool color_flag = false;
const int stdout_fd = 1;

char palette_as_ascii_art[256];
char palette_as_color_art[256][BYTES_PER_COLOR_PIXEL];  // Pascal style strings.

void reset_palette_as_ascii_art() {
  memset(palette_as_ascii_art, '-', 256);
}

void reset_palette_as_color_art() {
  memset(palette_as_color_art, 0, 256 * BYTES_PER_COLOR_PIXEL);
}

// update_palette_as_ascii_art calculates a grayscale value and therefore an
// ASCII art character for each (red, green, blue) palette entry.
void update_palette_as_ascii_art(wuffs_base__slice_u8 palette) {
  if (!palette.ptr || (palette.len != 256 * 4)) {
    reset_palette_as_ascii_art();
    return;
  }

  uint32_t i;
  for (i = 0; i < 256; i++) {
    // Convert to grayscale via the formula
    //  Y = (0.299 * R) + (0.587 * G) + (0.114 * B)
    // translated into fixed point arithmetic.
    uint32_t b = palette.ptr[4 * i + 0];
    uint32_t g = palette.ptr[4 * i + 1];
    uint32_t r = palette.ptr[4 * i + 2];
    uint32_t y = ((19595 * r) + (38470 * g) + (7471 * b) + (1 << 15)) >> 16;
    palette_as_ascii_art[i] = "-:=+IOX@"[(y & 0xFF) >> 5];
  }
}

// update_palette_as_color_art calculates a terminal escape code for each (red,
// green, blue) palette entry.
void update_palette_as_color_art(wuffs_base__slice_u8 palette) {
  if (!palette.ptr || (palette.len != 256 * 4)) {
    reset_palette_as_color_art();
    return;
  }

  uint32_t i;
  for (i = 0; i < 256; i++) {
    int b = palette.ptr[4 * i + 0];
    int g = palette.ptr[4 * i + 1];
    int r = palette.ptr[4 * i + 2];
    palette_as_color_art[i][0] =
        sprintf(palette_as_color_art[i] + 1, "\x1B[38;2;%d;%d;%dm", r, g, b);
  }
}

void print_ascii_art(wuffs_base__image_buffer* ib) {
  wuffs_base__image_config* ic = wuffs_base__image_buffer__image_config(ib);
  uint32_t width = wuffs_base__image_config__width(ic);
  uint32_t height = wuffs_base__image_config__height(ic);

  uint8_t* d = dst_buffer;
  uint8_t* p = print_buffer;
  *p++ = '\n';
  uint32_t y;
  for (y = 0; y < height; y++) {
    uint32_t x;
    for (x = 0; x < width; x++) {
      *p++ = palette_as_ascii_art[*d++];
    }
    *p++ = '\n';
  }
  ignore_return_value(write(stdout_fd, print_buffer, p - print_buffer));
}

void print_color_art(wuffs_base__image_buffer* ib) {
  wuffs_base__image_config* ic = wuffs_base__image_buffer__image_config(ib);
  uint32_t width = wuffs_base__image_config__width(ic);
  uint32_t height = wuffs_base__image_config__height(ic);

  int32_t previous_palette_index = -1;
  uint8_t* d = dst_buffer;
  uint8_t* p = print_buffer;
  *p++ = '\n';
  p = append_pascal_string(p, reset_color_pascal_string);
  uint32_t y;
  for (y = 0; y < height; y++) {
    uint32_t x;
    for (x = 0; x < width; x++) {
      uint8_t palette_index = *d++;
      if (palette_index != previous_palette_index) {
        p = append_pascal_string(p, palette_as_color_art[palette_index]);
        previous_palette_index = palette_index;
      }
      p = append_pascal_string(p, full_block_pascal_string);
    }
    *p++ = '\n';
  }
  p = append_pascal_string(p, reset_color_pascal_string);

  ignore_return_value(write(stdout_fd, print_buffer, p - print_buffer));
}

// ----

const char* play() {
  if (color_flag) {
    reset_palette_as_color_art();
  } else {
    reset_palette_as_ascii_art();
  }

  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);

  wuffs_base__io_buffer src = {
      .ptr = src_buffer, .len = src_len, .wi = src_len, .closed = true};
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);

  wuffs_base__image_buffer ib = ((wuffs_base__image_buffer){});
  wuffs_base__image_config ic = {{0}};
  wuffs_gif__status s =
      wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
  if (s) {
    return wuffs_gif__status__string(s);
  }
  if (!wuffs_base__image_config__is_valid(&ic)) {
    return "invalid image configuration";
  }
  uint32_t width = wuffs_base__image_config__width(&ic);
  uint32_t height = wuffs_base__image_config__height(&ic);
  if ((width > MAX_DIMENSION) || (height > MAX_DIMENSION)) {
    return "image dimensions are too large";
  }
  if (!dst_buffer) {
    dst_len = wuffs_base__image_config__pixbuf_size(&ic);
    dst_buffer = malloc(dst_len);
    if (!dst_buffer) {
      return "could not allocate dst buffer";
    }
    uint64_t plen = 1 + ((uint64_t)(width) + 1) * (uint64_t)(height);
    uint64_t bytes_per_print_pixel = color_flag ? BYTES_PER_COLOR_PIXEL : 1;
    if (plen <= ((uint64_t)SIZE_MAX) / bytes_per_print_pixel) {
      print_len = (size_t)(plen * bytes_per_print_pixel);
      print_buffer = malloc(print_len);
    }
    if (!print_buffer) {
      return "could not allocate print buffer";
    }
  }
  // TODO: check wuffs_base__image_buffer__set_from_slice errors?
  wuffs_base__image_buffer__set_from_slice(
      &ib, ic, ((wuffs_base__slice_u8){.ptr = dst_buffer, .len = dst_len}));

  if (!seen_num_loops) {
    seen_num_loops = true;
    // TODO: provide API for getting num_loops.
    num_loops_remaining = dec.private_impl.f_num_loops;
  }

  bool first_frame = true;
  for (;; first_frame = false) {
    s = wuffs_gif__decoder__decode_frame(&dec, &ib, src_reader);
    if (s) {
      if (s == WUFFS_GIF__SUSPENSION_END_OF_DATA) {
        break;
      }
      return wuffs_gif__status__string(s);
    }

    if (wuffs_base__image_buffer__palette_changed(&ib)) {
      if (color_flag) {
        update_palette_as_color_art(wuffs_base__image_buffer__palette(&ib));
      } else {
        update_palette_as_ascii_art(wuffs_base__image_buffer__palette(&ib));
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

    cumulative_delay_micros +=
        (1000 * wuffs_base__image_buffer__duration(&ib)) /
        WUFFS_BASE__FLICKS_PER_MILLISECOND;

    if (color_flag) {
      print_color_art(&ib);
    } else {
      print_ascii_art(&ib);
    }

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
