// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

/*
To manually run this test:

for cc in clang gcc; do
  $cc -std=c99 -Wall -Werror lzw.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).
*/

#include "../../../gen/c/gif.c"
#include "../testlib/testlib.c"

const char* test_filename = "gif/lzw.c";

#define BUFFER_SIZE (1024 * 1024)

uint8_t global_dst_buffer[BUFFER_SIZE];
uint8_t global_src_buffer[BUFFER_SIZE];

void test_lzw_decode() {
  puffs_base_buf1 dst = {.ptr = global_dst_buffer, .cap = BUFFER_SIZE};
  puffs_base_buf1 src = {.ptr = global_src_buffer, .cap = BUFFER_SIZE};
  if (!read_file(&src, "../../testdata/bricks-nodither.giflzw")) {
    goto cleanup0;
  }
  // That .giflzw file should be 13382 bytes long.
  if (src.wi != 13382) {
    FAIL("test_lzw_decode: file size: got %d, want %d", (int)(src.wi), 13382);
    goto cleanup0;
  }
  // The first byte in that file, the LZW literal width, should be 0x08.
  if (src.ptr[0] != 0x08) {
    FAIL("test_lzw_decode: LZW literal width: got %d, want %d",
         (int)(src.ptr[0]), 0x08);
    goto cleanup0;
  }

  puffs_gif_lzw_decoder dec;
  puffs_gif_lzw_decoder_constructor(&dec, PUFFS_VERSION, 0);
  puffs_gif_status status =
      puffs_gif_lzw_decoder_decode(&dec, &dst, &src, false);
  if (status != puffs_gif_status_ok) {
    FAIL("test_lzw_decode: status: got %d, want %d", status,
         puffs_gif_status_ok);
    goto cleanup1;
  }
cleanup1:
  puffs_gif_lzw_decoder_destructor(&dec);
cleanup0:;
}

// The empty comments forces clang-format to place one element per line.
test tests[] = {
    test_lzw_decode,  //
    NULL,             //
};
