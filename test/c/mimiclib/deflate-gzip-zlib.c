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

#include "zlib.h"

uint32_t global_mimiclib_deflate_unused_u32;

const char* mimic_bench_adler32(wuffs_base__buf1* dst,
                                wuffs_base__buf1* src,
                                uint64_t wlimit,
                                uint64_t rlimit) {
  // TODO: don't ignore wlimit and rlimit.
  uint8_t* ptr = src->ptr + src->ri;
  size_t len = src->wi - src->ri;
  if (len > 0x7FFFFFFF) {
    return "src length is too large";
  }
  global_mimiclib_deflate_unused_u32 = adler32(0L, ptr, len);
  src->ri = src->wi;
  return NULL;
}

const char* mimic_bench_crc32(wuffs_base__buf1* dst,
                              wuffs_base__buf1* src,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  // TODO: don't ignore wlimit and rlimit.
  uint8_t* ptr = src->ptr + src->ri;
  size_t len = src->wi - src->ri;
  if (len > 0x7FFFFFFF) {
    return "src length is too large";
  }
  global_mimiclib_deflate_unused_u32 = crc32(0L, ptr, len);
  src->ri = src->wi;
  return NULL;
}

unsigned int mimic_deflate_read_func(void* ctx, unsigned char** buf) {
  wuffs_base__buf1* src = (wuffs_base__buf1*)(ctx);
  *buf = src->ptr + src->ri;
  return src->wi - src->ri;
}

int mimic_deflate_write_func(void* ctx, unsigned char* ptr, unsigned int len) {
  wuffs_base__buf1* dst = (wuffs_base__buf1*)(ctx);
  size_t n = len;
  if (n > dst->len - dst->wi) {
    return 1;
  }
  memmove(dst->ptr + dst->wi, ptr, n);
  dst->wi += n;
  return 0;
}

const char* mimic_deflate_decode(wuffs_base__buf1* dst,
                                 wuffs_base__buf1* src,
                                 uint64_t wlimit,
                                 uint64_t rlimit) {
  // TODO: don't ignore wlimit and rlimit.
  const char* ret = NULL;
  uint8_t window[32 * 1024];

  z_stream z = {0};
  const int window_bits = 15;
  int ibi_err = inflateBackInit(&z, window_bits, window);
  if (ibi_err != Z_OK) {
    ret = "inflateBackInit failed";
    goto cleanup0;
  }

  int ib_err = inflateBack(&z, mimic_deflate_read_func, src,
                           mimic_deflate_write_func, dst);
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

const char* mimic_gzip_zlib_decode(wuffs_base__buf1* dst,
                                   wuffs_base__buf1* src,
                                   uint64_t wlimit,
                                   uint64_t rlimit,
                                   bool gzip_instead_of_zlib) {
  // TODO: don't ignore wlimit and rlimit.
  const char* ret = NULL;

  z_stream z = {0};
  int window_bits = 15;
  if (gzip_instead_of_zlib) {
    // See inflateInit2 in the zlib manual, or in zlib.h, for details about
    // this magic constant.
    window_bits |= 16;
  }
  int ii2_err = inflateInit2(&z, window_bits);
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

const char* mimic_gzip_decode(wuffs_base__buf1* dst,
                              wuffs_base__buf1* src,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  return mimic_gzip_zlib_decode(dst, src, wlimit, rlimit, true);
}

const char* mimic_zlib_decode(wuffs_base__buf1* dst,
                              wuffs_base__buf1* src,
                              uint64_t wlimit,
                              uint64_t rlimit) {
  return mimic_gzip_zlib_decode(dst, src, wlimit, rlimit, false);
}
