// After editing this file, run "go generate" in the parent directory.

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

// ---------------- Images

const uint32_t wuffs_base__pixel_format__bits_per_channel[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x0A, 0x0C, 0x10, 0x18, 0x20, 0x30, 0x40,
};

// --------

static inline uint32_t  //
wuffs_base__swap_u32_argb_abgr(uint32_t u) {
  uint32_t o = u & 0xFF00FF00;
  uint32_t r = u & 0x00FF0000;
  uint32_t b = u & 0x000000FF;
  return o | (r >> 16) | (b << 16);
}

// --------

wuffs_base__color_u32_argb_premul  //
wuffs_base__pixel_buffer__color_u32_at(const wuffs_base__pixel_buffer* pb,
                                       uint32_t x,
                                       uint32_t y) {
  if (!pb || (x >= pb->pixcfg.private_impl.width) ||
      (y >= pb->pixcfg.private_impl.height)) {
    return 0;
  }

  if (wuffs_base__pixel_format__is_planar(&pb->pixcfg.private_impl.pixfmt)) {
    // TODO: support planar formats.
    return 0;
  }

  size_t stride = pb->private_impl.planes[0].stride;
  uint8_t* row = pb->private_impl.planes[0].ptr + (stride * ((size_t)y));

  switch (pb->pixcfg.private_impl.pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
      return wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x)));

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY: {
      uint8_t* palette = pb->private_impl.planes[3].ptr;
      return wuffs_base__load_u32le__no_bounds_check(palette +
                                                     (4 * ((size_t)row[x])));
    }

      // Common formats above. Rarer formats below.

    case WUFFS_BASE__PIXEL_FORMAT__Y:
      return 0xFF000000 | (0x00010101 * ((uint32_t)(row[x])));

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL: {
      uint8_t* palette = pb->private_impl.planes[3].ptr;
      return wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
          wuffs_base__load_u32le__no_bounds_check(palette +
                                                  (4 * ((size_t)row[x]))));
    }

    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      return wuffs_base__color_u16_rgb_565__as__color_u32_argb_premul(
          wuffs_base__load_u16le__no_bounds_check(row + (2 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      return 0xFF000000 |
             wuffs_base__load_u24le__no_bounds_check(row + (3 * ((size_t)x)));
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      return wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      return 0xFF000000 |
             wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x)));

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      return wuffs_base__swap_u32_argb_abgr(
          0xFF000000 |
          wuffs_base__load_u24le__no_bounds_check(row + (3 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
      return wuffs_base__swap_u32_argb_abgr(
          wuffs_base__color_u32_argb_nonpremul__as__color_u32_argb_premul(
              wuffs_base__load_u32le__no_bounds_check(row +
                                                      (4 * ((size_t)x)))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
      return wuffs_base__swap_u32_argb_abgr(
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      return wuffs_base__swap_u32_argb_abgr(
          0xFF000000 |
          wuffs_base__load_u32le__no_bounds_check(row + (4 * ((size_t)x))));

    default:
      // TODO: support more formats.
      break;
  }

  return 0;
}

wuffs_base__status  //
wuffs_base__pixel_buffer__set_color_u32_at(
    wuffs_base__pixel_buffer* pb,
    uint32_t x,
    uint32_t y,
    wuffs_base__color_u32_argb_premul color) {
  if (!pb) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  if ((x >= pb->pixcfg.private_impl.width) ||
      (y >= pb->pixcfg.private_impl.height)) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }

  if (wuffs_base__pixel_format__is_planar(&pb->pixcfg.private_impl.pixfmt)) {
    // TODO: support planar formats.
    return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }

  size_t stride = pb->private_impl.planes[0].stride;
  uint8_t* row = pb->private_impl.planes[0].ptr + (stride * ((size_t)y));

  switch (pb->pixcfg.private_impl.pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__BGRX:
      wuffs_base__store_u32le__no_bounds_check(row + (4 * ((size_t)x)), color);
      break;

      // Common formats above. Rarer formats below.

    case WUFFS_BASE__PIXEL_FORMAT__Y:
      wuffs_base__store_u8__no_bounds_check(
          row + ((size_t)x),
          wuffs_base__color_u32_argb_premul__as__color_u8_gray(color));
      break;

    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
      wuffs_base__store_u8__no_bounds_check(
          row + ((size_t)x), wuffs_base__pixel_palette__closest_element(
                                 wuffs_base__pixel_buffer__palette(pb),
                                 pb->pixcfg.private_impl.pixfmt, color));
      break;

    case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      wuffs_base__store_u16le__no_bounds_check(
          row + (2 * ((size_t)x)),
          wuffs_base__color_u32_argb_premul__as__color_u16_rgb_565(color));
      break;
    case WUFFS_BASE__PIXEL_FORMAT__BGR:
      wuffs_base__store_u24le__no_bounds_check(row + (3 * ((size_t)x)), color);
      break;
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)),
          wuffs_base__color_u32_argb_premul__as__color_u32_argb_nonpremul(
              color));
      break;

    case WUFFS_BASE__PIXEL_FORMAT__RGB:
      wuffs_base__store_u24le__no_bounds_check(
          row + (3 * ((size_t)x)), wuffs_base__swap_u32_argb_abgr(color));
      break;
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)),
          wuffs_base__color_u32_argb_premul__as__color_u32_argb_nonpremul(
              wuffs_base__swap_u32_argb_abgr(color)));
      break;
    case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__RGBX:
      wuffs_base__store_u32le__no_bounds_check(
          row + (4 * ((size_t)x)), wuffs_base__swap_u32_argb_abgr(color));
      break;

    default:
      // TODO: support more formats.
      return wuffs_base__make_status(wuffs_base__error__unsupported_option);
  }

  return wuffs_base__make_status(NULL);
}

