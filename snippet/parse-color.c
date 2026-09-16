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

// This is a small, self-contained, single-file C library to parse an RGB or
// ARGB color as hex (like "b0279c") or a well-known name (like "purple").
//
// To use this file as a "foo.c"-like implementation, instead of a "foo.h"-like
// header, #define PARSE_COLOR_IMPLEMENTATION before #include'ing or compiling
// it.
//
// As an option, you may also #define PARSE_COLOR_CONFIG__STATIC_FUNCTIONS to
// make these functions have static storage. This can help the compiler ignore
// or discard unused code, which can produce faster compiles and smaller
// binaries.
//
// It is a C port of the github.com/google/wuffs/lib/parsecolor library.

#ifndef PARSE_COLOR_INCLUDE_GUARD
#define PARSE_COLOR_INCLUDE_GUARD

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#if defined(PARSE_COLOR_CONFIG__STATIC_FUNCTIONS)
#define PARSE_COLOR__MAYBE_STATIC static
#else
#define PARSE_COLOR__MAYBE_STATIC
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// parse_color parses a string that's a hex color (like "#ff8" or "ffb0279c")
// or a well-known name (like "black", "purple" or "darkred"). Well-known names
// are an ad-hoc set based on the Material Design color names, optionally
// prefixed by "dark" or "light".
//
// On success, it returns a non-negative integer (that fits into a uint32_t,
// but cast as an int64_t) that is 0xAARRGGBB, using non-premultiplied alpha.
//
// On failure, it returns a negative int64_t.
//
// Parsing is case-insensitive. This function is thread-safe.
PARSE_COLOR__MAYBE_STATIC int64_t  //
parse_color(const char* s_ptr, size_t s_len);

// --------

#ifdef PARSE_COLOR_IMPLEMENTATION

PARSE_COLOR__MAYBE_STATIC int64_t  //
parse_color(const char* s_ptr, size_t s_len) {
  if ((s_len <= 0) || (16 <= s_len)) {
    return -1;
  }

  bool starts_with_hash = (*s_ptr == '#');
  if (starts_with_hash) {
    s_ptr++;
    s_len--;
    if (s_len <= 0) {
      return -1;
    }
  }

  // Canonicalize ASCII to lower case.
  char buf[16];
  for (size_t i = 0; i < s_len; i++) {
    if (((int8_t)(s_ptr[i])) < 0) {
      return -1;
    }
    buf[i] = s_ptr[i] | 0x20;
  }

  // Map well known names to Material Design "500 shade" colors.
  if (!starts_with_hash) {
    const char* b_ptr = &buf[0];
    size_t b_len = s_len;
    int adjustment = 0;
    if (((b_len == 4) && !memcmp(buf, "none", 4)) ||
        ((b_len == 11) && !memcmp(buf, "transparent", 11))) {
      return 0;
    } else if ((b_len > 4) && !memcmp(buf, "dark", 4)) {
      adjustment = 1;
      b_ptr += 4;
      b_len -= 4;
    } else if ((b_len > 5) && !memcmp(buf, "light", 5)) {
      adjustment = 2;
      b_ptr += 5;
      b_len -= 5;
    }

    static const int num_offsets = 19;
    static const uint32_t offsets[19] = {
        0x00000000u,  //
        0x05000000u,  // black
        0x0AFFFFFFu,  // white
        0x0DF44336u,  // red
        0x11E91E63u,  // pink
        0x179C27B0u,  // purple
        0x1D3F51B5u,  // indigo
        0x212196F3u,  // blue
        0x2500BCD4u,  // cyan
        0x29009688u,  // teal
        0x2E4CAF50u,  // green
        0x32CDDC39u,  // lime
        0x38FFEB3Bu,  // yellow
        0x3DFFC107u,  // amber
        0x43FF9800u,  // orange
        0x48795548u,  // brown
        0x4C9E9E9Eu,  // gray

        // Ad-hoc synonyms for some of the official Material Design names.
        0x509E9E9Eu,  // grey
        0x579C27B0u,  // magenta
    };

    static const char names[] =
        "blackwhiteredpinkpurpleindigobluecyantealgreenlimeyellowamberorangebro"
        "wngraygreymagenta";

    for (int i = 1; i < num_offsets; i++) {
      size_t name_len = (offsets[i] >> 24) - (offsets[i - 1] >> 24);
      if (name_len != b_len) {
        continue;
      }
      const char* p = b_ptr;
      const char* p_end = &buf[s_len];
      const char* q = &names[offsets[i - 1] >> 24];

      while (1) {
        if (p == p_end) {
          uint32_t r = 0xFFu & (offsets[i] >> 16);
          uint32_t g = 0xFFu & (offsets[i] >> 8);
          uint32_t b = 0xFFu & (offsets[i] >> 0);
          if (adjustment == 1) {
            r /= 2;
            g /= 2;
            b /= 2;
          } else if (adjustment == 2) {
            r = 0xFFu - ((0xFFu - r) / 2);
            g = 0xFFu - ((0xFFu - g) / 2);
            b = 0xFFu - ((0xFFu - b) / 2);
          }
          return ((int64_t)(0xFF000000u | (r << 16) | (g << 8) | b));

        } else if (*p++ != *q++) {
          break;
        }
      }
    }
  }

  uint32_t ret = 0u;
  uint32_t multiplier = 0u;
  int shift = 0;

  if (s_len == 3) {
    ret = 0xFFu;
    multiplier = 0x11u;
    shift = 8;
  } else if (s_len == 4) {
    ret = 0u;
    multiplier = 0x11u;
    shift = 8;
  } else if (s_len == 6) {
    ret = 0xFFu;
    multiplier = 0x01u;
    shift = 4;
  } else if (s_len == 8) {
    ret = 0u;
    multiplier = 0x01u;
    shift = 4;
  } else {
    return -1;
  }

  for (size_t i = 0; i < s_len; i++) {
    char c = buf[i];
    if (('0' <= c) && (c <= '9')) {
      c -= '0';
    } else if (('a' <= c) && (c <= 'f')) {
      c -= 'a' - 10;
    } else {
      return -1;
    }
    ret <<= shift;
    ret |= multiplier * ((uint32_t)(c));
  }

  return ((int64_t)(ret));
}

#endif  // PARSE_COLOR_IMPLEMENTATION

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif  // PARSE_COLOR_INCLUDE_GUARD
