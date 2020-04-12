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

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// By #include'ing it "as is" without #define'ing WUFFS_IMPLEMENTATION, we use
// it as a "foo.h"-like header, instead of a "foo.c"-like implementation.
#if defined(WUFFS_IMPLEMENTATION)
#error "example/library should not #define WUFFS_IMPLEMENTATION"
#endif
#include "wuffs/release/c/wuffs-unsupported-snapshot.c"

#ifndef DST_BUFFER_ARRAY_SIZE
#define DST_BUFFER_ARRAY_SIZE 1024
#endif
uint8_t g_dst_buffer_array[DST_BUFFER_ARRAY_SIZE];

// src_ptr and src_len hold a gzip-encoded "Hello Wuffs."
//
// $ echo "Hello Wuffs." | gzip --no-name | xxd
// 00000000: 1f8b 0800 0000 0000 0003 f348 cdc9 c957  ...........H...W
// 00000010: 082f 4d4b 2bd6 e302 003c 8475 bb0d 0000  ./MK+....<.u....
// 00000020: 00                                       .
//
// Passing --no-name to the gzip command line also means to skip the timestamp,
// which means that its output is deterministic.
uint8_t g_src_array[] = {
    0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,  // 00..07
    0x00, 0x03, 0xf3, 0x48, 0xcd, 0xc9, 0xc9, 0x57,  // 08..0F
    0x08, 0x2f, 0x4d, 0x4b, 0x2b, 0xd6, 0xe3, 0x02,  // 10..17
    0x00, 0x3c, 0x84, 0x75, 0xbb, 0x0d, 0x00, 0x00,  // 18..1F
    0x00,                                            // 20..20
};
size_t g_src_len = 0x21;

#define WORK_BUFFER_ARRAY_SIZE \
  WUFFS_GZIP__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE
#if WORK_BUFFER_ARRAY_SIZE > 0
uint8_t g_work_buffer_array[WORK_BUFFER_ARRAY_SIZE];
#else
// Not all C/C++ compilers support 0-length arrays.
uint8_t g_work_buffer_array[1];
#endif

static const char*  //
decode() {
  wuffs_gzip__decoder* dec = wuffs_gzip__decoder__alloc();
  if (!dec) {
    return "out of memory";
  }

  wuffs_base__io_buffer dst =
      wuffs_base__ptr_u8__writer(&g_dst_buffer_array[0], DST_BUFFER_ARRAY_SIZE);

  static const bool closed = true;
  wuffs_base__io_buffer src =
      wuffs_base__ptr_u8__reader(&g_src_array[0], g_src_len, closed);

  wuffs_base__status status = wuffs_gzip__decoder__transform_io(
      dec, &dst, &src,
      wuffs_base__make_slice_u8(&g_work_buffer_array[0],
                                WORK_BUFFER_ARRAY_SIZE));
  free(dec);

  if (!wuffs_base__status__is_ok(&status)) {
    return wuffs_base__status__message(&status);
  }
  fwrite(dst.data.ptr, sizeof(uint8_t), dst.meta.wi, stdout);
  return NULL;
}

int  //
main(int argc, char** argv) {
  const char* status_msg = decode();
  if (status_msg) {
    fprintf(stderr, "%s\n", status_msg);
    return 1;
  }
  return 0;
}
