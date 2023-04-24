// Copyright 2023 The Wuffs Authors.
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
This test program is typically run indirectly, by the "wuffs test" or "wuffs
bench" commands. These commands take an optional "-mimic" flag to check that
Wuffs' output mimics (i.e. exactly matches) other libraries' output, such as
giflib for GIF, libpng for PNG, etc.

To manually run this test:

for CC in clang gcc; do
  $CC -std=c99 -Wall -Werror jpeg.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).

Add the "wuffs mimic cflags" (everything after the colon below) to the C
compiler flags (after the .c file) to run the mimic tests.

To manually run the benchmarks, replace "-Wall -Werror" with "-O3" and replace
the first "./a.out" with "./a.out -bench". Combine these changes with the
"wuffs mimic cflags" to run the mimic benchmarks.
*/

// ¿ wuffs mimic cflags: -DWUFFS_MIMIC

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c choose which parts of Wuffs to build. That file contains the
// entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__JPEG

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
// No mimic library.
#endif

// ---------------- JPEG Tests

const char*  //
wuffs_jpeg_decode(uint64_t* n_bytes_out,
                  wuffs_base__io_buffer* dst,
                  uint32_t wuffs_initialize_flags,
                  wuffs_base__pixel_format pixfmt,
                  uint32_t* quirks_ptr,
                  size_t quirks_len,
                  wuffs_base__io_buffer* src) {
  wuffs_jpeg__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_jpeg__decoder__initialize(&dec, sizeof dec, WUFFS_VERSION,
                                               wuffs_initialize_flags));
  return do_run__wuffs_base__image_decoder(
      wuffs_jpeg__decoder__upcast_as__wuffs_base__image_decoder(&dec),
      n_bytes_out, dst, pixfmt, quirks_ptr, quirks_len, src);
}

const char*  //
test_wuffs_jpeg_decode_interface() {
  CHECK_FOCUS(__func__);
  wuffs_jpeg__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_jpeg__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));
  return do_test__wuffs_base__image_decoder(
      wuffs_jpeg__decoder__upcast_as__wuffs_base__image_decoder(&dec),
      "test/data/bricks-color.jpeg", 0, SIZE_MAX, 160, 120, 0xFF777F9F);
}

