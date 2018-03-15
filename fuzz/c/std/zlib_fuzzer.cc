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

// Silence the nested slash-star warning for the next comment's command line.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcomment"

/*
This fuzzer (the fuzz function) is typically run indirectly, by a framework
such as https://github.com/google/oss-fuzz calling LLVMFuzzerTestOneInput.

When working on the fuzz implementation, or as a sanity check, defining
WUFFS_CONFIG__FUZZLIB_MAIN will let you manually run fuzz over a set of files:

g++ -DWUFFS_CONFIG__FUZZLIB_MAIN zlib_fuzzer.cc
./a.out ../../../test/data/*.zlib
rm -f ./a.out

It should print "PASS", amongst other information, and exit(0).
*/

#pragma clang diagnostic pop

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../gen/c/std/deflate.c"
#include "../../../gen/c/std/zlib.c"
#include "../fuzzlib/fuzzlib.cc"

void fuzz(wuffs_base__reader1 src_reader, uint32_t hash) {
  wuffs_zlib__status s;
  wuffs_zlib__decoder dec;
  wuffs_zlib__decoder__initialize(&dec, WUFFS_VERSION, 0);

  // Ignore the checksum for 99.99%-ish of all input. When fuzzers generate
  // random input, the checkum is very unlikely to match. Still, it's useful to
  // verify that checksumming does not lead to e.g. buffer overflows.
  wuffs_zlib__decoder__set_ignore_checksum(&dec, hash & 0xFFFF);

  const size_t dstbuf_size = 65536;
  uint8_t dstbuf[dstbuf_size];
  wuffs_base__buf1 dst = {.ptr = dstbuf, .len = dstbuf_size};
  wuffs_base__writer1 dst_writer = {.buf = &dst};

  while (true) {
    dst.wi = 0;
    s = wuffs_zlib__decoder__decode(&dec, dst_writer, src_reader);
    if (s != WUFFS_ZLIB__SUSPENSION_SHORT_WRITE) {
      break;
    }
    if (dst.wi == 0) {
      fprintf(stderr, "wuffs_zlib__decoder__decode made no progress\n");
      intentional_segfault();
    }
  }
}
