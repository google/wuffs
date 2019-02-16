// Copyright 2018 The Wuffs Authors.
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

// Silence the nested slash-star warning for the next comment's command line.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcomment"

/*
This fuzzer (the fuzz function) is typically run indirectly, by a framework
such as https://github.com/google/oss-fuzz calling LLVMFuzzerTestOneInput.

When working on the fuzz implementation, or as a sanity check, defining
WUFFS_CONFIG__FUZZLIB_MAIN will let you manually run fuzz over a set of files:

gcc -DWUFFS_CONFIG__FUZZLIB_MAIN zlib_fuzzer.c
./a.out ../../../test/data/*.zlib
rm -f ./a.out

It should print "PASS", amongst other information, and exit(0).
*/

#pragma clang diagnostic pop

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../fuzzlib/fuzzlib.c"

#define DST_BUFFER_SIZE 65536

// Wuffs allows either statically or dynamically allocated work buffers. This
// program exercises static allocation.
#define WORK_BUFFER_SIZE WUFFS_ZLIB__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE
#if WORK_BUFFER_SIZE > 0
uint8_t work_buffer[WORK_BUFFER_SIZE];
#else
// Not all C/C++ compilers support 0-length arrays.
uint8_t work_buffer[1];
#endif

const char* fuzz(wuffs_base__io_reader src_reader, uint32_t hash) {
  wuffs_zlib__decoder dec;
  const char* status = wuffs_zlib__decoder__initialize(
      &dec, sizeof dec, WUFFS_VERSION,
      (hash & 1) ? WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED : 0);
  if (status) {
    return status;
  }

  // Ignore the checksum for 99.99%-ish of all input. When fuzzers generate
  // random input, the checkum is very unlikely to match. Still, it's useful to
  // verify that checksumming does not lead to e.g. buffer overflows.
  wuffs_zlib__decoder__set_ignore_checksum(&dec, hash & 0xFFFE);

  uint8_t dst_buffer[DST_BUFFER_SIZE];
  wuffs_base__io_buffer dst = ((wuffs_base__io_buffer){
      .data = ((wuffs_base__slice_u8){
          .ptr = dst_buffer,
          .len = DST_BUFFER_SIZE,
      }),
  });
  wuffs_base__io_writer dst_writer = wuffs_base__io_buffer__writer(&dst);

  while (true) {
    dst.meta.wi = 0;
    status = wuffs_zlib__decoder__decode_io_writer(&dec, dst_writer, src_reader,
                                                   ((wuffs_base__slice_u8){
                                                       .ptr = work_buffer,
                                                       .len = WORK_BUFFER_SIZE,
                                                   }));
    if (status != wuffs_base__suspension__short_write) {
      break;
    }
    if (dst.meta.wi == 0) {
      fprintf(stderr,
              "wuffs_zlib__decoder__decode_io_writer made no progress\n");
      intentional_segfault();
    }
  }
  return status;
}
