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

// This exercises snippet/uncompng.c to write a Julia Set image to stdout.
//
// See also
// https://nigeltao.github.io/blog/2025/uncompressed-png.html

#include <errno.h>
#include <unistd.h>

#define UNCOMPNG_CONFIG__STATIC_FUNCTIONS
#define UNCOMPNG_IMPLEMENTATION
#include "../snippet/uncompng.c"

#define CONFIG_CX -0.70000f
#define CONFIG_CY +0.27015f
#define CONFIG_IMAGE_WIDTH 256
#define CONFIG_IMAGE_HEIGHT 256

int  //
my_write_func(void* context, const uint8_t* data_ptr, size_t data_len) {
  static const int stdout_fd = 1;
  return (write(stdout_fd, data_ptr, data_len) < 0) ? -errno : 0;
}

uint8_t  //
my_pixel_func(float fx, float fy) {
  static const float escape_distance_squared = 100.0f;
  for (int i = 255; i > 0; i--) {
    float distance_squared = (fx * fx) + (fy * fy);
    if (distance_squared >= escape_distance_squared) {
      return i;
    }
    float gx = CONFIG_CX + (fx * fx) - (fy * fy);
    float gy = CONFIG_CY + (2 * fx * fy);
    fx = gx;
    fy = gy;
  }
  return 0;
}

int  //
main(int argc, char** argv) {
  static const float w2 = CONFIG_IMAGE_WIDTH / 2;
  static const float h2 = CONFIG_IMAGE_HEIGHT / 2;
  static uint8_t pixels[CONFIG_IMAGE_HEIGHT][CONFIG_IMAGE_WIDTH];
  for (int iy = 0; iy < CONFIG_IMAGE_HEIGHT; iy++) {
    float fy = (iy - h2) / h2;
    for (int ix = 0; ix < CONFIG_IMAGE_WIDTH; ix++) {
      float fx = (ix - w2) / w2;
      pixels[iy][ix] = my_pixel_func(fx, fy);
    }
  }
  return uncompng__encode(&my_write_func, NULL, UNCOMPNG__PIXEL_FORMAT__Y,
                          CONFIG_IMAGE_WIDTH, CONFIG_IMAGE_HEIGHT,
                          &pixels[0][0], sizeof(pixels), CONFIG_IMAGE_WIDTH);
}
