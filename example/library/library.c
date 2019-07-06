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

// ----------------

/*
library exercises the software libraries built by `wuffs genlib`.

To exercise the static library:

$CC -static -I../../.. library.c ../../gen/lib/c/$CC-static/libwuffs.a
./a.out
rm -f a.out

To exercise the dynamic library:

$CC -I../../.. library.c -L../../gen/lib/c/$CC-dynamic -lwuffs
LD_LIBRARY_PATH=../../gen/lib/c/$CC-dynamic ./a.out
rm -f a.out

for a C compiler $CC, such as clang or gcc.
*/

#include <stdio.h>
#include <stdlib.h>

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// By #include'ing it "as is" without #define'ing WUFFS_IMPLEMENTATION, we use
// it as a "foo.h"-like header, instead of a "foo.c"-like implementation.
#if defined(WUFFS_IMPLEMENTATION)
#error "example/library should not #define WUFFS_IMPLEMENTATION"
#endif
#include "wuffs/release/c/wuffs-unsupported-snapshot.c"

#ifndef DST_BUFFER_SIZE
#define DST_BUFFER_SIZE 1024
#endif
uint8_t dst_buffer[DST_BUFFER_SIZE];

// src_ptr and src_len hold a gzip-encoded "Hello Wuffs."
//
// $ echo "Hello Wuffs." | gzip --no-name | xxd
// 00000000: 1f8b 0800 0000 0000 0003 f348 cdc9 c957  ...........H...W
// 00000010: 082f 4d4b 2bd6 e302 003c 8475 bb0d 0000  ./MK+....<.u....
// 00000020: 00                                       .
//
// Passing --no-name to the gzip command line also means to skip the timestamp,
// which means that its output is deterministic.
uint8_t src_ptr[] = {
    0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,  // 00..07
    0x00, 0x03, 0xf3, 0x48, 0xcd, 0xc9, 0xc9, 0x57,  // 08..0F
    0x08, 0x2f, 0x4d, 0x4b, 0x2b, 0xd6, 0xe3, 0x02,  // 10..17
    0x00, 0x3c, 0x84, 0x75, 0xbb, 0x0d, 0x00, 0x00,  // 18..1F
    0x00,                                            // 20..20
};
size_t src_len = 0x21;

#define WORK_BUFFER_SIZE WUFFS_GZIP__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE
#if WORK_BUFFER_SIZE > 0
uint8_t work_buffer[WORK_BUFFER_SIZE];
#else
// Not all C/C++ compilers support 0-length arrays.
uint8_t work_buffer[1];
#endif

static const char* decode() {
  wuffs_base__io_buffer dst;
  dst.data.ptr = dst_buffer;
  dst.data.len = DST_BUFFER_SIZE;
  dst.meta.wi = 0;
  dst.meta.ri = 0;
  dst.meta.pos = 0;
  dst.meta.closed = false;

  wuffs_base__io_buffer src;
  src.data.ptr = src_ptr;
  src.data.len = src_len;
  src.meta.wi = src_len;
  src.meta.ri = 0;
  src.meta.pos = 0;
  src.meta.closed = true;

  wuffs_gzip__decoder* dec =
      (wuffs_gzip__decoder*)(calloc(sizeof__wuffs_gzip__decoder(), 1));
  if (!dec) {
    return "out of memory";
  }
  const char* status = wuffs_gzip__decoder__initialize(
      dec, sizeof__wuffs_gzip__decoder(), WUFFS_VERSION,
      WUFFS_INITIALIZE__ALREADY_ZEROED);
  if (status) {
    free(dec);
    return status;
  }
  status = wuffs_gzip__decoder__decode_io_writer(
      dec, &dst, &src,
      wuffs_base__make_slice_u8(work_buffer, WORK_BUFFER_SIZE));
  if (status) {
    free(dec);
    return status;
  }
  fwrite(dst.data.ptr, sizeof(uint8_t), dst.meta.wi, stdout);
  free(dec);
  return NULL;
}

int main(int argc, char** argv) {
  const char* status = decode();
  if (status) {
    fprintf(stderr, "%s\n", status);
    return 1;
  }
  return 0;
}
