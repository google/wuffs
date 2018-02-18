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
gifplayer prints an ASCII representation of the GIF image read from stdin. To
play Eadweard Muybridge's iconic galloping horse animation, run:

$cc gifplayer.c && ./a.out < ../../test/testdata/muybridge.gif; rm -f a.out

for a C compiler $cc, such as clang or gcc.
*/

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../gen/c/std/gif.c"

// Limit the input GIF image to (64 MiB - 1 byte) compressed and 4096 × 4096
// pixels uncompressed. This is a limitation of this example program (which
// uses the Wuffs standard library), not a limitation of Wuffs per se.
//
// We keep the whole input in memory, instead of one-pass stream processing,
// because playing a looping animation requires re-winding the input.
#define SRC_BUF_SIZE (64 * 1024 * 1024)
#define MAX_DIMENSION (4096)

uint8_t src_buf[SRC_BUF_SIZE] = {0};
size_t src_len = 0;
uint8_t* dst_buf = NULL;
size_t dst_len = 0;
uint8_t* print_buf = NULL;
size_t print_len = 0;

bool seen_num_loops = false;
uint32_t num_loops_remaining = 0;

const char* read_stdin() {
  while (src_len < SRC_BUF_SIZE) {
    const int stdin_fd = 0;
    ssize_t n = read(stdin_fd, src_buf + src_len, SRC_BUF_SIZE - src_len);
    if (n > 0) {
      src_len += n;
    } else if (n == 0) {
      return NULL;
    } else if (errno == EINTR) {
      // No-op.
    } else {
      return strerror(errno);
    }
  }
  return "input is too large";
}

const char* play() {
  wuffs_gif__decoder dec;
  wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);

  wuffs_base__buf1 src = {
      .ptr = src_buf, .len = src_len, .wi = src_len, .closed = true};
  wuffs_base__reader1 src_reader = {.buf = &src};

  wuffs_base__image_config ic = {{0}};
  wuffs_gif__status s =
      wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
  if (s) {
    return wuffs_gif__status__string(s);
  }
  if (!wuffs_base__image_config__valid(&ic)) {
    return "invalid image configuration";
  }
  uint32_t width = wuffs_base__image_config__width(&ic);
  uint32_t height = wuffs_base__image_config__height(&ic);
  if ((width > MAX_DIMENSION) || (height > MAX_DIMENSION)) {
    return "image dimensions are too large";
  }
  if (!dst_buf) {
    dst_len = wuffs_base__image_config__pixbuf_size(&ic);
    dst_buf = malloc(dst_len);
    if (!dst_buf) {
      return "could not allocate dst buffer";
    }
    uint64_t plen = 1 + ((uint64_t)(width) + 1) * (uint64_t)(height);
    if (plen <= (uint64_t)SIZE_MAX) {
      print_len = (size_t)plen;
      print_buf = malloc(print_len);
    }
    if (!print_buf) {
      return "could not allocate print buffer";
    }
  }

  if (!seen_num_loops) {
    seen_num_loops = true;
    // TODO: provide API for getting num_loops.
    num_loops_remaining = dec.private_impl.f_num_loops;
  }

  while (true) {
    wuffs_base__buf1 dst = {.ptr = dst_buf, .len = dst_len};
    wuffs_base__writer1 dst_writer = {.buf = &dst};
    // TODO: provide API and support for when the frame rect is different from
    // the image rect.
    s = wuffs_gif__decoder__decode_frame(&dec, dst_writer, src_reader);
    if (s) {
      if (s == WUFFS_GIF__SUSPENSION_END_OF_DATA) {
        break;
      }
      return wuffs_gif__status__string(s);
    }

    // TODO: don't hard code the 100ms sleep time. Wuffs needs an API to
    // provide the frame delay, and this program should also track that across
    // the last frame of one play through and the first frame of the next. The
    // usleep arg should also take into account the decoding time.
    usleep(100000);

    uint8_t* d = dst_buf;
    uint8_t* p = print_buf;
    *p++ = '\n';
    uint32_t y;
    for (y = 0; y < height; y++) {
      uint32_t x;
      for (x = 0; x < width; x++) {
        uint8_t palette_index = *d++;
        // TODO: translate the palette_index into an (R, G, B) triple, and then
        // to a uint8_t grayscale value.
        *p++ = "-+X@"[palette_index >> 6];
      }
      *p++ = '\n';
    }
    write(1, print_buf, print_len);
  }
  return NULL;
}

int fail(const char* msg) {
  const int stderr_fd = 2;
  write(stderr_fd, msg, strnlen(msg, 4095));
  write(stderr_fd, "\n", 1);
  return 1;
}

int main(int argc, char** argv) {
  const char* msg = read_stdin();
  if (msg) {
    return fail(msg);
  }
  while (true) {
    msg = play();
    if (msg) {
      return fail(msg);
    }
    if (num_loops_remaining == 0) {
      continue;
    }
    num_loops_remaining--;
    if (num_loops_remaining == 0) {
      break;
    }
  }
  return 0;
}
