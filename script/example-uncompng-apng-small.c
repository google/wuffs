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

// This exercises snippet/uncompng.c to write a small APNG image to stdout.
//
// It is a C port of lib/uncompng's TestAnimationEncoderSmall.

#include <errno.h>
#include <unistd.h>

#define UNCOMPNG_CONFIG__STATIC_FUNCTIONS
#define UNCOMPNG_IMPLEMENTATION
#include "../snippet/uncompng.c"

#define IMAGE_HEIGHT 2
#define IMAGE_WIDTH 3
#define NUM_FRAMES 2
#define NUM_PLAYS 10

int  //
my_write_func(void* context, const uint8_t* data_ptr, size_t data_len) {
  static const int stdout_fd = 1;
  return (write(stdout_fd, data_ptr, data_len) < 0) ? -errno : 0;
}

int  //
main(int argc, char** argv) {
  static uint8_t pixels[NUM_FRAMES][IMAGE_WIDTH * IMAGE_HEIGHT * 4] = {
      {
          0xFF, 0x00, 0x00, 0xFF,  // Blue.
          0xFF, 0xFF, 0xFF, 0xFF,  // White.
          0x00, 0x00, 0xFF, 0xFF,  // Red.

          0xFF, 0x00, 0x00, 0xFF,  // Blue.
          0xFF, 0xFF, 0xFF, 0xFF,  // White.
          0x00, 0x00, 0xFF, 0xFF,  // Red.
      },
      {
          0x00, 0xFF, 0x00, 0xFF,  // Green.
          0xFF, 0xFF, 0xFF, 0xFF,  // White.
          0x00, 0x00, 0xFF, 0xFF,  // Red.

          0x00, 0xFF, 0x00, 0xFF,  // Green.
          0xFF, 0xFF, 0xFF, 0xFF,  // White.
          0x00, 0x00, 0xFF, 0xFF,  // Red.
      }};

  // Change "if (0)" to "if (1)" to write a still (not animated) PNG.
  if (0) {
    const int frame = 0;
    return uncompng__encode(                                      //
        &my_write_func, NULL,                                     //
        UNCOMPNG__PIXEL_FORMAT__BGRX, IMAGE_WIDTH, IMAGE_HEIGHT,  //
        &pixels[frame][0], sizeof(pixels[frame]), IMAGE_WIDTH * 4);
  }

  static uint16_t delay_millis[NUM_FRAMES] = {1000, 2000};

  int err0 = uncompng__encode_apng_header(                      //
      &my_write_func, NULL,                                     //
      UNCOMPNG__PIXEL_FORMAT__BGRX, IMAGE_WIDTH, IMAGE_HEIGHT,  //
      NUM_FRAMES, NUM_PLAYS);
  if (err0) {
    return err0;
  }

  for (int frame = 0; frame < NUM_FRAMES; frame++) {
    int err1 = uncompng__encode_apng_frame(  //
        &my_write_func, NULL,                //
        delay_millis[frame], 1000,           //
        &pixels[frame][0], sizeof(pixels[frame]), IMAGE_WIDTH * 4);
    if (err1) {
      return err1;
    }
  }

  return 0;
}
