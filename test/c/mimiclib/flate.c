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
  int ibi_err = inflateBackInit(&z, 15, window);
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
  size_t remaining = z.avail_in;
  if (readable < remaining) {
    ret = "inconsistent avail_in";
    goto cleanup1;
  }
  src->ri += readable - remaining;

cleanup1:;
  int ibe_err = inflateBackEnd(&z);
  if ((ibe_err != Z_OK) && !ret) {
    ret = "inflateBackEnd failed";
  }

cleanup0:;
  return ret;
}
