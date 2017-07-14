// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "zlib.h"

unsigned int mimic_flate_read_func(void* ctx, unsigned char** buf) {
  puffs_base_buf1* src = (puffs_base_buf1*)(ctx);
  *buf = src->ptr + src->ri;
  return src->wi - src->ri;
}

int mimic_flate_write_func(void* ctx, unsigned char* ptr, unsigned int len) {
  puffs_base_buf1* dst = (puffs_base_buf1*)(ctx);
  size_t n = len;
  if (n > dst->len - dst->wi) {
    return 1;
  }
  memmove(dst->ptr + dst->wi, ptr, n);
  dst->wi += n;
  return 0;
}

const char* mimic_flate_decode(puffs_base_buf1* dst, puffs_base_buf1* src) {
  const char* ret = NULL;
  uint8_t window[32 * 1024];

  z_stream z = {0};
  const int window_bits = 15;
  int ibi_err = inflateBackInit(&z, window_bits, window);
  if (ibi_err != Z_OK) {
    ret = "inflateBackInit failed";
    goto cleanup0;
  }

  int ib_err =
      inflateBack(&z, mimic_flate_read_func, src, mimic_flate_write_func, dst);
  if (ib_err != Z_STREAM_END) {
    ret = "inflateBack failed";
    goto cleanup1;
  }

  size_t readable = src->wi - src->ri;
  size_t r_remaining = z.avail_in;
  if (readable < r_remaining) {
    ret = "inconsistent avail_in";
    goto cleanup1;
  }
  src->ri += readable - r_remaining;

cleanup1:;
  int ibe_err = inflateBackEnd(&z);
  if ((ibe_err != Z_OK) && !ret) {
    ret = "inflateBackEnd failed";
  }

cleanup0:;
  return ret;
}

const char* mimic_gzip_decode(puffs_base_buf1* dst, puffs_base_buf1* src) {
  const char* ret = NULL;

  z_stream z = {0};
  const int window_bits = 15;
  // See inflateInit2 in the zlib manual, or in zlib.h, for details about this
  // magic gzip_instead_of_zlib constant.
  const int gzip_instead_of_zlib = 16;
  int ii2_err = inflateInit2(&z, window_bits | gzip_instead_of_zlib);
  if (ii2_err != Z_OK) {
    ret = "inflateInit2 failed";
    goto cleanup0;
  }

  z.avail_in = src->wi - src->ri;
  z.next_in = src->ptr + src->ri;
  z.avail_out = dst->len - dst->wi;
  z.next_out = dst->ptr + dst->wi;

  int i_err = inflate(&z, Z_NO_FLUSH);
  if (i_err != Z_STREAM_END) {
    ret = "inflate failed";
    goto cleanup1;
  }

  size_t readable = src->wi - src->ri;
  size_t r_remaining = z.avail_in;
  if (readable < r_remaining) {
    ret = "inconsistent avail_in";
    goto cleanup1;
  }
  src->ri += readable - r_remaining;

  size_t writable = dst->len - dst->wi;
  size_t w_remaining = z.avail_out;
  if (writable < w_remaining) {
    ret = "inconsistent avail_out";
    goto cleanup1;
  }
  dst->wi += writable - w_remaining;

cleanup1:;
  int ie_err = inflateEnd(&z);
  if ((ie_err != Z_OK) && !ret) {
    ret = "inflateEnd failed";
  }

cleanup0:;
  return ret;
}
