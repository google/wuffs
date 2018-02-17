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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef WUFFS_BASE_HEADER_H
#error "Wuffs' .c or .h files need to be included before this file"
#endif

volatile int* intentional_segfault_ptr = NULL;

void intentional_segfault() {
  *intentional_segfault_ptr = 0;
}

void fuzz(wuffs_base__reader1 src_reader, uint32_t hash);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Hash input as per https://en.wikipedia.org/wiki/Jenkins_hash_function
  size_t i = 0;
  uint32_t hash = 0;
  while (i != size) {
    hash += data[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;

  wuffs_base__buf1 src = {.ptr = (uint8_t*)(data),
                          .len = size,
                          .wi = size,
                          .ri = 0,
                          .closed = true};
  fuzz((wuffs_base__reader1){.buf = &src}, hash);
  return 0;
}

#ifdef WUFFS_CONFIG__FUZZLIB_MAIN

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char** argv) {
  int i;
  for (i = 1; i < argc; i++) {
    printf("Processing %s\n", argv[i]);

    struct stat z;
    int fd = open(argv[i], O_RDONLY, 0);
    if (fd == -1) {
      fprintf(stderr, "FAIL: open: %s\n", strerror(errno));
      return 1;
    }
    if (fstat(fd, &z)) {
      fprintf(stderr, "FAIL: fstat: %s\n", strerror(errno));
      return 1;
    }
    if ((z.st_size < 0) || (0x7FFFFFFF < z.st_size)) {
      fprintf(stderr, "FAIL: file size out of bounds");
      return 1;
    }
    size_t n = z.st_size;
    void* data = mmap(NULL, n, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
      fprintf(stderr, "FAIL: mmap: %s\n", strerror(errno));
      return 1;
    }

    int fuzz = LLVMFuzzerTestOneInput((const uint8_t*)(data), n);
    if (fuzz) {
      return fuzz;
    }

    if (munmap(data, n)) {
      fprintf(stderr, "FAIL: mmap: %s\n", strerror(errno));
      return 1;
    }
    if (close(fd)) {
      fprintf(stderr, "FAIL: close: %s\n", strerror(errno));
      return 1;
    }
  }
  printf("PASS: %d files processed\n", argc > 1 ? argc - 1 : 0);
  return 0;
}

#endif  // WUFFS_CONFIG__FUZZLIB_MAIN
