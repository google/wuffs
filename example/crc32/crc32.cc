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

/*
crc32 prints the CRC-32 checksum (using the IEEE polynomial) of stdin. It is
similar to the standard /usr/bin/crc32 program, except that this example
program only reads from stdin.

This example program differs from the other example Wuffs programs in that it
is written in C++, not C.

$CXX crc32.cc && ./a.out < ../../README.md; rm -f a.out

for a C++ compiler $CXX, such as clang++ or g++.
*/

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C++ file.
#include "../../gen/c/base.c"
#include "../../gen/c/std/crc32.c"

#ifndef SRC_BUFFER_SIZE
#define SRC_BUFFER_SIZE (16 * 1024)
#endif

uint8_t src_buffer[SRC_BUFFER_SIZE];

int main(int argc, char** argv) {
  wuffs_crc32__ieee_hasher h = ((wuffs_crc32__ieee_hasher){});
  wuffs_crc32__ieee_hasher__check_wuffs_version(&h, sizeof h, WUFFS_VERSION);

  while (true) {
    const int stdin_fd = 0;
    ssize_t n = read(stdin_fd, src_buffer, SRC_BUFFER_SIZE);
    if (n < 0) {
      if (errno != EINTR) {
        fprintf(stderr, "%s\n", strerror(errno));
        return 1;
      }
      continue;
    }

    uint32_t checksum =
        wuffs_crc32__ieee_hasher__update(&h, ((wuffs_base__slice_u8){
                                                 .ptr = src_buffer,
                                                 .len = static_cast<size_t>(n),
                                             }));

    if (n == 0) {
      printf("%08" PRIx32 "\n", checksum);
      return 0;
    }
  }
}
