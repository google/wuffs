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
peterface decodes pjw's iconic face, stored as a GIF image. To run:

$cc peterface.c && ./a.out; rm -f a.out

for a C compiler $cc, such as clang or gcc.
*/

// TODO: delete this program, as example/gifplayer is a more interesting
// example of Wuffs' GIF codec. This program's seccomp code should move
// somewhere before we delete it, though, as that's still a feature worth
// demonstrating. The gifplayer program can't use SECCOMP_MODE_STRICT, as it
// needs to sleep between animation frames.

#include <string.h>
#include <unistd.h>

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../gen/c/std/gif.c"

#ifdef __linux__
#include <linux/prctl.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#define WUFFS_EXAMPLE_USE_SECCOMP
#endif

#define DST_BUFFER_SIZE (1024 * 1024)
#define PRINT_BUFFER_SIZE (1024)

uint8_t pjw_ptr[];
size_t pjw_len;

// ignore_return_value suppresses errors from -Wall -Werror.
static void ignore_return_value(int ignored) {}

static const char* decode() {
  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);

  wuffs_base__buf1 src = {
      .ptr = pjw_ptr, .len = pjw_len, .wi = pjw_len, .closed = true};
  wuffs_base__reader1 src_reader = {.buf = &src};

  wuffs_base__image_config ic = {{0}};
  wuffs_gif__status s =
      wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
  if (s) {
    return wuffs_gif__status__string(s);
  }
  uint32_t width = wuffs_base__image_config__width(&ic);
  uint32_t height = wuffs_base__image_config__height(&ic);
  if ((width > PRINT_BUFFER_SIZE - 1) ||
      (wuffs_base__image_config__pixbuf_size(&ic) > DST_BUFFER_SIZE)) {
    return "image is too large";
  }

  uint8_t dst_buffer[DST_BUFFER_SIZE];
  wuffs_base__buf1 dst = {.ptr = dst_buffer, .len = DST_BUFFER_SIZE};
  wuffs_base__writer1 dst_writer = {.buf = &dst};
  s = wuffs_gif__decoder__decode_frame(&dec, dst_writer, src_reader);
  if (s) {
    return wuffs_gif__status__string(s);
  }

  uint8_t buf[PRINT_BUFFER_SIZE];
  uint8_t* p = dst.ptr;
  uint32_t y;
  for (y = 0; y < height; y++) {
    uint32_t x;
    for (x = 0; x < width; x++) {
      buf[x] = *p++ ? '-' : '8';
    }
    buf[width] = '\n';
    ignore_return_value(write(1, buf, width + 1));
  }
  return NULL;
}

int main(int argc, char** argv) {
#ifdef WUFFS_EXAMPLE_USE_SECCOMP
  prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT);
#endif

  const char* status_msg = decode();
  int status = 0;
  if (status_msg) {
    status = 1;
    ignore_return_value(write(2, status_msg, strnlen(status_msg, 4095)));
    ignore_return_value(write(2, "\n", 1));
  }

#ifdef WUFFS_EXAMPLE_USE_SECCOMP
  // Call SYS_exit explicitly instead of SYS_exit_group implicitly.
  // SECCOMP_MODE_STRICT allows only the former.
  syscall(SYS_exit, status);
#endif
  return status;
}

/*
The remainder of this C program was generated from running this Go program via:

go run x.go < ../../test/testdata/pjw-thumbnail.gif

----
package main

import (
        "fmt"
        "io/ioutil"
        "log"
        "os"
)

func main() {
        data, err := ioutil.ReadAll(os.Stdin)
        if err != nil {
                log.Fatal(err)
        }
        fmt.Printf("uint8_t pjw_ptr[] = {\n")
        for _, c := range data {
                fmt.Printf("0x%02X,", c)
        }
        fmt.Printf("\n};\n")
        fmt.Printf("size_t pjw_len = %d;\n", len(data))
}
----

and piping the result through clang-format.
*/

uint8_t pjw_ptr[] = {
    0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x20, 0x00, 0x20, 0x00, 0xf0, 0x01,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x21, 0xf9, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x00, 0x02, 0x75, 0x8c, 0x8f, 0xa9, 0xcb, 0x0b, 0x0f, 0x5f, 0x9b, 0x28,
    0x5a, 0x49, 0x19, 0x08, 0xf7, 0x66, 0xb5, 0x25, 0x1d, 0xf6, 0x35, 0x23,
    0x59, 0x8a, 0x67, 0x16, 0x6a, 0xab, 0x4b, 0x9d, 0x68, 0xd5, 0x9a, 0xaf,
    0x5a, 0xdb, 0x9e, 0x83, 0xcd, 0x86, 0x9c, 0xe3, 0x44, 0x0e, 0x9b, 0x22,
    0x30, 0xe8, 0x39, 0x8e, 0x70, 0x43, 0xc8, 0xef, 0xc8, 0x73, 0x56, 0x7e,
    0xc2, 0x25, 0x48, 0xea, 0xa0, 0x76, 0x60, 0x34, 0xa2, 0xc4, 0xe8, 0x03,
    0x3d, 0xaf, 0xdb, 0x09, 0x32, 0x69, 0x89, 0xb9, 0xbe, 0xd5, 0xf0, 0x74,
    0x6d, 0xb5, 0x3d, 0x95, 0xee, 0x77, 0x94, 0xd3, 0x4e, 0x79, 0xd3, 0xd8,
    0x14, 0xe9, 0x5b, 0xa6, 0x47, 0x16, 0xe4, 0xc7, 0xd4, 0x57, 0x12, 0x02,
    0x24, 0x38, 0x76, 0x23, 0x08, 0x16, 0xb8, 0xf8, 0x98, 0xd6, 0x50, 0x00,
    0x00, 0x3b,
};
size_t pjw_len = 158;
