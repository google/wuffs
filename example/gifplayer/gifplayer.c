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

bool g_started = false;
struct timespec g_start_time = {0};

int64_t  //
micros_since_start(struct timespec* now) {
  if (!g_started) {
    return 0;
  }
  int64_t nanos = (int64_t)(now->tv_sec - g_start_time.tv_sec) * 1000000000 +
                  (int64_t)(now->tv_nsec - g_start_time.tv_nsec);
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

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c whitelist which parts of Wuffs to build. That file contains
// the entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__LZW
#define WUFFS_CONFIG__MODULE__GIF

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../release/c/wuffs-unsupported-snapshot.c"

#define TRY(error_msg)         \
  do {                         \
    const char* z = error_msg; \
    if (z) {                   \
      return z;                \
    }                          \
  } while (false)

// Limit the input GIF image to (64 MiB - 1 byte) compressed and 4096 × 4096
// pixels uncompressed. This is a limitation of this example program (which
// uses the Wuffs standard library), not a limitation of Wuffs per se.
//
// We keep the whole input in memory, instead of one-pass stream processing,
// because playing a looping animation requires re-winding the input.
#ifndef SRC_BUFFER_ARRAY_SIZE
#define SRC_BUFFER_ARRAY_SIZE (64 * 1024 * 1024)
#endif
#ifndef MAX_DIMENSION
#define MAX_DIMENSION (4096)
#endif

uint8_t g_src_buffer_array[SRC_BUFFER_ARRAY_SIZE] = {0};
size_t g_src_len = 0;

uint8_t* g_curr_dst_buffer = NULL;
uint8_t* g_prev_dst_buffer = NULL;
size_t g_dst_len;  // Length in bytes.

wuffs_base__slice_u8 g_workbuf = {0};
wuffs_base__slice_u8 g_printbuf = {0};

bool g_first_play = true;
uint32_t g_num_loops_remaining = 0;
wuffs_base__image_config g_ic = {0};
wuffs_base__pixel_buffer g_pb = {0};

wuffs_base__flicks g_cumulative_delay_micros = 0;

const char*  //
read_stdin() {
  while (g_src_len < SRC_BUFFER_ARRAY_SIZE) {
    size_t n = fread(g_src_buffer_array + g_src_len, sizeof(uint8_t),
                     SRC_BUFFER_ARRAY_SIZE - g_src_len, stdin);
    g_src_len += n;
    if (feof(stdin)) {
      return NULL;
    } else if (ferror(stdin)) {
      return "read error";
    }
  }
  return "input is too large";
}

// ----

struct {
  int remaining_argc;
  char** remaining_argv;

  bool color;
  bool quirk_honor_background_color;
} g_flags = {0};

const char*  //
parse_flags(int argc, char** argv) {
  int c = (argc > 0) ? 1 : 0;  // Skip argv[0], the program name.
  for (; c < argc; c++) {
    char* arg = argv[c];
    if (*arg++ != '-') {
      break;
    }

    // A double-dash "--foo" is equivalent to a single-dash "-foo". As special
    // cases, a bare "-" is not a flag (some programs may interpret it as
    // stdin) and a bare "--" means to stop parsing flags.
    if (*arg == '\x00') {
      break;
    } else if (*arg == '-') {
      arg++;
      if (*arg == '\x00') {
        c++;
        break;
      }
    }

    if (!strcmp(arg, "c") || !strcmp(arg, "color")) {
      g_flags.color = true;
      continue;
    }
    if (!strcmp(arg, "quirk_honor_background_color")) {
      g_flags.quirk_honor_background_color = true;
      continue;
    }

    return "main: unrecognized flag argument";
  }

  g_flags.remaining_argc = argc - c;
  g_flags.remaining_argv = argv + c;
  return NULL;
}

// ----

// BYTES_PER_COLOR_PIXEL is long enough to contain "\x1B[38;2;255;255;255mABC"
// plus a trailing NUL byte and a few bytes of slack. It starts with a true
// color terminal escape code. ABC is three bytes for the UTF-8 representation
// "\xE2\x96\x88" of "█", U+2588 FULL BLOCK.
#define BYTES_PER_COLOR_PIXEL 32

const char* g_reset_color = "\x1B[0m";

void  //
restore_background(wuffs_base__pixel_buffer* pb,
                   wuffs_base__rect_ie_u32 bounds,
                   wuffs_base__color_u32_argb_premul background_color) {
  size_t width4 = (size_t)(wuffs_base__pixel_config__width(&pb->pixcfg)) * 4;
  size_t y;
  for (y = bounds.min_incl_y; y < bounds.max_excl_y; y++) {
    size_t x;
    uint8_t* d = g_curr_dst_buffer + (y * width4) + (bounds.min_incl_x * 4);
    for (x = bounds.min_incl_x; x < bounds.max_excl_x; x++) {
      wuffs_base__store_u32le__no_bounds_check(d, background_color);
      d += sizeof(wuffs_base__color_u32_argb_premul);
    }
  }
}

size_t  //
print_ascii_art(wuffs_base__pixel_buffer* pb) {
  uint32_t width = wuffs_base__pixel_config__width(&pb->pixcfg);
  uint32_t height = wuffs_base__pixel_config__height(&pb->pixcfg);

  uint8_t* d = g_curr_dst_buffer;
  uint8_t* p = g_printbuf.ptr;
  *p++ = '\n';
  uint32_t y;
  for (y = 0; y < height; y++) {
    uint32_t x;
    for (x = 0; x < width; x++) {
      wuffs_base__color_u32_argb_premul c =
          wuffs_base__load_u32le__no_bounds_check(d);
      d += sizeof(wuffs_base__color_u32_argb_premul);
      // Convert to grayscale via the formula
      //  Y = (0.299 * R) + (0.587 * G) + (0.114 * B)
      // translated into fixed point arithmetic.
      uint32_t b = 0xFF & (c >> 0);
      uint32_t g = 0xFF & (c >> 8);
      uint32_t r = 0xFF & (c >> 16);
      uint32_t gray =
          ((19595 * r) + (38470 * g) + (7471 * b) + (1 << 15)) >> 16;
      *p++ = "-:=+IOX@"[(gray & 0xFF) >> 5];
    }
    *p++ = '\n';
  }
  return p - g_printbuf.ptr;
}

size_t  //
print_color_art(wuffs_base__pixel_buffer* pb) {
  uint32_t width = wuffs_base__pixel_config__width(&pb->pixcfg);
  uint32_t height = wuffs_base__pixel_config__height(&pb->pixcfg);

  uint8_t* d = g_curr_dst_buffer;
  uint8_t* p = g_printbuf.ptr;
  *p++ = '\n';
  p += sprintf((char*)p, "%s", g_reset_color);
  uint32_t y;
  for (y = 0; y < height; y++) {
    uint32_t x;
    for (x = 0; x < width; x++) {
      wuffs_base__color_u32_argb_premul c =
          wuffs_base__load_u32le__no_bounds_check(d);
      d += sizeof(wuffs_base__color_u32_argb_premul);
      int b = 0xFF & (c >> 0);
      int g = 0xFF & (c >> 8);
      int r = 0xFF & (c >> 16);
      // "\xE2\x96\x88" is U+2588 FULL BLOCK. Before that is a true color
      // terminal escape code.
      p += sprintf((char*)p, "\x1B[38;2;%d;%d;%dm\xE2\x96\x88", r, g, b);
    }
    *p++ = '\n';
  }
  p += sprintf((char*)p, "%s", g_reset_color);
  return p - g_printbuf.ptr;
}

// ----

const char*  //
try_allocate(wuffs_gif__decoder* dec) {
  uint32_t width = wuffs_base__pixel_config__width(&g_ic.pixcfg);
  uint32_t height = wuffs_base__pixel_config__height(&g_ic.pixcfg);
  uint64_t num_pixels = ((uint64_t)width) * ((uint64_t)height);
  if (num_pixels > (SIZE_MAX / sizeof(wuffs_base__color_u32_argb_premul))) {
    return "could not allocate dst buffer";
  }

  g_dst_len = num_pixels * sizeof(wuffs_base__color_u32_argb_premul);
  g_curr_dst_buffer = (uint8_t*)calloc(g_dst_len, 1);
  if (!g_curr_dst_buffer) {
    return "could not allocate curr-dst buffer";
  }

  g_prev_dst_buffer = (uint8_t*)malloc(g_dst_len);
  if (!g_prev_dst_buffer) {
    return "could not allocate prev-dst buffer";
  }

  uint64_t workbuf_len_max_incl = wuffs_gif__decoder__workbuf_len(dec).max_incl;
  if (workbuf_len_max_incl > 0) {
    g_workbuf = wuffs_base__malloc_slice_u8(
        malloc, wuffs_gif__decoder__workbuf_len(dec).max_incl);
    if (!g_workbuf.ptr) {
      return "could not allocate work buffer";
    }
  } else {
    g_workbuf = wuffs_base__make_slice_u8(NULL, 0);
  }

  uint64_t plen = 1 + ((uint64_t)(width) + 1) * (uint64_t)(height);
  uint64_t bytes_per_print_pixel = g_flags.color ? BYTES_PER_COLOR_PIXEL : 1;
  if (plen <= ((uint64_t)SIZE_MAX) / bytes_per_print_pixel) {
    g_printbuf =
        wuffs_base__malloc_slice_u8(malloc, plen * bytes_per_print_pixel);
  }
  if (!g_printbuf.ptr) {
    return "could not allocate print buffer";
  }

  return NULL;
}

const char*  //
allocate(wuffs_gif__decoder* dec) {
  const char* status_msg = try_allocate(dec);
  if (status_msg) {
    free(g_printbuf.ptr);
    g_printbuf = wuffs_base__make_slice_u8(NULL, 0);
    free(g_workbuf.ptr);
    g_workbuf = wuffs_base__make_slice_u8(NULL, 0);
    free(g_prev_dst_buffer);
    g_prev_dst_buffer = NULL;
    free(g_curr_dst_buffer);
    g_curr_dst_buffer = NULL;
    g_dst_len = 0;
  }
  return status_msg;
}

const char*  //
play() {
  wuffs_gif__decoder dec;
  wuffs_base__status i_status =
      wuffs_gif__decoder__initialize(&dec, sizeof dec, WUFFS_VERSION, 0);
  if (!wuffs_base__status__is_ok(&i_status)) {
    return wuffs_base__status__message(&i_status);
  }

  if (g_flags.quirk_honor_background_color) {
    wuffs_gif__decoder__set_quirk_enabled(
        &dec, WUFFS_GIF__QUIRK_HONOR_BACKGROUND_COLOR, true);
  }

  wuffs_base__io_buffer src;
  src.data.ptr = &g_src_buffer_array[0];
  src.data.len = g_src_len;
  src.meta.wi = g_src_len;
  src.meta.ri = 0;
  src.meta.pos = 0;
  src.meta.closed = true;

  if (g_first_play) {
    wuffs_base__status dic_status =
        wuffs_gif__decoder__decode_image_config(&dec, &g_ic, &src);
    if (!wuffs_base__status__is_ok(&dic_status)) {
      return wuffs_base__status__message(&dic_status);
    }
    if (!wuffs_base__image_config__is_valid(&g_ic)) {
      return "invalid image configuration";
    }
    uint32_t width = wuffs_base__pixel_config__width(&g_ic.pixcfg);
    uint32_t height = wuffs_base__pixel_config__height(&g_ic.pixcfg);
    if ((width > MAX_DIMENSION) || (height > MAX_DIMENSION)) {
      return "image dimensions are too large";
    }

    // Override the source's indexed pixel format to be non-indexed.
    wuffs_base__pixel_config__set(
        &g_ic.pixcfg, WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
        WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, width, height);

    const char* msg = allocate(&dec);
    if (msg) {
      return msg;
    }
    wuffs_base__status sfs0_status = wuffs_base__pixel_buffer__set_from_slice(
        &g_pb, &g_ic.pixcfg,
        wuffs_base__make_slice_u8(g_curr_dst_buffer, g_dst_len));
    if (!wuffs_base__status__is_ok(&sfs0_status)) {
      return wuffs_base__status__message(&sfs0_status);
    }
  }

  while (1) {
    wuffs_base__frame_config fc = {0};
    wuffs_base__status dfc_status =
        wuffs_gif__decoder__decode_frame_config(&dec, &fc, &src);
    if (!wuffs_base__status__is_ok(&dfc_status)) {
      if (dfc_status.repr == wuffs_base__note__end_of_data) {
        break;
      }
      return wuffs_base__status__message(&dfc_status);
    }

    if (wuffs_base__frame_config__index(&fc) == 0) {
      wuffs_base__color_u32_argb_premul background_color =
          wuffs_base__frame_config__background_color(&fc);
      size_t i;
      size_t n = g_dst_len / sizeof(wuffs_base__color_u32_argb_premul);
      uint8_t* p = g_curr_dst_buffer;
      for (i = 0; i < n; i++) {
        wuffs_base__store_u32le__no_bounds_check(p, background_color);
        p += sizeof(wuffs_base__color_u32_argb_premul);
      }
    }

    switch (wuffs_base__frame_config__disposal(&fc)) {
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS: {
        memcpy(g_prev_dst_buffer, g_curr_dst_buffer, g_dst_len);
        break;
      }
    }

    wuffs_base__status decode_frame_status = wuffs_gif__decoder__decode_frame(
        &dec, &g_pb, &src,
        wuffs_base__frame_config__overwrite_instead_of_blend(&fc)
            ? WUFFS_BASE__PIXEL_BLEND__SRC
            : WUFFS_BASE__PIXEL_BLEND__SRC_OVER,
        g_workbuf, NULL);
    if (decode_frame_status.repr == wuffs_base__note__end_of_data) {
      break;
    }

    size_t n = g_flags.color ? print_color_art(&g_pb) : print_ascii_art(&g_pb);

    switch (wuffs_base__frame_config__disposal(&fc)) {
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_BACKGROUND: {
        restore_background(&g_pb, wuffs_base__frame_config__bounds(&fc),
                           wuffs_base__frame_config__background_color(&fc));
        break;
      }
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS: {
        uint8_t* swap = g_curr_dst_buffer;
        g_curr_dst_buffer = g_prev_dst_buffer;
        g_prev_dst_buffer = swap;

        wuffs_base__status sfs1_status =
            wuffs_base__pixel_buffer__set_from_slice(
                &g_pb, &g_ic.pixcfg,
                wuffs_base__make_slice_u8(g_curr_dst_buffer, g_dst_len));
        if (!wuffs_base__status__is_ok(&sfs1_status)) {
          return wuffs_base__status__message(&sfs1_status);
        }
        break;
      }
    }

#if defined(WUFFS_EXAMPLE_USE_TIMERS)
    if (g_started) {
      struct timespec now;
      if (clock_gettime(CLOCK_MONOTONIC, &now)) {
        return strerror(errno);
      }
      int64_t elapsed_micros = micros_since_start(&now);
      if (g_cumulative_delay_micros > elapsed_micros) {
        usleep(g_cumulative_delay_micros - elapsed_micros);
      }

    } else {
      if (clock_gettime(CLOCK_MONOTONIC, &g_start_time)) {
        return strerror(errno);
      }
      g_started = true;
    }
#endif

    fwrite(g_printbuf.ptr, sizeof(uint8_t), n, stdout);
    fflush(stdout);

    g_cumulative_delay_micros +=
        (1000 * wuffs_base__frame_config__duration(&fc)) /
        WUFFS_BASE__FLICKS_PER_MILLISECOND;

    // TODO: should a zero duration mean to show this frame forever?

    if (!wuffs_base__status__is_ok(&decode_frame_status)) {
      return wuffs_base__status__message(&decode_frame_status);
    }
  }

  if (g_first_play) {
    g_first_play = false;
    g_num_loops_remaining = wuffs_gif__decoder__num_animation_loops(&dec);
  }

  return NULL;
}

const char*  //
main1(int argc, char** argv) {
  TRY(parse_flags(argc, argv));
  if (g_flags.remaining_argc > 0) {
    return "main: bad argument: use \"program < input\", not \"program input\"";
  }
  TRY(read_stdin());
  while (true) {
    TRY(play());
    if (g_num_loops_remaining == 0) {
      continue;
    }
    g_num_loops_remaining--;
    if (g_num_loops_remaining == 0) {
      break;
    }
  }
  return NULL;
}

int  //
main(int argc, char** argv) {
  const char* z = main1(argc, argv);
  if (z) {
    fprintf(stderr, "%s\n", z);
    return 1;
  }
  return 0;
}
