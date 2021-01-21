// Copyright 2020 The Wuffs Authors.
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

#include "png.h"

const char*  //
mimic_png_decode(uint64_t* n_bytes_out,
                 wuffs_base__io_buffer* dst,
                 uint32_t wuffs_initialize_flags,
                 wuffs_base__pixel_format pixfmt,
                 uint32_t* quirks_ptr,
                 size_t quirks_len,
                 wuffs_base__io_buffer* src) {
  wuffs_base__io_buffer dst_fallback =
      wuffs_base__slice_u8__writer(g_mimiclib_scratch_slice_u8);
  if (!dst) {
    dst = &dst_fallback;
  }

  const char* ret = NULL;

  png_image pi;
  memset(&pi, 0, (sizeof pi));
  pi.version = PNG_IMAGE_VERSION;

  if (!png_image_begin_read_from_memory(&pi, src->data.ptr + src->meta.ri,
                                        src->meta.wi - src->meta.ri)) {
    ret = "mimic_png_decode: png_image_begin_read_from_memory failed";
    goto cleanup0;
  }

  switch (pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__Y:
      pi.format = PNG_FORMAT_GRAY;
      break;
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      pi.format = PNG_FORMAT_BGRA;
      break;
    default:
      ret = "mimic_png_decode: unsupported pixfmt";
      goto cleanup0;
  }

  size_t n = PNG_IMAGE_SIZE(pi);
  if (n > wuffs_base__io_buffer__writer_length(dst)) {
    ret = "mimic_png_decode: image is too large";
    goto cleanup0;
  }
  if (!png_image_finish_read(
          &pi, NULL, wuffs_base__io_buffer__writer_pointer(dst), 0, NULL)) {
    ret = "mimic_png_decode: png_image_finish_read failed";
    goto cleanup0;
  }
  dst->meta.wi += n;
  if (n_bytes_out) {
    *n_bytes_out += n;
  }

cleanup0:;
  png_image_free(&pi);
  return ret;
}
