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

// ----------------

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

// ----------------

#if defined(__unix__) || defined(__MACH__)

#include <time.h>
#include <unistd.h>
#define WUFFS_EXAMPLE_USE_TIMERS

bool started = false;
struct timespec start_time = {0};

int64_t micros_since_start(struct timespec* now) {
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

#else
#warning "TODO: timers on non-POSIX systems"
#endif

// ----------------

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
#include "../../release/c/wuffs-unsupported-snapshot.c"

// Limit the input GIF image to (64 MiB - 1 byte) compressed and 4096 × 4096
// pixels uncompressed. This is a limitation of this example program (which
// uses the Wuffs standard library), not a limitation of Wuffs per se.
//
// We keep the whole input in memory, instead of one-pass stream processing,
// because playing a looping animation requires re-winding the input.
#ifndef SRC_BUFFER_SIZE
#define SRC_BUFFER_SIZE (64 * 1024 * 1024)
#endif
#ifndef MAX_DIMENSION
#define MAX_DIMENSION (4096)
#endif

uint8_t src_buffer[SRC_BUFFER_SIZE] = {0};
size_t src_len = 0;

wuffs_base__color_u32_argb_premul* curr_dst_buffer = NULL;
wuffs_base__color_u32_argb_premul* prev_dst_buffer = NULL;
size_t dst_len;  // Length in bytes.

wuffs_base__slice_u8 pixbuf = {0};
wuffs_base__slice_u8 workbuf = {0};
wuffs_base__slice_u8 printbuf = {0};

bool first_play = true;
uint32_t num_loops_remaining = 0;
wuffs_base__pixel_buffer pb = {0};

wuffs_base__flicks cumulative_delay_micros = 0;

const char* read_stdin() {
  while (src_len < SRC_BUFFER_SIZE) {
    size_t n = fread(src_buffer + src_len, sizeof(uint8_t),
                     SRC_BUFFER_SIZE - src_len, stdin);
    src_len += n;
    if (feof(stdin)) {
      return NULL;
    } else if (ferror(stdin)) {
      return "read error";
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
bool quirk_honor_background_color_flag = false;
const int stdout_fd = 1;

static inline uint32_t load_u32le(uint8_t* p) {
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16) | ((uint32_t)(p[3]) << 24);
}

void restore_background(wuffs_base__pixel_buffer* pb,
                        wuffs_base__rect_ie_u32 bounds,
                        wuffs_base__color_u32_argb_premul background_color) {
  size_t width = wuffs_base__pixel_config__width(&pb->pixcfg);
  size_t y;
  for (y = bounds.min_incl_y; y < bounds.max_excl_y; y++) {
    size_t x;
    wuffs_base__color_u32_argb_premul* d =
        curr_dst_buffer + (y * width) + bounds.min_incl_x;
    for (x = bounds.min_incl_x; x < bounds.max_excl_x; x++) {
      *d++ = background_color;
    }
  }
}

void compose(wuffs_base__pixel_buffer* pb, wuffs_base__rect_ie_u32 bounds) {
  wuffs_base__table_u8 tab = wuffs_base__pixel_buffer__plane(pb, 0);
  size_t width = wuffs_base__pixel_config__width(&pb->pixcfg);
  size_t y;
  for (y = bounds.min_incl_y; y < bounds.max_excl_y; y++) {
    size_t x;
    wuffs_base__color_u32_argb_premul* d =
        curr_dst_buffer + (y * width) + (bounds.min_incl_x * 1);
    uint8_t* s = tab.ptr + (y * tab.stride) + (bounds.min_incl_x * 4);
    for (x = bounds.min_incl_x; x < bounds.max_excl_x; x++) {
      wuffs_base__color_u32_argb_premul c = load_u32le(s);
      if (c) {
        *d = c;
      }
      d += 1;
      s += 4;
    }
  }
}

size_t print_ascii_art(wuffs_base__pixel_buffer* pb) {
  uint32_t width = wuffs_base__pixel_config__width(&pb->pixcfg);
  uint32_t height = wuffs_base__pixel_config__height(&pb->pixcfg);

  wuffs_base__color_u32_argb_premul* d = curr_dst_buffer;
  uint8_t* p = printbuf.ptr;
  *p++ = '\n';
  uint32_t y;
  for (y = 0; y < height; y++) {
    uint32_t x;
    for (x = 0; x < width; x++) {
      wuffs_base__color_u32_argb_premul c = *d++;
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
  return p - printbuf.ptr;
}

size_t print_color_art(wuffs_base__pixel_buffer* pb) {
  uint32_t width = wuffs_base__pixel_config__width(&pb->pixcfg);
  uint32_t height = wuffs_base__pixel_config__height(&pb->pixcfg);

  wuffs_base__color_u32_argb_premul* d = curr_dst_buffer;
  uint8_t* p = printbuf.ptr;
  *p++ = '\n';
  p += sprintf((char*)p, "%s", reset_color);
  uint32_t y;
  for (y = 0; y < height; y++) {
    uint32_t x;
    for (x = 0; x < width; x++) {
      wuffs_base__color_u32_argb_premul c = *d++;
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
  return p - printbuf.ptr;
}

// ----

const char* try_allocate(wuffs_gif__decoder* dec,
                         wuffs_base__image_config* ic) {
  uint32_t width = wuffs_base__pixel_config__width(&ic->pixcfg);
  uint32_t height = wuffs_base__pixel_config__height(&ic->pixcfg);
  uint64_t num_pixels = ((uint64_t)width) * ((uint64_t)height);
  if (num_pixels > (SIZE_MAX / sizeof(wuffs_base__color_u32_argb_premul))) {
    return "could not allocate dst buffer";
  }

  dst_len = num_pixels * sizeof(wuffs_base__color_u32_argb_premul);
  curr_dst_buffer = (wuffs_base__color_u32_argb_premul*)malloc(dst_len);
  if (!curr_dst_buffer) {
    return "could not allocate curr-dst buffer";
  }

  prev_dst_buffer = (wuffs_base__color_u32_argb_premul*)malloc(dst_len);
  if (!prev_dst_buffer) {
    return "could not allocate prev-dst buffer";
  }

  pixbuf = wuffs_base__malloc_slice_u8(
      malloc, wuffs_base__pixel_config__pixbuf_len(&ic->pixcfg));
  if (!pixbuf.ptr) {
    return "could not allocate pixel buffer";
  }

  workbuf = wuffs_base__malloc_slice_u8(
      malloc, wuffs_gif__decoder__workbuf_len(dec).max_incl);
  if (!workbuf.ptr) {
    return "could not allocate work buffer";
  }

  uint64_t plen = 1 + ((uint64_t)(width) + 1) * (uint64_t)(height);
  uint64_t bytes_per_print_pixel = color_flag ? BYTES_PER_COLOR_PIXEL : 1;
  if (plen <= ((uint64_t)SIZE_MAX) / bytes_per_print_pixel) {
    printbuf =
        wuffs_base__malloc_slice_u8(malloc, plen * bytes_per_print_pixel);
  }
  if (!printbuf.ptr) {
    return "could not allocate print buffer";
  }

  return NULL;
}

const char* allocate(wuffs_gif__decoder* dec, wuffs_base__image_config* ic) {
  const char* status = try_allocate(dec, ic);
  if (status) {
    free(printbuf.ptr);
    printbuf = wuffs_base__make_slice_u8(NULL, 0);
    free(workbuf.ptr);
    workbuf = wuffs_base__make_slice_u8(NULL, 0);
    free(pixbuf.ptr);
    pixbuf = wuffs_base__make_slice_u8(NULL, 0);
    free(prev_dst_buffer);
    prev_dst_buffer = NULL;
    free(curr_dst_buffer);
    curr_dst_buffer = NULL;
    dst_len = 0;
  }
  return status;
}

const char* play() {
  wuffs_gif__decoder dec;
  wuffs_base__status status =
      wuffs_gif__decoder__initialize(&dec, sizeof dec, WUFFS_VERSION, 0);
  if (status) {
    return status;
  }

  if (quirk_honor_background_color_flag) {
    wuffs_gif__decoder__set_quirk_enabled(
        &dec, WUFFS_GIF__QUIRK_HONOR_BACKGROUND_COLOR, true);
  }

  wuffs_base__io_buffer src;
  src.data.ptr = src_buffer;
  src.data.len = src_len;
  src.meta.wi = src_len;
  src.meta.ri = 0;
  src.meta.pos = 0;
  src.meta.closed = true;

  if (first_play) {
    wuffs_base__image_config ic = {0};
    status = wuffs_gif__decoder__decode_image_config(&dec, &ic, &src);
    if (status) {
      return status;
    }
    if (!wuffs_base__image_config__is_valid(&ic)) {
      return "invalid image configuration";
    }
    uint32_t width = wuffs_base__pixel_config__width(&ic.pixcfg);
    uint32_t height = wuffs_base__pixel_config__height(&ic.pixcfg);
    if ((width > MAX_DIMENSION) || (height > MAX_DIMENSION)) {
      return "image dimensions are too large";
    }

    // Override the source's indexed pixel format to be non-indexed.
    wuffs_base__pixel_config__set(
        &ic.pixcfg, WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
        WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width, height);

    const char* msg = allocate(&dec, &ic);
    if (msg) {
      return msg;
    }
    status = wuffs_base__pixel_buffer__set_from_slice(&pb, &ic.pixcfg, pixbuf);
    if (status) {
      return status;
    }
    memset(pixbuf.ptr, 0, pixbuf.len);
  }

  while (1) {
    wuffs_base__frame_config fc = {0};
    wuffs_base__status status =
        wuffs_gif__decoder__decode_frame_config(&dec, &fc, &src);
    if (status) {
      if (status == wuffs_base__warning__end_of_data) {
        break;
      }
      return status;
    }

    if (wuffs_base__frame_config__index(&fc) == 0) {
      wuffs_base__color_u32_argb_premul background_color =
          wuffs_base__frame_config__background_color(&fc);
      size_t i;
      size_t n = dst_len / sizeof(wuffs_base__color_u32_argb_premul);
      for (i = 0; i < n; i++) {
        curr_dst_buffer[i] = background_color;
      }
    }

    switch (wuffs_base__frame_config__disposal(&fc)) {
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS: {
        memcpy(prev_dst_buffer, curr_dst_buffer, dst_len);
        break;
      }
    }

    wuffs_base__status decode_frame_status =
        wuffs_gif__decoder__decode_frame(&dec, &pb, &src, workbuf, NULL);
    if (decode_frame_status == wuffs_base__warning__end_of_data) {
      break;
    }

    compose(&pb, wuffs_base__frame_config__bounds(&fc));

    size_t n = color_flag ? print_color_art(&pb) : print_ascii_art(&pb);

    switch (wuffs_base__frame_config__disposal(&fc)) {
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_BACKGROUND: {
        restore_background(&pb, wuffs_base__frame_config__bounds(&fc),
                           wuffs_base__frame_config__background_color(&fc));
        break;
      }
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS: {
        wuffs_base__color_u32_argb_premul* swap = curr_dst_buffer;
        curr_dst_buffer = prev_dst_buffer;
        prev_dst_buffer = swap;
        break;
      }
    }

#if defined(WUFFS_EXAMPLE_USE_TIMERS)
    if (started) {
      struct timespec now;
      if (clock_gettime(CLOCK_MONOTONIC, &now)) {
        return strerror(errno);
      }
      int64_t elapsed_micros = micros_since_start(&now);
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

    fwrite(printbuf.ptr, sizeof(uint8_t), n, stdout);
    fflush(stdout);

    cumulative_delay_micros +=
        (1000 * wuffs_base__frame_config__duration(&fc)) /
        WUFFS_BASE__FLICKS_PER_MILLISECOND;

    // TODO: should a zero duration mean to show this frame forever?

    if (decode_frame_status) {
      return decode_frame_status;
    }
  }

  if (first_play) {
    first_play = false;
    num_loops_remaining = wuffs_gif__decoder__num_animation_loops(&dec);
  }

  return NULL;
}

int main(int argc, char** argv) {
  int i;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-color")) {
      color_flag = true;
    }
    if (!strcmp(argv[i], "-quirk_honor_background_color")) {
      quirk_honor_background_color_flag = true;
    }
  }

  const char* status = read_stdin();
  if (status) {
    fprintf(stderr, "%s\n", status);
    return 1;
  }

  while (true) {
    status = play();
    if (status) {
      fprintf(stderr, "%s\n", status);
      return 1;
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
