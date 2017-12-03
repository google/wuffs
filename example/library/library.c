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

/*
library exercises the software libraries built by `wuffs genlib`.

To exercise the static library:

$cc -static -I../../.. library.c ../../gen/lib/c/$cc-static/libwuffs.a
./a.out
rm -f a.out

To exercise the dynamic library:

$cc -I../../.. library.c -L../../gen/lib/c/$cc-dynamic -lwuffs
LD_LIBRARY_PATH=../../gen/lib/c/$cc-dynamic ./a.out
rm -f a.out

for a C compiler $cc, such as clang or gcc.
*/

#include <unistd.h>

#include "wuffs/gen/h/std/deflate.h"

#define DST_BUFFER_SIZE (1024 * 1024)

// lgtm_ptr and lgtm_len hold a deflate-encoded "LGTM" message.
uint8_t lgtm_ptr[] = {
    0xf3, 0xc9, 0xcf, 0xcf, 0x2e, 0x56, 0x48, 0xcf, 0xcf, 0x4f,
    0x51, 0x28, 0xc9, 0x57, 0xc8, 0x4d, 0xd5, 0xe3, 0x02, 0x00,
};
size_t lgtm_len = 20;

// ignore_return_value suppresses errors from -Wall -Werror.
static void ignore_return_value(int ignored) {}

static const char* decode() {
  uint8_t dst_buffer[DST_BUFFER_SIZE];
  wuffs_base__buf1 dst = {.ptr = dst_buffer, .len = DST_BUFFER_SIZE};
  wuffs_base__buf1 src = {
      .ptr = lgtm_ptr, .len = lgtm_len, .wi = lgtm_len, .closed = true};
  wuffs_base__writer1 dst_writer = {.buf = &dst};
  wuffs_base__reader1 src_reader = {.buf = &src};

  wuffs_deflate__decoder dec;
  wuffs_deflate__decoder__initialize(&dec, WUFFS_VERSION, 0);
  wuffs_deflate__status s =
      wuffs_deflate__decoder__decode(&dec, dst_writer, src_reader);
  if (s) {
    return wuffs_deflate__status__string(s);
  }
  ignore_return_value(write(1, dst.ptr, dst.wi));
  return NULL;
}

int main(int argc, char** argv) {
  const char* status_msg = decode();
  int status = 0;
  if (status_msg) {
    status = 1;
    ignore_return_value(write(2, status_msg, strnlen(status_msg, 4095)));
    ignore_return_value(write(2, "\n", 1));
  }
  return status;
}
