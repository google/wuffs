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

// ----------------

// Uncomment this line to test and bench libspng instead of libpng.
// #define WUFFS_MIMICLIB_USE_LIBSPNG_INSTEAD_OF_LIBPNG 1

#ifdef WUFFS_MIMICLIB_USE_LIBSPNG_INSTEAD_OF_LIBPNG

// We #include a foo.c file, not a foo.h file, as libspng is a "single file C
// library".
#include "/path/to/your/copy/of/github.com/randy408/libspng/spng/spng.c"

// We deliberately do not define the
// WUFFS_MIMICLIB_PNG_DOES_NOT_SUPPORT_QUIRK_IGNORE_CHECKSUM macro.

// libspng (version 0.6.2, released November 2020) calculates but does not
// verify the CRC-32 checksum on the final IDAT chunk. It also does not verify
// the Adler-32 checksum. After calling spng_decode_image, it ends in
// SPNG_STATE_EOI but not a later state such as SPNG_STATE_AFTER_IDAT.
//
// libspng commit 9c35dc3 "fix handling of SPNG_CTX_IGNORE_ADLER32", submitted
// December 2020, fixed Adler-32 but not CRC-32.
#define WUFFS_MIMICLIB_PNG_DOES_NOT_VERIFY_FINAL_IDAT_CHECKSUMS 1

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

  spng_ctx* ctx = spng_ctx_new(0);

  size_t i;
  for (i = 0; i < quirks_len; i++) {
    uint32_t q = quirks_ptr[i];
    if (q == WUFFS_BASE__QUIRK_IGNORE_CHECKSUM) {
      spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);
    }
  }

  if (spng_set_png_buffer(ctx, wuffs_base__io_buffer__reader_pointer(src),
                          wuffs_base__io_buffer__reader_length(src))) {
    ret = "mimic_png_decode: spng_set_png_buffer failed";
    goto cleanup0;
  }

  int fmt = 0;
  switch (pixfmt.repr) {
    case WUFFS_BASE__PIXEL_FORMAT__Y:
      fmt = SPNG_FMT_G8;
      break;
    case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      // libspng doesn't do BGRA8. RGBA8 is the closest approximation. We'll
      // fix it up later.
      fmt = SPNG_FMT_RGBA8;
      break;
    default:
      ret = "mimic_png_decode: unsupported pixfmt";
      goto cleanup0;
  }

  size_t n = 0;
  if (spng_decoded_image_size(ctx, fmt, &n)) {
    ret = "mimic_png_decode: spng_decoded_image_size failed";
    goto cleanup0;
  }
  if (n > wuffs_base__io_buffer__writer_length(dst)) {
    ret = "mimic_png_decode: image is too large";
    goto cleanup0;
  }

  uint8_t* dst_ptr = wuffs_base__io_buffer__writer_pointer(dst);
  if (spng_decode_image(ctx, dst_ptr, n, fmt, 0)) {
    ret = "mimic_png_decode: spng_decode_image failed";
    goto cleanup0;
  }
  dst->meta.wi += n;
  if (n_bytes_out) {
    *n_bytes_out += n;
  }

  // Fix up BGRA8 vs RGBA8.
  if (fmt == SPNG_FMT_RGBA8) {
    for (; n >= 4; n -= 4) {
      uint8_t swap = dst_ptr[0];
      dst_ptr[0] = dst_ptr[2];
      dst_ptr[2] = swap;
      dst_ptr += 4;
    }
  }

cleanup0:;
  spng_ctx_free(ctx);
  return ret;
}

#else  // WUFFS_MIMICLIB_USE_LIBSPNG_INSTEAD_OF_LIBPNG
#include "png.h"

#define WUFFS_MIMICLIB_PNG_DOES_NOT_SUPPORT_QUIRK_IGNORE_CHECKSUM 1

// We deliberately do not define the
// WUFFS_MIMICLIB_PNG_DOES_NOT_VERIFY_FINAL_IDAT_CHECKSUMS macro.

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

  size_t i;
  for (i = 0; i < quirks_len; i++) {
    uint32_t q = quirks_ptr[i];
    if (q == WUFFS_BASE__QUIRK_IGNORE_CHECKSUM) {
      // TODO: configure libpng to ignore Adler-32 and CRC-32 checksums.
      //
      // The libpng "simplified API" (based on the png_image struct type) does
      // not offer an equivalent of libpng's lower level API's
      // png_set_crc_action and png_set_option(etc, PNG_IGNORE_ADLER32, etc).
      // But the lower level API is complicated (hence why libpng introduced a
      // "simplified API" in the first place), especially trying to make the
      // equivalent of the png_image.format field work.
      //
      // Instead, we simply disable (via the
      // WUFFS_MIMICLIB_PNG_DOES_NOT_SUPPORT_QUIRK_IGNORE_CHECKSUM macro) any
      // tests or benches that try to use WUFFS_BASE__QUIRK_IGNORE_CHECKSUM.
    }
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
#endif  // WUFFS_MIMICLIB_USE_LIBSPNG_INSTEAD_OF_LIBPNG
