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

static uint64_t  //
wuffs_base__pixel_swizzler__copy_1_1(wuffs_base__slice_u8 dst,
                                     wuffs_base__slice_u8 dst_palette,
                                     wuffs_base__slice_u8 src) {
  return wuffs_base__slice_u8__copy_from_slice(dst, src);
}

static uint64_t  //
wuffs_base__pixel_swizzler__copy_4_1(wuffs_base__slice_u8 dst,
                                     wuffs_base__slice_u8 dst_palette,
                                     wuffs_base__slice_u8 src) {
  if (dst_palette.len != 1024) {
    return 0;
  }
  size_t dst_len4 = dst.len / 4;
  size_t len = dst_len4 < src.len ? dst_len4 : src.len;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;

  size_t n = len;
  const int N = 4;

  while (n >= N) {
    wuffs_base__store_u32le(
        d + (0 * 4),
        wuffs_base__load_u32le(dst_palette.ptr + ((uint32_t)(s[0]) * 4)));
    wuffs_base__store_u32le(
        d + (1 * 4),
        wuffs_base__load_u32le(dst_palette.ptr + ((uint32_t)(s[1]) * 4)));
    wuffs_base__store_u32le(
        d + (2 * 4),
        wuffs_base__load_u32le(dst_palette.ptr + ((uint32_t)(s[2]) * 4)));
    wuffs_base__store_u32le(
        d + (3 * 4),
        wuffs_base__load_u32le(dst_palette.ptr + ((uint32_t)(s[3]) * 4)));

    s += 1 * N;
    d += 4 * N;
    n -= (size_t)(1 * N);
  }

  while (n >= 1) {
    wuffs_base__store_u32le(
        d + (0 * 4),
        wuffs_base__load_u32le(dst_palette.ptr + ((uint32_t)(s[0]) * 4)));

    s += 1 * 1;
    d += 4 * 1;
    n -= (size_t)(1 * 1);
  }

  return len;
}

static uint64_t  //
wuffs_base__pixel_swizzler__swap_rgbx_bgrx(wuffs_base__slice_u8 dst,
                                           wuffs_base__slice_u8 src) {
  size_t len4 = (dst.len < src.len ? dst.len : src.len) / 4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;

  size_t n = len4;
  while (n--) {
    uint8_t b0 = s[0];
    uint8_t b1 = s[1];
    uint8_t b2 = s[2];
    uint8_t b3 = s[3];
    d[0] = b2;
    d[1] = b1;
    d[2] = b0;
    d[3] = b3;
    s += 4;
    d += 4;
  }
  return len4 * 4;
}

void  //
wuffs_base__pixel_swizzler__prepare(wuffs_base__pixel_swizzler* p,
                                    wuffs_base__pixel_format dst_format,
                                    wuffs_base__slice_u8 dst_palette,
                                    wuffs_base__pixel_format src_format,
                                    wuffs_base__slice_u8 src_palette) {
  if (!p) {
    return;
  }

  // TODO: support many more formats.

  uint64_t (*func)(wuffs_base__slice_u8 dst, wuffs_base__slice_u8 dst_palette,
                   wuffs_base__slice_u8 src) = NULL;

  switch (src_format) {
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
      switch (dst_format) {
        case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL:
        case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_PREMUL:
        case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_BINARY:
          if (wuffs_base__slice_u8__copy_from_slice(dst_palette, src_palette) !=
              1024) {
            break;
          }
          func = wuffs_base__pixel_swizzler__copy_1_1;
          break;
        case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
        case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
        case WUFFS_BASE__PIXEL_FORMAT__BGRA_BINARY:
          if (wuffs_base__slice_u8__copy_from_slice(dst_palette, src_palette) !=
              1024) {
            break;
          }
          func = wuffs_base__pixel_swizzler__copy_4_1;
          break;
        case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
        case WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL:
        case WUFFS_BASE__PIXEL_FORMAT__RGBA_BINARY:
          if (wuffs_base__pixel_swizzler__swap_rgbx_bgrx(dst_palette,
                                                         src_palette) != 1024) {
            break;
          }
          func = wuffs_base__pixel_swizzler__copy_4_1;
          break;
        default:
          break;
      }
      break;

    default:
      break;
  }

  p->private_impl.func = func;
}

uint64_t  //
wuffs_base__pixel_swizzler__swizzle_packed(const wuffs_base__pixel_swizzler* p,
                                           wuffs_base__slice_u8 dst,
                                           wuffs_base__slice_u8 dst_palette,
                                           wuffs_base__slice_u8 src) {
  if (p && p->private_impl.func) {
    return (*(p->private_impl.func))(dst, dst_palette, src);
  }
  return 0;
}
