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
zcat decodes gzip'ed data to stdout. It is similar to the standard /bin/zcat
program, except that this example program only reads from stdin. On Linux, it
also self-imposes a SECCOMP_MODE_STRICT sandbox. To run:

$cc zcat.c && ./a.out < ../../test/data/romeo.txt.gz; rm -f a.out

for a C compiler $cc, such as clang or gcc.
*/

#include <errno.h>
#include <unistd.h>

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// the release/c/wuffs-etc/etc.c code whitelist which parts of Wuffs to build.
// That C file contains the entire Wuffs standard library, implementing a
// variety of codecs and file formats. Without this macro definition, an
// optimizing compiler or linker may very well discard Wuffs code for unused
// codecs, but listing the Wuffs modules we use makes that process explicit.
// Preprocessing means that such code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__CRC32
#define WUFFS_CONFIG__MODULE__DEFLATE
#define WUFFS_CONFIG__MODULE__GZIP

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../release/c/wuffs-v0.2/wuffs-v0.2.c"
//
// To build this program against the development (instead of the release)
// editions, comment out the #include line above and uncomment the #include
// lines below.
//
// #include "../../gen/c/base.c"
// #include "../../gen/c/std/crc32.c"
// #include "../../gen/c/std/deflate.c"
// #include "../../gen/c/std/gzip.c"

#ifdef __linux__
#include <linux/prctl.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#define WUFFS_EXAMPLE_USE_SECCOMP
#endif

#ifndef DST_BUFFER_SIZE
#define DST_BUFFER_SIZE (16 * 1024)
#endif

#ifndef SRC_BUFFER_SIZE
#define SRC_BUFFER_SIZE (16 * 1024)
#endif

char dst_buffer[DST_BUFFER_SIZE];
char src_buffer[SRC_BUFFER_SIZE];

// ignore_return_value suppresses errors from -Wall -Werror.
static void ignore_return_value(int ignored) {}

static const char* decode() {
  wuffs_gzip__decoder dec = ((wuffs_gzip__decoder){});
  wuffs_gzip__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);

  while (true) {
    const int stdin_fd = 0;
    ssize_t n_src = read(stdin_fd, src_buffer, SRC_BUFFER_SIZE);
    if (n_src < 0) {
      if (errno != EINTR) {
        return strerror(errno);
      }
      continue;
    }

    wuffs_base__io_buffer src = ((wuffs_base__io_buffer){
        .ptr = src_buffer,
        .len = SRC_BUFFER_SIZE,
        .wi = n_src,
        .closed = n_src == 0,
    });
    wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);

    while (true) {
      wuffs_base__io_buffer dst =
          ((wuffs_base__io_buffer){.ptr = dst_buffer, .len = DST_BUFFER_SIZE});
      wuffs_base__io_writer dst_writer = wuffs_base__io_buffer__writer(&dst);
      wuffs_base__status s =
          wuffs_gzip__decoder__decode(&dec, dst_writer, src_reader);

      if (dst.wi) {
        // TODO: handle EINTR and other write errors; see "man 2 write".
        const int stdout_fd = 1;
        ignore_return_value(write(stdout_fd, dst_buffer, dst.wi));
      }

      if (s == WUFFS_BASE__STATUS_OK) {
        return NULL;
      }
      if (s == WUFFS_BASE__SUSPENSION_SHORT_READ) {
        break;
      }
      if (s != WUFFS_BASE__SUSPENSION_SHORT_WRITE) {
        return wuffs_gzip__status__string(s);
      }
    }
  }
}

int fail(const char* msg) {
  const int stderr_fd = 2;
  ignore_return_value(write(stderr_fd, msg, strnlen(msg, 4095)));
  ignore_return_value(write(stderr_fd, "\n", 1));
  return 1;
}

int main(int argc, char** argv) {
#ifdef WUFFS_EXAMPLE_USE_SECCOMP
  prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT);
#endif

  const char* msg = decode();
  int status = msg ? fail(msg) : 0;

#ifdef WUFFS_EXAMPLE_USE_SECCOMP
  // Call SYS_exit explicitly instead of SYS_exit_group implicitly.
  // SECCOMP_MODE_STRICT allows only the former.
  syscall(SYS_exit, status);
#endif
  return status;
}
