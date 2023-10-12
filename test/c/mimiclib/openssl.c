// Copyright 2023 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

#include "openssl/sha.h"

uint8_t global_mimiclib_sha256_unused_bitvec256[SHA256_DIGEST_LENGTH];

const char*  //
mimic_bench_sha256(wuffs_base__io_buffer* dst,
                   wuffs_base__io_buffer* src,
                   uint32_t wuffs_initialize_flags,
                   uint64_t wlimit,
                   uint64_t rlimit) {
  SHA256_CTX ctx;
  SHA256_Init(&ctx);

  while (src->meta.ri < src->meta.wi) {
    uint8_t* ptr = src->data.ptr + src->meta.ri;
    size_t len = src->meta.wi - src->meta.ri;
    if (len > 0x7FFFFFFF) {
      return "src length is too large";
    } else if (len > rlimit) {
      len = rlimit;
    }
    SHA256_Update(&ctx, ptr, len);
    src->meta.ri += len;
  }

  SHA256_Final(global_mimiclib_sha256_unused_bitvec256, &ctx);
  return NULL;
}

wuffs_base__bitvec256  //
mimic_sha256_one_shot_checksum_bitvec256(wuffs_base__slice_u8 data) {
  SHA256_CTX ctx;
  uint8_t results[SHA256_DIGEST_LENGTH];
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, data.ptr, data.len);
  SHA256_Final(results, &ctx);

  return wuffs_base__make_bitvec256(
      wuffs_base__peek_u64be__no_bounds_check(results + 0x18),
      wuffs_base__peek_u64be__no_bounds_check(results + 0x10),
      wuffs_base__peek_u64be__no_bounds_check(results + 0x08),
      wuffs_base__peek_u64be__no_bounds_check(results + 0x00));
}
