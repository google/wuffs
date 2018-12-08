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

static uint64_t  //
wuffs_base__pixel_swizzler__copy_1_1(wuffs_base__slice_u8 dst,
                                     wuffs_base__slice_u8 src) {
  return wuffs_base__slice_u8__copy_from_slice(dst, src);
}

void  //
wuffs_base__pixel_swizzler__initialize(wuffs_base__pixel_swizzler* p,
                                       wuffs_base__pixel_format dst_format,
                                       wuffs_base__pixel_format src_format) {
  if (!p) {
    return;
  }

  // TODO: support many more formats.

  uint64_t (*func)(wuffs_base__slice_u8 dst, wuffs_base__slice_u8 src) = NULL;

  switch (src_format) {
    case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL:
      switch (dst_format) {
        case WUFFS_BASE__PIXEL_FORMAT__INDEXED__BGRA_NONPREMUL:
          func = wuffs_base__pixel_swizzler__copy_1_1;
          break;
        case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
        case WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL:
          // TODO: implement.
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
wuffs_base__pixel_swizzler__swizzle_packed(wuffs_base__pixel_swizzler* p,
                                           wuffs_base__slice_u8 dst,
                                           wuffs_base__slice_u8 src) {
  if (p && p->private_impl.func) {
    return (*(p->private_impl.func))(dst, src);
  }
  return 0;
}
