// Copyright 2026 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

// ----------------

// This exercises snippet/uncompng.c to write a large APNG image to stdout.
//
// It is a C port of lib/uncompng's TestAnimationEncoderLarge.
//
// Run it from the Wuffs root directory, the one that contains the
// wuffs-root-directory.txt file, so that it can find and read the
// test/data/hibiscus.regular.bmp file.

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define UNCOMPNG_CONFIG__STATIC_FUNCTIONS
#define UNCOMPNG_IMPLEMENTATION
#include "../snippet/uncompng.c"

#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__BMP
#define WUFFS_CONFIG__STATIC_FUNCTIONS
#define WUFFS_IMPLEMENTATION
#include "../release/c/wuffs-unsupported-snapshot.c"

#define IMAGE_HEIGHT 442
#define IMAGE_WIDTH 312
#define NUM_FRAMES 5
#define NUM_PLAYS 0
#define SRC_BMP_SIZE 413850

static uint8_t g_src_bmp[SRC_BMP_SIZE];
static uint8_t g_src_pixels[IMAGE_WIDTH * IMAGE_HEIGHT * 4];

bool  //
load_src_bmp() {
  int fd = open("test/data/hibiscus.regular.bmp", O_RDONLY, 0);
  if (fd == -1) {
    fprintf(stderr, "FAIL: open: %s\n", strerror(errno));
    return false;
  }

  for (ssize_t num_read = 0; num_read < SRC_BMP_SIZE;) {
    ssize_t n = read(fd, g_src_bmp + num_read, sizeof(g_src_bmp) - num_read);
    if (n > 0) {
      num_read += n;
    } else if (n == 0) {
      break;
    } else if (errno == EINTR) {
      // No-op.
    } else {
      close(fd);
      fprintf(stderr, "FAIL: read: %s\n", strerror(errno));
      return false;
    }
  }

  close(fd);
  return true;
}

bool  //
load_src_pixels() {
  static wuffs_bmp__decoder dec;

  wuffs_base__io_buffer src = wuffs_base__make_io_buffer(
      wuffs_base__make_slice_u8(g_src_bmp, SRC_BMP_SIZE),
      wuffs_base__make_io_buffer_meta(SRC_BMP_SIZE, 0, 0, true));

  wuffs_base__status status = wuffs_bmp__decoder__initialize(
      &dec, sizeof dec, WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
  if (!wuffs_base__status__is_ok(&status)) {
    fprintf(stderr, "FAIL: initialize: %s\n",
            wuffs_base__status__message(&status));
    return false;
  }

  wuffs_base__image_config ic;
  status = wuffs_bmp__decoder__decode_image_config(&dec, &ic, &src);
  if (!wuffs_base__status__is_ok(&status)) {
    fprintf(stderr, "FAIL: decode_image_config: %s\n",
            wuffs_base__status__message(&status));
    return false;
  }

  uint32_t w = wuffs_base__pixel_config__width(&ic.pixcfg);
  uint32_t h = wuffs_base__pixel_config__height(&ic.pixcfg);
  uint32_t pixfmt = wuffs_base__pixel_config__pixel_format(&ic.pixcfg).repr;
  if ((w != IMAGE_WIDTH) || (h != IMAGE_HEIGHT) ||
      (pixfmt != WUFFS_BASE__PIXEL_FORMAT__BGRX)) {
    fprintf(stderr, "FAIL: decode_image_config: unexpected configuration\n");
    return false;
  }

  if ((WUFFS_BASE__PIXEL_FORMAT__BGRX != UNCOMPNG__PIXEL_FORMAT__BGRX) ||
      (WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL !=
       UNCOMPNG__PIXEL_FORMAT__BGRA_NONPREMUL)) {
    fprintf(stderr, "FAIL: wuffs and uncompng are incompatible\n");
    return false;
  }

  wuffs_base__pixel_config__set(&ic.pixcfg,
                                WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL,
                                WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, w, h);

  wuffs_base__pixel_buffer pb;
  status = wuffs_base__pixel_buffer__set_from_slice(
      &pb, &ic.pixcfg,
      wuffs_base__make_slice_u8(&g_src_pixels[0], sizeof g_src_pixels));
  if (!wuffs_base__status__is_ok(&status)) {
    fprintf(stderr, "FAIL: set_from_slice: %s\n",
            wuffs_base__status__message(&status));
    return false;
  }

  status = wuffs_bmp__decoder__decode_frame(&dec, &pb, &src,
                                            WUFFS_BASE__PIXEL_BLEND__SRC,
                                            wuffs_base__empty_slice_u8(), NULL);
  if (!wuffs_base__status__is_ok(&status)) {
    fprintf(stderr, "FAIL: decode_frame: %s\n",
            wuffs_base__status__message(&status));
    return false;
  }

  return true;
}

int  //
my_write_func(void* context, const uint8_t* data_ptr, size_t data_len) {
  static const int stdout_fd = 1;
  return (write(stdout_fd, data_ptr, data_len) < 0) ? -errno : 0;
}

int  //
main(int argc, char** argv) {
  if (!load_src_bmp() || !load_src_pixels()) {
    return 1;
  }

  // Change "if (0)" to "if (1)" to write a still (not animated) PNG.
  if (0) {
    const int frame = 0;
    return uncompng__encode(                                                //
        &my_write_func, NULL,                                               //
        UNCOMPNG__PIXEL_FORMAT__BGRA_NONPREMUL, IMAGE_WIDTH, IMAGE_HEIGHT,  //
        &g_src_pixels[0], sizeof(g_src_pixels), IMAGE_WIDTH * 4);
  }

  static uint16_t delay_millis[NUM_FRAMES] = {300, 500, 600, 400, 200};

  int err0 = uncompng__encode_apng_header(                                //
      &my_write_func, NULL,                                               //
      UNCOMPNG__PIXEL_FORMAT__BGRA_NONPREMUL, IMAGE_WIDTH, IMAGE_HEIGHT,  //
      NUM_FRAMES, NUM_PLAYS);
  if (err0) {
    return err0;
  }

  for (int frame = 0; frame < NUM_FRAMES; frame++) {
    int err1 = uncompng__encode_apng_frame(  //
        &my_write_func, NULL,                //
        delay_millis[frame], 1000,           //
        &g_src_pixels[0], sizeof(g_src_pixels), IMAGE_WIDTH * 4);
    if (err1) {
      return err1;
    }

    // Over time, set the R, G, B, A channel values to 0x80.
    //
    // Use RGBA order, not BGRA, to match the output of lib/uncompng's
    // TestAnimationEncoderLarge, in Go.
    if (frame < 4) {
      static size_t order[4] = {2, 1, 0, 3};
      for (size_t i = order[frame]; i < sizeof(g_src_pixels); i += 4) {
        g_src_pixels[i] = 0x80;
      }
    }
  }

  return 0;
}