const char*  //
test_wuffs_jpeg_decode_truncated_input() {
  CHECK_FOCUS(__func__);

  wuffs_base__io_buffer src =
      wuffs_base__ptr_u8__reader(g_src_array_u8, 0, false);
  wuffs_jpeg__decoder dec;
  CHECK_STATUS("initialize",
               wuffs_jpeg__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  wuffs_base__status status =
      wuffs_jpeg__decoder__decode_image_config(&dec, NULL, &src);
  if (status.repr != wuffs_base__suspension__short_read) {
    RETURN_FAIL("closed=false: have \"%s\", want \"%s\"", status.repr,
                wuffs_base__suspension__short_read);
  }

  src.meta.closed = true;
  status = wuffs_jpeg__decoder__decode_image_config(&dec, NULL, &src);
  if (status.repr != wuffs_jpeg__error__truncated_input) {
    RETURN_FAIL("closed=true: have \"%s\", want \"%s\"", status.repr,
                wuffs_jpeg__error__truncated_input);
  }
  return NULL;
}

const char*  //
do_test_wuffs_jpeg_decode_dht(wuffs_base__io_buffer* src,
                              const uint32_t arg_bits,
                              const uint32_t arg_n_bits,
                              const int32_t* want_dst_ptr,
                              const size_t want_dst_len,
                              const uint8_t (*want_symbols)[256],
                              const uint32_t (*want_slow)[16],
                              const uint16_t (*want_fast)[256]) {
  wuffs_jpeg__decoder dec;
  memset(&dec, 0xAB, sizeof dec);  // 0xAB is arbitrary, non-zero junk.
  CHECK_STATUS("initialize",
               wuffs_jpeg__decoder__initialize(
                   &dec, sizeof dec, WUFFS_VERSION,
                   WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED));

  // Decode that DHT payload.
  if (wuffs_base__io_buffer__reader_length(src) <= 0) {
    return "empty src";
  }
  uint32_t tc4_th = ((src->data.ptr[src->meta.ri] >> 2) & 0x04) |
                    ((src->data.ptr[src->meta.ri] >> 0) & 0x03);
  dec.private_impl.f_sof_marker = 0xC0;
  dec.private_impl.f_payload_length =
      ((uint32_t)(wuffs_base__io_buffer__reader_length(src)));
  CHECK_STATUS("decode_dht", wuffs_jpeg__decoder__decode_dht(&dec, src));

  if (memcmp(dec.private_impl.f_huff_tables_symbols[tc4_th], want_symbols,
             sizeof(*want_symbols))) {
    RETURN_FAIL("unexpected huff_tables_symbols");
  } else if (memcmp(dec.private_impl.f_huff_tables_slow[tc4_th], want_slow,
                    sizeof(*want_slow))) {
    RETURN_FAIL("unexpected huff_tables_slow");
  } else if (memcmp(dec.private_impl.f_huff_tables_fast[tc4_th], want_fast,
                    sizeof(*want_fast))) {
    RETURN_FAIL("unexpected huff_tables_fast");
  }

  const int32_t err_not_enough_bits = -1;
  const int32_t err_fast_not_applicable = -2;
  const int32_t err_invalid_code = -3;

  // Check decoding of (arg_bits, arg_n_bits).
  for (int use_fast = 0; use_fast < 2; use_fast++) {
    uint32_t bits = arg_bits;
    uint32_t n_bits = arg_n_bits;
    for (size_t i = 0; i < want_dst_len; i++) {
      int have = err_fast_not_applicable;

      if (use_fast) {
        uint16_t x = (*want_fast)[bits >> 24];
        uint32_t n = x >> 8;
        if (x == 0) {
          have = err_fast_not_applicable;
        } else if (n > n_bits) {
          have = err_not_enough_bits;
        } else {
          bits <<= n;
          n_bits -= n;
          have = x & 0xFF;
        }
      }

      if (have == err_fast_not_applicable) {
        uint32_t code = 0;
        for (uint32_t j = 0; true; j++) {
          if (n_bits <= 0) {
            have = err_not_enough_bits;
            break;
          } else if (j >= 16) {
            have = err_invalid_code;
            break;
          }
          code = (code << 1) | (bits >> 31);
          bits <<= 1;
          n_bits -= 1;
          uint32_t x = (*want_slow)[j];
          if (code < (x >> 8)) {
            have = (*want_symbols)[(code + x) & 0xFF];
            break;
          }
        }
      }

      if (have != want_dst_ptr[i]) {
        RETURN_FAIL("output symbols: use_fast=%d, i=%zu: have %d, want %d",
                    use_fast, i, have, want_dst_ptr[i]);
      }
    }
  }

  return NULL;
}

const char*  //
test_wuffs_jpeg_decode_dht_easy() {
  CHECK_FOCUS(__func__);

  const uint8_t want_symbols[256] = {
      6, 7, 4, 5, 0, 3, 8, 9, 2, 1, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //
  };

  // Set src to this fragment of "hd test/data/bricks-color.jpeg".
  //   000000b0  .. ff c4 00 1d 00 00 02  02 03 01 01 01 00 00 00
  //   000000c0  00 00 00 00 00 00 06 07  04 05 00 03 08 09 02 01
  // The "ff c4" is the DHT marker. The "00 1d" is the payload length. The
  // remaining payload has 0x001D - 2 = 0x1B = 17 + 10 bytes.
  g_src_array_u8[0x00] = 0x00;  // (tc, th) selectors are (0, 0).
  g_src_array_u8[0x01] = 0x00;  // 0 codes of bit_length 1.
  g_src_array_u8[0x02] = 0x02;  // 2 codes of bit_length 2.
  g_src_array_u8[0x03] = 0x02;  // 2 codes of bit_length 3.
  g_src_array_u8[0x04] = 0x03;  // 3 codes of bit_length 4.
  g_src_array_u8[0x05] = 0x01;  // 1 codes of bit_length 5.
  g_src_array_u8[0x06] = 0x01;  // 1 codes of bit_length 6.
  g_src_array_u8[0x07] = 0x01;  // 1 codes of bit_length 7.
  g_src_array_u8[0x08] = 0x00;  // 0 codes of bit_length 8.
  g_src_array_u8[0x09] = 0x00;  // 0 codes of bit_length 9.
  g_src_array_u8[0x0A] = 0x00;  // etc.
  g_src_array_u8[0x0B] = 0x00;
  g_src_array_u8[0x0C] = 0x00;
  g_src_array_u8[0x0D] = 0x00;
  g_src_array_u8[0x0E] = 0x00;
  g_src_array_u8[0x0F] = 0x00;
  g_src_array_u8[0x10] = 0x00;  // 0 codes of bit-length 16. 10 codes total.
  g_src_array_u8[0x11] = want_symbols[0];  // The 1st symbol is 0x06.
  g_src_array_u8[0x12] = want_symbols[1];  // The 2nd symbol is 0x07.
  g_src_array_u8[0x13] = want_symbols[2];  // The 3rd symbol is 0x04.
  g_src_array_u8[0x14] = want_symbols[3];  // etc.
  g_src_array_u8[0x15] = want_symbols[4];
  g_src_array_u8[0x16] = want_symbols[5];
  g_src_array_u8[0x17] = want_symbols[6];
  g_src_array_u8[0x18] = want_symbols[7];
  g_src_array_u8[0x19] = want_symbols[8];
  g_src_array_u8[0x1A] = want_symbols[9];
  wuffs_base__io_buffer src =
      wuffs_base__ptr_u8__reader(g_src_array_u8, 0x1B, false);

  // The Huffman codes are:
  //   0b00......   bit_length=2   symbol=6
  //   0b01......   bit_length=2   symbol=7
  //   0b100.....   bit_length=3   symbol=4
  //   0b101.....   bit_length=3   symbol=5
  //   0b1100....   bit_length=4   symbol=0
  //   0b1101....   bit_length=4   symbol=3
  //   0b1110....   bit_length=4   symbol=8
  //   0b11110...   bit_length=5   symbol=9
  //   0b111110..   bit_length=6   symbol=2
  //   0b1111110.   bit_length=7   symbol=1
  //   0b1111111.   invalid

  // Running this on "wxyz" input should give these symbols:
  //   0x77 'w'      0x78 'x'      0x79 'y'      0x7A 'z'
  //   0b0111_0111   0b0111_1000   0b0111_1001   0b0111_1010
  //     01 1101 1101    1110 00     01 1110 01    01 1110 10
  //     s7 s3   s3      s8   s6     s7 s8   s7    s7 s8   err_not_enough_bits
  const uint32_t bits = 0x7778797A;
  const uint32_t n_bits = 32;
  const int32_t want_dst_ptr[11] = {7, 3, 3, 8, 6, 7, 8, 7, 7, 8, -1};
  const size_t want_dst_len = 11;

  const uint32_t want_slow[16] = {
      0x00000000, 0x00000200, 0x000006FE, 0x00000FF8,  //
      0x00001FE9, 0x00003FCA, 0x00007F8B, 0x00000000,  //
      0x00000000, 0x00000000, 0x00000000, 0x00000000,  //
      0x00000000, 0x00000000, 0x00000000, 0x00000000,  //
  };

  const uint16_t want_fast[256] = {
      0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206,  //
      0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206,  //
      0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206,  //
      0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206,  //
      0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206,  //
      0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206,  //
      0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206,  //
      0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206, 0x0206,  //

      0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207,  //
      0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207,  //
      0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207,  //
      0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207,  //
      0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207,  //
      0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207,  //
      0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207,  //
      0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207, 0x0207,  //

      0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304,  //
      0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304,  //
      0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304,  //
      0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304, 0x0304,  //
      0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305,  //
      0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305,  //
      0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305,  //
      0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305, 0x0305,  //

      0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400,  //
      0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400,  //
      0x0403, 0x0403, 0x0403, 0x0403, 0x0403, 0x0403, 0x0403, 0x0403,  //
      0x0403, 0x0403, 0x0403, 0x0403, 0x0403, 0x0403, 0x0403, 0x0403,  //
      0x0408, 0x0408, 0x0408, 0x0408, 0x0408, 0x0408, 0x0408, 0x0408,  //
      0x0408, 0x0408, 0x0408, 0x0408, 0x0408, 0x0408, 0x0408, 0x0408,  //
      0x0509, 0x0509, 0x0509, 0x0509, 0x0509, 0x0509, 0x0509, 0x0509,  //
      0x0602, 0x0602, 0x0602, 0x0602, 0x0701, 0x0701, 0x0000, 0x0000,  //
  };

  return do_test_wuffs_jpeg_decode_dht(&src, bits, n_bits, want_dst_ptr,
                                       want_dst_len, &want_symbols, &want_slow,
                                       &want_fast);
}

const char*  //
test_wuffs_jpeg_decode_dht_hard() {
  CHECK_FOCUS(__func__);

  const uint8_t want_symbols[256] = {
      0x01, 0x02, 0x03, 0x04, 0x05, 0x11, 0x00, 0x06,  //
      0x07, 0x12, 0x21, 0x31, 0x13, 0x41, 0x51, 0x08,  //
      0x14, 0x22, 0x61, 0x71, 0x32, 0x42, 0x81, 0x91,  //
      0xA1, 0xB1, 0x15, 0x23, 0x52, 0x33, 0x82, 0xC1,  //
      0xD1, 0xF0, 0x16, 0x62, 0x72, 0x92, 0xC2, 0xE1,  //
      0x17, 0x53, 0x63, 0xA2, 0xA4, 0xB2, 0xF1, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
  };

  // Set src to this fragment of "hd test/data/bricks-color.jpeg".
  //   000000d0  ff c4 00 42 10 00 02 01  03 02 04 03 05 06 03 05
  //   000000e0  06 07 00 00 00 01 02 03  04 05 11 00 06 07 12 21
  //   000000f0  31 13 41 51 08 14 22 61  71 32 42 81 91 a1 b1 15
  //   00000100  23 52 33 82 c1 d1 f0 16  62 72 92 c2 e1 17 53 63
  //   00000110  a2 a4 b2 f1 .. .. .. ..  .. .. .. .. .. .. .. ..
  // The "ff c4" is the DHT marker. The "00 42" is the payload length. The
  // remaining payload has 0x0042 - 2 = 0x40 = 17 + 47 bytes.
  g_src_array_u8[0x00] = 0x10;  // (tc, th) selectors are (1, 0).
  g_src_array_u8[0x01] = 0x00;  // 0 codes of bit_length 1.
  g_src_array_u8[0x02] = 0x02;  // 2 codes of bit_length 2.
  g_src_array_u8[0x03] = 0x01;  // 1 codes of bit_length 3.
  g_src_array_u8[0x04] = 0x03;  // 3 codes of bit_length 4.
  g_src_array_u8[0x05] = 0x02;  // 2 codes of bit_length 5.
  g_src_array_u8[0x06] = 0x04;  // 4 codes of bit_length 6.
  g_src_array_u8[0x07] = 0x03;  // 3 codes of bit_length 7.
  g_src_array_u8[0x08] = 0x05;  // 5 codes of bit_length 8.
  g_src_array_u8[0x09] = 0x06;  // 6 codes of bit_length 9.
  g_src_array_u8[0x0A] = 0x03;  // etc.
  g_src_array_u8[0x0B] = 0x05;
  g_src_array_u8[0x0C] = 0x06;
  g_src_array_u8[0x0D] = 0x07;
  g_src_array_u8[0x0E] = 0x00;
  g_src_array_u8[0x0F] = 0x00;
  g_src_array_u8[0x10] = 0x00;  // 0 codes of bit-length 16. 47 codes total.
  memcpy(&g_src_array_u8[0x11], want_symbols, 47);
  wuffs_base__io_buffer src =
      wuffs_base__ptr_u8__reader(g_src_array_u8, 0x40, false);

  // The Huffman codes are:
  //   0b00......_........   bit_length=0x02   symbol=0x01
  //   0b01......_........   bit_length=0x02   symbol=0x02
  //   0b100....._........   bit_length=0x03   symbol=0x03
  //   0b1010...._........   bit_length=0x04   symbol=0x04
  //   0b1011...._........   bit_length=0x04   symbol=0x05
  //   0b1100...._........   bit_length=0x04   symbol=0x11
  //   0b11010..._........   bit_length=0x05   symbol=0x00
  //   0b11011..._........   bit_length=0x05   symbol=0x06
  //   0b111000.._........   bit_length=0x06   symbol=0x07
  //   0b111001.._........   bit_length=0x06   symbol=0x12
  //   0b111010.._........   bit_length=0x06   symbol=0x21
  //   0b111011.._........   bit_length=0x06   symbol=0x31
  //   0b1111000._........   bit_length=0x07   symbol=0x13
  //   0b1111001._........   bit_length=0x07   symbol=0x41
  //   0b1111010._........   bit_length=0x07   symbol=0x51
  //   0b11110110_........   bit_length=0x08   symbol=0x08
  //   0b11110111_........   bit_length=0x08   symbol=0x14
  //   0b11111000_........   bit_length=0x08   symbol=0x22
  //   0b11111001_........   bit_length=0x08   symbol=0x61
  //   0b11111010_........   bit_length=0x08   symbol=0x71
  //   0b11111011_0.......   bit_length=0x09   symbol=0x32
  //   0b11111011_1.......   bit_length=0x09   symbol=0x42
  //   0b11111100_0.......   bit_length=0x09   symbol=0x81
  //   0b11111100_1.......   bit_length=0x09   symbol=0x91
  //   0b11111101_0.......   bit_length=0x09   symbol=0xA1
  //   0b11111101_1.......   bit_length=0x09   symbol=0xB1
  //   0b11111110_00......   bit_length=0x0A   symbol=0x15
  //   0b11111110_01......   bit_length=0x0A   symbol=0x23
  //   0b11111110_10......   bit_length=0x0A   symbol=0x52
  //   0b11111110_110.....   bit_length=0x0B   symbol=0x33
  //   0b11111110_111.....   bit_length=0x0B   symbol=0x82
  //   0b11111111_000.....   bit_length=0x0B   symbol=0xC1
  //   0b11111111_001.....   bit_length=0x0B   symbol=0xD1
  //   0b11111111_010.....   bit_length=0x0B   symbol=0xF0
  //   0b11111111_0110....   bit_length=0x0C   symbol=0x16
  //   0b11111111_0111....   bit_length=0x0C   symbol=0x62
  //   0b11111111_1000....   bit_length=0x0C   symbol=0x72
  //   0b11111111_1001....   bit_length=0x0C   symbol=0x92
  //   0b11111111_1010....   bit_length=0x0C   symbol=0xC2
  //   0b11111111_1011....   bit_length=0x0C   symbol=0xE1
  //   0b11111111_11000...   bit_length=0x0D   symbol=0x17
  //   0b11111111_11001...   bit_length=0x0D   symbol=0x53
  //   0b11111111_11010...   bit_length=0x0D   symbol=0x63
  //   0b11111111_11011...   bit_length=0x0D   symbol=0xA2
  //   0b11111111_11100...   bit_length=0x0D   symbol=0xA4
  //   0b11111111_11101...   bit_length=0x0D   symbol=0xB2
  //   0b11111111_11110...   bit_length=0x0D   symbol=0xF1
  //   0b11111111_11111...   invalid

  // Running this on "wx\x7Fh" input should give these symbols:
  //   0x77 'w'      0x78 'x'      0x7F '\x7F'   0x68 'h'
  //   0b0111_0111   0b0111_1000   0b0111_1111   0b0110_1000
  //     01 11011 1011    1100 00     11111110110       100 0
  //     s2 s6    s5      s11  s1     s33               s3  err_not_enough_bits
  const uint32_t bits = 0x77787F68;
  const uint32_t n_bits = 32;
  const int32_t want_dst_ptr[8] = {0x02, 0x06, 0x05, 0x11,
                                   0x01, 0x33, 0x03, -1};
  const size_t want_dst_len = 8;

  const uint32_t want_slow[16] = {
      0x00000000, 0x00000200, 0x000005FE, 0x00000DF9,  //
      0x00001CEC, 0x00003CD0, 0x00007B94, 0x0000FB19,  //
      0x0001FC1E, 0x0003FB22, 0x0007FB27, 0x000FFC2C,  //
      0x001FFF30, 0x00000000, 0x00000000, 0x00000000,  //
  };

  const uint16_t want_fast[256] = {
      0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201,  //
      0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201,  //
      0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201,  //
      0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201,  //
      0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201,  //
      0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201,  //
      0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201,  //
      0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201, 0x0201,  //

      0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202,  //
      0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202,  //
      0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202,  //
      0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202,  //
      0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202,  //
      0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202,  //
      0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202,  //
      0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202, 0x0202,  //

      0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303,  //
      0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303,  //
      0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303,  //
      0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303, 0x0303,  //
      0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404,  //
      0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404, 0x0404,  //
      0x0405, 0x0405, 0x0405, 0x0405, 0x0405, 0x0405, 0x0405, 0x0405,  //
      0x0405, 0x0405, 0x0405, 0x0405, 0x0405, 0x0405, 0x0405, 0x0405,  //

      0x0411, 0x0411, 0x0411, 0x0411, 0x0411, 0x0411, 0x0411, 0x0411,  //
      0x0411, 0x0411, 0x0411, 0x0411, 0x0411, 0x0411, 0x0411, 0x0411,  //
      0x0500, 0x0500, 0x0500, 0x0500, 0x0500, 0x0500, 0x0500, 0x0500,  //
      0x0506, 0x0506, 0x0506, 0x0506, 0x0506, 0x0506, 0x0506, 0x0506,  //
      0x0607, 0x0607, 0x0607, 0x0607, 0x0612, 0x0612, 0x0612, 0x0612,  //
      0x0621, 0x0621, 0x0621, 0x0621, 0x0631, 0x0631, 0x0631, 0x0631,  //
      0x0713, 0x0713, 0x0741, 0x0741, 0x0751, 0x0751, 0x0808, 0x0814,  //
      0x0822, 0x0861, 0x0871, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,  //
  };

  return do_test_wuffs_jpeg_decode_dht(&src, bits, n_bits, want_dst_ptr,
                                       want_dst_len, &want_symbols, &want_slow,
                                       &want_fast);
}

// ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

// No mimic tests.

#endif  // WUFFS_MIMIC

// ---------------- JPEG Benches

const char*  //
bench_wuffs_jpeg_decode_77k_24bpp() {
  CHECK_FOCUS(__func__);
  return do_bench_image_decode(
      &wuffs_jpeg_decode,
      WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED,
      wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL),
      NULL, 0, "test/data/bricks-color.jpeg", 0, SIZE_MAX, 1000);
}

// ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

// No mimic benches.

#endif  // WUFFS_MIMIC

// ---------------- Manifest

proc g_tests[] = {

    test_wuffs_jpeg_decode_dht_easy,
    test_wuffs_jpeg_decode_dht_hard,
    test_wuffs_jpeg_decode_interface,
    test_wuffs_jpeg_decode_truncated_input,

#ifdef WUFFS_MIMIC

// No mimic tests.

#endif  // WUFFS_MIMIC

    NULL,
};

proc g_benches[] = {

    bench_wuffs_jpeg_decode_77k_24bpp,

#ifdef WUFFS_MIMIC

// No mimic benches.

#endif  // WUFFS_MIMIC

    NULL,
};

int  //
main(int argc, char** argv) {
  g_proc_package_name = "std/jpeg";
  return test_main(argc, argv, g_tests, g_benches);
}
