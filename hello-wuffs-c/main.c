// Copyright 2019 The Wuffs Authors.
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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef USE_WUFFS
uint32_t parse(char* p, size_t n) {
  uint32_t ret = 0;
  for (size_t i = 0; (i < n) && p[i]; i++) {
    ret = (10 * ret) + (p[i] - '0');
  }
  return ret;
}
#else  // #ifndef USE_WUFFS

// TODO: this 'rough edge' shouldn't be necessary. See
// https://github.com/google/wuffs/issues/24
#define WUFFS_CONFIG__MODULE__BASE

#define WUFFS_IMPLEMENTATION
#include "./parse.c"

uint32_t parse(char* p, size_t n) {
  wuffs_base__status status;
  wuffs_demo__parser* parser =
      (wuffs_demo__parser*)(malloc(sizeof__wuffs_demo__parser()));
  if (!parser) {
    printf("malloc failed\n");
    return 0;
  }

  status = wuffs_demo__parser__initialize(parser, sizeof__wuffs_demo__parser(),
                                          WUFFS_VERSION, 0);
  if (!wuffs_base__status__is_ok(&status)) {
    printf("initialize: %s\n", wuffs_base__status__message(&status));
    free(parser);
    return 0;
  }

  wuffs_base__io_buffer iobuf;
  iobuf.data.ptr = (uint8_t*)p;
  iobuf.data.len = n;
  iobuf.meta.wi = n;
  iobuf.meta.ri = 0;
  iobuf.meta.pos = 0;
  iobuf.meta.closed = true;

  status = wuffs_demo__parser__parse(parser, &iobuf);
  if (!wuffs_base__status__is_ok(&status)) {
    printf("parse: %s\n", wuffs_base__status__message(&status));
    free(parser);
    return 0;
  }

  uint32_t ret = wuffs_demo__parser__value(parser);
  free(parser);
  return ret;
}
#endif  // #ifndef USE_WUFFS

void run(char* p) {
  size_t n = strlen(p) + 1;  // +1 for the trailing NUL that ends a C string.
  uint32_t i = parse(p, n);
  printf("%" PRIu32 "\n", i);
}

int main(int argc, char** argv) {
  run("0");
  run("12");
  run("56789");
  run("4294967295");  // (1<<32) - 1, aka UINT32_MAX.
  run("4294967296");  // (1<<32).
  run("123456789012");
  return 0;
}
