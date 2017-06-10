// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

/*
To manually run this test:

for cc in clang gcc; do
  $cc -std=c99 -Wall -Werror gif.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).
*/

#include "../../../gen/c/gif.c"
#include "../testlib/testlib.c"

const char* test_filename = "gif/gif.c";

#define BUFFER_SIZE (1024 * 1024)

uint8_t global_got_buffer[BUFFER_SIZE];
uint8_t global_want_buffer[BUFFER_SIZE];
uint8_t global_src_buffer[BUFFER_SIZE];

void test_lzw_decode() {
  test_funcname = __func__;
  puffs_base_buf1 got = {.ptr = global_got_buffer, .len = BUFFER_SIZE};
  puffs_base_buf1 want = {.ptr = global_want_buffer, .len = BUFFER_SIZE};
  puffs_base_buf1 src = {.ptr = global_src_buffer, .len = BUFFER_SIZE};

  // The want .indexes file should be 19200 bytes long, as the image size is
  // 160 * 120 pixels and there is 1 palette index byte per pixel.
  if (!read_file(&want, "../../testdata/bricks-nodither.indexes")) {
    goto cleanup0;
  }
  if (want.wi != 19200) {
    FAIL("want size: got %d, want %d", (int)(want.wi), 19200);
    goto cleanup0;
  }

  // The src .giflzw file should be 13382 bytes long.
  if (!read_file(&src, "../../testdata/bricks-nodither.giflzw")) {
    goto cleanup0;
  }
  if (src.wi != 13382) {
    FAIL("src size: got %d, want %d", (int)(src.wi), 13382);
    goto cleanup0;
  }
  // The first byte in that file, the LZW literal width, should be 0x08.
  if (src.ptr[0] != 0x08) {
    FAIL("LZW literal width: got %d, want %d", (int)(src.ptr[0]), 0x08);
    goto cleanup0;
  }
  src.ri++;

  puffs_gif_lzw_decoder dec;
  puffs_gif_lzw_decoder_constructor(&dec, PUFFS_VERSION, 0);
  // TODO: call puffs_gif_lzw_decoder_set_literal_width.
  puffs_gif_status status =
      puffs_gif_lzw_decoder_decode(&dec, &got, &src, false);
  if (status != puffs_gif_status_ok) {
    FAIL("status: got %d, want %d", status, puffs_gif_status_ok);
    goto cleanup1;
  }

  if (!buf1s_equal(&got, &want)) {
    goto cleanup1;
  }
  // As a sanity check, the first decoded byte should be 0xDC.
  if (got.ptr[0] != 0xDC) {
    FAIL("first decoded byte: got 0x%02x, want 0x%02x", (int)(got.ptr[0]),
         0xDC);
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