// --------

uint8_t  //
wuffs_base__pixel_palette__closest_element(
    wuffs_base__slice_u8 palette_slice,
    wuffs_base__pixel_format palette_format,
    wuffs_base__color_u32_argb_premul c) {
  size_t n = palette_slice.len / 4;
  if (n > 256) {
    n = 256;
  }
  size_t best_index = 0;
  uint64_t best_score = 0xFFFFFFFFFFFFFFFF;

  // Work in 16-bit color.
  uint32_t ca = 0x101 * (0xFF & (c >> 24));
  uint32_t cr = 0x101 * (0xFF & (c >> 16));
  uint32_t cg = 0x101 * (0xFF & (c >> 8));
  uint32_t cb = 0x101 * (0xFF & (c >> 0));

  switch (palette_format.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY: {
      bool nonpremul = palette_format.repr ==
                       WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL;

      size_t i;
      for (i = 0; i < n; i++) {
        // Work in 16-bit color.
        uint32_t pb = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 0]));
        uint32_t pg = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 1]));
        uint32_t pr = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 2]));
        uint32_t pa = 0x101 * ((uint32_t)(palette_slice.ptr[(4 * i) + 3]));

        // Convert to premultiplied alpha.
        if (nonpremul && (pa != 0xFFFF)) {
          pb = (pb * pa) / 0xFFFF;
          pg = (pg * pa) / 0xFFFF;
          pr = (pr * pa) / 0xFFFF;
        }

        // These deltas are conceptually int32_t (signed) but after squaring,
        // it's equivalent to work in uint32_t (unsigned).
        pb -= cb;
        pg -= cg;
        pr -= cr;
        pa -= ca;
        uint64_t score = ((uint64_t)(pb * pb)) + ((uint64_t)(pg * pg)) +
                         ((uint64_t)(pr * pr)) + ((uint64_t)(pa * pa));
        if (best_score > score) {
          best_score = score;
          best_index = i;
        }
      }
      break;
    }
  }

  return (uint8_t)best_index;
}
