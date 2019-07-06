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

/*
zcat decodes gzip'ed data to stdout. It is similar to the standard /bin/zcat
program, except that this example program only reads from stdin. On Linux, it
also self-imposes a SECCOMP_MODE_STRICT sandbox. To run:

$CC zcat.c && ./a.out < ../../test/data/romeo.txt.gz; rm -f a.out

for a C compiler $CC, such as clang or gcc.
*/

#include <errno.h>
#include <unistd.h>

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c whitelist which parts of Wuffs to build. That file contains
// the entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__CRC32
#define WUFFS_CONFIG__MODULE__DEFLATE
#define WUFFS_CONFIG__MODULE__GZIP

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../release/c/wuffs-unsupported-snapshot.c"

#if defined(__linux__)
#include <linux/prctl.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#define WUFFS_EXAMPLE_USE_SECCOMP
#endif

#ifndef DST_BUFFER_SIZE
#define DST_BUFFER_SIZE (128 * 1024)
#endif

#ifndef SRC_BUFFER_SIZE
#define SRC_BUFFER_SIZE (128 * 1024)
#endif

#define WORK_BUFFER_SIZE WUFFS_GZIP__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE

uint8_t dst_buffer[DST_BUFFER_SIZE];
uint8_t src_buffer[SRC_BUFFER_SIZE];
#if WORK_BUFFER_SIZE > 0
uint8_t work_buffer[WORK_BUFFER_SIZE];
#else
// Not all C/C++ compilers support 0-length arrays.
uint8_t work_buffer[1];
#endif

// ignore_return_value suppresses errors from -Wall -Werror.
static void ignore_return_value(int ignored) {}

static const char* decode() {
  wuffs_gzip__decoder dec;
  const char* status =
      wuffs_gzip__decoder__initialize(&dec, sizeof dec, WUFFS_VERSION, 0);
  if (status) {
    return status;
  }

  wuffs_base__io_buffer dst;
  dst.data.ptr = dst_buffer;
  dst.data.len = DST_BUFFER_SIZE;
  dst.meta.wi = 0;
  dst.meta.ri = 0;
  dst.meta.pos = 0;
  dst.meta.closed = false;

  wuffs_base__io_buffer src;
  src.data.ptr = src_buffer;
  src.data.len = SRC_BUFFER_SIZE;
  src.meta.wi = 0;
  src.meta.ri = 0;
  src.meta.pos = 0;
  src.meta.closed = false;

  while (true) {
    const int stdin_fd = 0;
    ssize_t n =
        read(stdin_fd, src.data.ptr + src.meta.wi, src.data.len - src.meta.wi);
    if (n < 0) {
      if (errno != EINTR) {
        return strerror(errno);
      }
      continue;
    }
    src.meta.wi += n;
    if (n == 0) {
      src.meta.closed = true;
    }

    while (true) {
      status = wuffs_gzip__decoder__decode_io_writer(
          &dec, &dst, &src,
          wuffs_base__make_slice_u8(work_buffer, WORK_BUFFER_SIZE));

      if (dst.meta.wi) {
        // TODO: handle EINTR and other write errors; see "man 2 write".
        const int stdout_fd = 1;
        ignore_return_value(write(stdout_fd, dst_buffer, dst.meta.wi));
        dst.meta.ri = dst.meta.wi;
        wuffs_base__io_buffer__compact(&dst);
      }

      if (status == wuffs_base__suspension__short_read) {
        break;
      }
      if (status == wuffs_base__suspension__short_write) {
        continue;
      }
      return status;
    }

    wuffs_base__io_buffer__compact(&src);
    if (src.meta.wi == src.data.len) {
      return "internal error: no I/O progress possible";
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
#if defined(WUFFS_EXAMPLE_USE_SECCOMP)
  prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT);
#endif

  const char* status = decode();
  int status_code = status ? fail(status) : 0;

#if defined(WUFFS_EXAMPLE_USE_SECCOMP)
  // Call SYS_exit explicitly instead of SYS_exit_group implicitly.
  // SECCOMP_MODE_STRICT allows only the former.
  syscall(SYS_exit, status_code);
#endif
  return status_code;
}
