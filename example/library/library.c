// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

/*
library exercises the software libraries built by `puffs genlib`.

To exercise the static library:

$cc -static -I../../.. library.c ../../gen/lib/c/$cc-static/libpuffs.a
./a.out
rm -f a.out

To exercise the dynamic library:

$cc -I../../.. library.c -L../../gen/lib/c/$cc-dynamic -lpuffs
LD_LIBRARY_PATH=../../gen/lib/c/$cc-dynamic ./a.out
rm -f a.out

for a C compiler $cc, such as clang or gcc.
*/

#include <unistd.h>

#include "puffs/gen/h/std/flate.h"

#define DST_BUFFER_SIZE (1024 * 1024)

size_t lgtm_len = 20;

uint8_t lgtm_ptr[] = {
    0xf3, 0xc9, 0xcf, 0xcf, 0x2e, 0x56, 0x48, 0xcf, 0xcf, 0x4f,
    0x51, 0x28, 0xc9, 0x57, 0xc8, 0x4d, 0xd5, 0xe3, 0x02, 0x00,
};

// ignore_return_value suppresses errors from -Wall -Werror.
static void ignore_return_value(int ignored) {}

static const char* decode(puffs_flate_decoder* dec) {
  uint8_t dst_buffer[DST_BUFFER_SIZE];
  puffs_base_buf1 dst = {.ptr = dst_buffer, .len = DST_BUFFER_SIZE};
  puffs_base_buf1 src = {
      .ptr = lgtm_ptr, .len = lgtm_len, .wi = lgtm_len, .closed = true};
  puffs_base_writer1 dst_writer = {.buf = &dst};
  puffs_base_reader1 src_reader = {.buf = &src};

  puffs_flate_status s =
      puffs_flate_decoder_decode(dec, dst_writer, src_reader);
  if (s) {
    return puffs_flate_status_string(s);
  }
  ignore_return_value(write(1, dst.ptr, dst.wi));
  return NULL;
}

int main(int argc, char** argv) {
  puffs_flate_decoder dec;
  puffs_flate_decoder_initialize(&dec, PUFFS_VERSION, 0);
  const char* status_msg = decode(&dec);

  int status = 0;
  if (status_msg) {
    status = 1;
    ignore_return_value(write(2, status_msg, strnlen(status_msg, 4095)));
    ignore_return_value(write(2, "\n", 1));
  }
  return status;
}
