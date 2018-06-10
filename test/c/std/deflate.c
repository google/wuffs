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
This test program is typically run indirectly, by the "wuffs test" or "wuffs
bench" commands. These commands take an optional "-mimic" flag to check that
Wuffs' output mimics (i.e. exactly matches) other libraries' output, such as
giflib for GIF, libpng for PNG, etc.

To manually run this test:

for cc in clang gcc; do
  $cc -std=c99 -Wall -Werror deflate.c && ./a.out
  rm -f a.out
done

Each edition should print "PASS", amongst other information, and exit(0).

Add the "wuffs mimic cflags" (everything after the colon below) to the C
compiler flags (after the .c file) to run the mimic tests.

To manually run the benchmarks, replace "-Wall -Werror" with "-O3" and replace
the first "./a.out" with "./a.out -bench". Combine these changes with the
"wuffs mimic cflags" to run the mimic benchmarks.
*/

// !! wuffs mimic cflags: -DWUFFS_MIMIC -lz

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../gen/c/base.c"
#include "../../../gen/c/std/deflate.c"
#include "../testlib/testlib.c"
#ifdef WUFFS_MIMIC
#include "../mimiclib/deflate-gzip-zlib.c"
#endif

// ---------------- Golden Tests

// The src_offset0 and src_offset1 magic numbers come from:
//
// go run script/extract-flate-offsets.go test/data/*.gz
//
// The empty comments forces clang-format to place one element per line.

golden_test deflate_256_bytes_gt = {
    .want_filename = "../../data/artificial/256.bytes",    //
    .src_filename = "../../data/artificial/256.bytes.gz",  //
    .src_offset0 = 20,                                     //
    .src_offset1 = 281,                                    //
};

golden_test deflate_deflate_backref_crosses_blocks_gt = {
    .want_filename =
        "../../data/artificial/"
        "deflate-backref-crosses-blocks.deflate.decompressed",
    .src_filename =
        "../../data/artificial/"
        "deflate-backref-crosses-blocks.deflate",
};

golden_test deflate_deflate_distance_32768_gt = {
    .want_filename =
        "../../data/artificial/"
        "deflate-distance-32768.deflate.decompressed",
    .src_filename =
        "../../data/artificial/"
        "deflate-distance-32768.deflate",
};

golden_test deflate_midsummer_gt = {
    .want_filename = "../../data/midsummer.txt",    //
    .src_filename = "../../data/midsummer.txt.gz",  //
    .src_offset0 = 24,                              //
    .src_offset1 = 5166,                            //
};

golden_test deflate_pi_gt = {
    .want_filename = "../../data/pi.txt",    //
    .src_filename = "../../data/pi.txt.gz",  //
    .src_offset0 = 17,                       //
    .src_offset1 = 48335,                    //
};

golden_test deflate_romeo_gt = {
    .want_filename = "../../data/romeo.txt",    //
    .src_filename = "../../data/romeo.txt.gz",  //
    .src_offset0 = 20,                          //
    .src_offset1 = 550,                         //
};

golden_test deflate_romeo_fixed_gt = {
    .want_filename = "../../data/romeo.txt",                    //
    .src_filename = "../../data/romeo.txt.fixed-huff.deflate",  //
};

// ---------------- Deflate Tests

const char* wuffs_deflate_decode(wuffs_base__io_buffer* dst,
                                 wuffs_base__io_buffer* src,
                                 uint64_t wlimit,
                                 uint64_t rlimit) {
  wuffs_deflate__decoder dec = ((wuffs_deflate__decoder){});
  wuffs_deflate__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);
  while (true) {
    wuffs_base__io_writer dst_writer = wuffs_base__io_buffer__writer(dst);
    if (wlimit) {
      set_writer_limit(&dst_writer, wlimit);
    }
    wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(src);
    if (rlimit) {
      set_reader_limit(&src_reader, rlimit);
    }

    wuffs_base__status s =
        wuffs_deflate__decoder__decode(&dec, dst_writer, src_reader);

    if (s == WUFFS_BASE__STATUS_OK) {
      return NULL;
    }
    if ((wlimit && (s == WUFFS_BASE__SUSPENSION_SHORT_WRITE)) ||
        (rlimit && (s == WUFFS_BASE__SUSPENSION_SHORT_READ))) {
      continue;
    }
    return wuffs_deflate__status__string(s);
  }
}

void test_wuffs_deflate_decode_256_bytes() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_deflate_decode, &deflate_256_bytes_gt, 0, 0);
}

void test_wuffs_deflate_decode_deflate_backref_crosses_blocks() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_deflate_decode,
                     &deflate_deflate_backref_crosses_blocks_gt, 0, 0);
}

void test_wuffs_deflate_decode_deflate_distance_32768() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_deflate_decode, &deflate_deflate_distance_32768_gt,
                     0, 0);
}

void test_wuffs_deflate_decode_midsummer() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_deflate_decode, &deflate_midsummer_gt, 0, 0);
}

void test_wuffs_deflate_decode_pi() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_deflate_decode, &deflate_pi_gt, 0, 0);
}

void test_wuffs_deflate_decode_pi_many_big_reads() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_deflate_decode, &deflate_pi_gt, 0, 4096);
}

void test_wuffs_deflate_decode_pi_many_medium_reads() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_deflate_decode, &deflate_pi_gt, 0, 599);
}

void test_wuffs_deflate_decode_pi_many_small_writes_reads() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_deflate_decode, &deflate_pi_gt, 59, 61);
}

void test_wuffs_deflate_decode_romeo() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_deflate_decode, &deflate_romeo_gt, 0, 0);
}

void test_wuffs_deflate_decode_romeo_fixed() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(wuffs_deflate_decode, &deflate_romeo_fixed_gt, 0, 0);
}

void test_wuffs_deflate_decode_split_src() {
  CHECK_FOCUS(__func__);

  wuffs_base__io_buffer src =
      ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});
  wuffs_base__io_buffer got =
      ((wuffs_base__io_buffer){.ptr = global_got_buffer, .len = BUFFER_SIZE});
  wuffs_base__io_buffer want =
      ((wuffs_base__io_buffer){.ptr = global_want_buffer, .len = BUFFER_SIZE});

  golden_test* gt = &deflate_256_bytes_gt;
  if (!read_file(&src, gt->src_filename)) {
    return;
  }
  if (!read_file(&want, gt->want_filename)) {
    return;
  }

  wuffs_base__io_writer dst_writer = wuffs_base__io_buffer__writer(&got);
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(&src);

  int i;
  for (i = 1; i < 32; i++) {
    size_t split = gt->src_offset0 + i;
    if (split >= gt->src_offset1) {
      FAIL("i=%d: split was not an interior split", i);
      return;
    }
    got.wi = 0;

    wuffs_deflate__decoder dec = ((wuffs_deflate__decoder){});
    wuffs_deflate__decoder__check_wuffs_version(&dec, sizeof dec,
                                                WUFFS_VERSION);

    src.closed = false;
    src.ri = gt->src_offset0;
    src.wi = split;
    wuffs_base__status s0 =
        wuffs_deflate__decoder__decode(&dec, dst_writer, src_reader);

    src.closed = true;
    src.ri = split;
    src.wi = gt->src_offset1;
    wuffs_base__status s1 =
        wuffs_deflate__decoder__decode(&dec, dst_writer, src_reader);

    if (s0 != WUFFS_BASE__SUSPENSION_SHORT_READ) {
      FAIL("i=%d: s0: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", i, s0,
           wuffs_deflate__status__string(s0), WUFFS_BASE__SUSPENSION_SHORT_READ,
           wuffs_deflate__status__string(WUFFS_BASE__SUSPENSION_SHORT_READ));
      return;
    }

    if (s1 != WUFFS_BASE__STATUS_OK) {
      FAIL("i=%d: s1: got %" PRIi32 " (%s), want %" PRIi32 " (%s)", i, s1,
           wuffs_deflate__status__string(s1), WUFFS_BASE__STATUS_OK,
           wuffs_deflate__status__string(WUFFS_BASE__STATUS_OK));
      return;
    }

    char prefix[64];
    snprintf(prefix, 64, "i=%d: ", i);
    if (!io_buffers_equal(prefix, &got, &want)) {
      return;
    }
  }
}

bool do_test_wuffs_deflate_history(int i,
                                   golden_test* gt,
                                   wuffs_base__io_buffer* src,
                                   wuffs_base__io_buffer* got,
                                   wuffs_deflate__decoder* dec,
                                   uint32_t starting_history_index,
                                   uint64_t limit,
                                   wuffs_base__status want_s) {
  src->ri = gt->src_offset0;
  src->wi = gt->src_offset1;
  got->ri = 0;
  got->wi = 0;

  wuffs_base__io_writer dst_writer = wuffs_base__io_buffer__writer(got);
  wuffs_base__io_reader src_reader = wuffs_base__io_buffer__reader(src);

  dec->private_impl.f_history_index = starting_history_index;

  set_writer_limit(&dst_writer, limit);

  wuffs_base__status got_s =
      wuffs_deflate__decoder__decode(dec, dst_writer, src_reader);
  if (got_s != want_s) {
    FAIL("i=%d: starting_history_index=0x%04" PRIX32
         ": decode status: got %" PRIi32 " (%s), want %" PRIi32 " (%s)",
         i, starting_history_index, got_s, wuffs_deflate__status__string(got_s),
         want_s, wuffs_deflate__status__string(want_s));
    return false;
  }
  return true;
}

void test_wuffs_deflate_history_full() {
  CHECK_FOCUS(__func__);

  wuffs_base__io_buffer src =
      ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});
  wuffs_base__io_buffer got =
      ((wuffs_base__io_buffer){.ptr = global_got_buffer, .len = BUFFER_SIZE});
  wuffs_base__io_buffer want =
      ((wuffs_base__io_buffer){.ptr = global_want_buffer, .len = BUFFER_SIZE});

  golden_test* gt = &deflate_pi_gt;
  if (!read_file(&src, gt->src_filename)) {
    return;
  }
  if (!read_file(&want, gt->want_filename)) {
    return;
  }

  const int full_history_size = 0x8000;
  int i;
  for (i = -2; i <= +2; i++) {
    wuffs_deflate__decoder dec = ((wuffs_deflate__decoder){});
    wuffs_deflate__decoder__check_wuffs_version(&dec, sizeof dec,
                                                WUFFS_VERSION);

    if (!do_test_wuffs_deflate_history(
            i, gt, &src, &got, &dec, 0, want.wi + i,
            i >= 0 ? WUFFS_BASE__STATUS_OK
                   : WUFFS_BASE__SUSPENSION_SHORT_WRITE)) {
      return;
    }

    uint32_t want_history_index = i >= 0 ? 0 : full_history_size;
    if (dec.private_impl.f_history_index != want_history_index) {
      FAIL("i=%d: history_index: got %" PRIu32 ", want %" PRIu32, i,
           dec.private_impl.f_history_index, want_history_index);
      return;
    }
    if (i >= 0) {
      continue;
    }

    wuffs_base__io_buffer history_got = ((wuffs_base__io_buffer){
        .ptr = dec.private_impl.f_history,
        .len = full_history_size,
        .wi = full_history_size,
    });
    if (want.wi < full_history_size - i) {
      FAIL("i=%d: want file is too short", i);
      return;
    }
    wuffs_base__io_buffer history_want = ((wuffs_base__io_buffer){
        .ptr = global_want_buffer + want.wi - (full_history_size - i),
        .len = full_history_size,
        .wi = full_history_size,
    });
    if (!io_buffers_equal("", &history_got, &history_want)) {
      return;
    }
  }
}

void test_wuffs_deflate_history_partial() {
  CHECK_FOCUS(__func__);

  wuffs_base__io_buffer src =
      ((wuffs_base__io_buffer){.ptr = global_src_buffer, .len = BUFFER_SIZE});
  wuffs_base__io_buffer got =
      ((wuffs_base__io_buffer){.ptr = global_got_buffer, .len = BUFFER_SIZE});

  golden_test* gt = &deflate_pi_gt;
  if (!read_file(&src, gt->src_filename)) {
    return;
  }

  uint32_t starting_history_indexes[] = {
      0x0000, 0x0001, 0x1234, 0x7FFB, 0x7FFC, 0x7FFD, 0x7FFE, 0x7FFF,
      0x8000, 0x8001, 0x9234, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF,
  };

  int i;
  for (i = 0; i < WUFFS_TESTLIB_ARRAY_SIZE(starting_history_indexes); i++) {
    uint32_t starting_history_index = starting_history_indexes[i];

    // The flate_pi_gt golden test file decodes to the digits of pi.
    const char* fragment = "3.14";
    const uint32_t fragment_length = 4;

    wuffs_deflate__decoder dec = ((wuffs_deflate__decoder){});
    wuffs_deflate__decoder__check_wuffs_version(&dec, sizeof dec,
                                                WUFFS_VERSION);

    if (!do_test_wuffs_deflate_history(i, gt, &src, &got, &dec,
                                       starting_history_index, fragment_length,
                                       WUFFS_BASE__SUSPENSION_SHORT_WRITE)) {
      return;
    }

    bool got_full = dec.private_impl.f_history_index >= 0x8000;
    uint32_t got_history_index = dec.private_impl.f_history_index & 0x7FFF;
    bool want_full = (starting_history_index + fragment_length) >= 0x8000;
    uint32_t want_history_index =
        (starting_history_index + fragment_length) & 0x7FFF;
    if ((got_full != want_full) || (got_history_index != want_history_index)) {
      FAIL("i=%d: starting_history_index=0x%04" PRIX32
           ": history_index: got %d;%04" PRIX32 ", want %d;%04" PRIX32,
           i, starting_history_index, (int)(got_full), got_history_index,
           (int)(want_full), want_history_index);
      return;
    }

    int j;
    for (j = -2; j < (int)(fragment_length) + 2; j++) {
      uint32_t index = (starting_history_index + j) & 0x7FFF;
      uint8_t got = dec.private_impl.f_history[index];
      uint8_t want = (0 <= j && j < fragment_length) ? fragment[j] : 0;
      if (got != want) {
        FAIL("i=%d: starting_history_index=0x%04" PRIX32
             ": j=%d: got 0x%02" PRIX8 ", want 0x%02" PRIX8,
             i, starting_history_index, j, got, want);
        return;
      }
    }
  }
}

void test_wuffs_deflate_table_redirect() {
  CHECK_FOCUS(__func__);

  // Call init_huff with a Huffman code that looks like:
  //
  //           code_bits  cl   c   r   s          1st  2nd
  //  0b_______________0   1   1   1   0  0b........0
  //  0b______________10   2   1   1   1  0b.......01
  //  0b_____________110   3   1   1   2  0b......011
  //  0b____________1110   4   1   1   3  0b.....0111
  //  0b__________1_1110   5   1   1   4  0b....01111
  //  0b_________11_1110   6   1   1   5  0b...011111
  //  0b________111_1110   7   1   1   6  0b..0111111
  //                       8   0   2
  //  0b_____1_1111_1100   9   1   3   7  0b001111111
  //  0b____11_1111_1010  10   1   5   8  0b101111111  0b..0   (3 bits)
  //                      11   0  10
  //  0b__1111_1110_1100  12  19  19   9  0b101111111  0b001
  //  0b__1111_1110_1101  12      18  10  0b101111111  0b101
  //  0b__1111_1110_1110  12      17  11  0b101111111  0b011
  //  0b__1111_1110_1111  12      16  12  0b101111111  0b111
  //  0b__1111_1111_0000  12      15  13  0b011111111  0b000   (3 bits)
  //  0b__1111_1111_0001  12      14  14  0b011111111  0b100
  //  0b__1111_1111_0010  12      13  15  0b011111111  0b010
  //  0b__1111_1111_0011  12      12  16  0b011111111  0b110
  //  0b__1111_1111_0100  12      11  17  0b011111111  0b001
  //  0b__1111_1111_0101  12      10  18  0b011111111  0b101
  //  0b__1111_1111_0110  12       9  19  0b011111111  0b011
  //  0b__1111_1111_0111  12       8  20  0b011111111  0b111
  //  0b__1111_1111_1000  12       7  21  0b111111111  0b.000  (4 bits)
  //  0b__1111_1111_1001  12       6  22  0b111111111  0b.100
  //  0b__1111_1111_1010  12       5  23  0b111111111  0b.010
  //  0b__1111_1111_1011  12       4  24  0b111111111  0b.110
  //  0b__1111_1111_1100  12       3  25  0b111111111  0b.001
  //  0b__1111_1111_1101  12       2  26  0b111111111  0b.101
  //  0b__1111_1111_1110  12       1  27  0b111111111  0b.011
  //  0b1_1111_1111_1110  13   2   1  28  0b111111111  0b0111
  //  0b1_1111_1111_1111  13       0  29  0b111111111  0b1111
  //
  // cl  is the code_length.
  // c   is counts[code_length]
  // r   is the number of codes (of that code_length) remaining.
  // s   is the symbol
  // 1st is the key in the first level table (9 bits).
  // 2nd is the key in the second level table (variable bits).

  wuffs_deflate__decoder dec = ((wuffs_deflate__decoder){});
  wuffs_deflate__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);

  int i;
  int n = 0;
  dec.private_impl.f_code_lengths[n++] = 1;
  dec.private_impl.f_code_lengths[n++] = 2;
  dec.private_impl.f_code_lengths[n++] = 3;
  dec.private_impl.f_code_lengths[n++] = 4;
  dec.private_impl.f_code_lengths[n++] = 5;
  dec.private_impl.f_code_lengths[n++] = 6;
  dec.private_impl.f_code_lengths[n++] = 7;
  dec.private_impl.f_code_lengths[n++] = 9;
  dec.private_impl.f_code_lengths[n++] = 10;
  for (i = 0; i < 19; i++) {
    dec.private_impl.f_code_lengths[n++] = 12;
  }
  dec.private_impl.f_code_lengths[n++] = 13;
  dec.private_impl.f_code_lengths[n++] = 13;

  wuffs_base__status s = wuffs_deflate__decoder__init_huff(&dec, 0, 0, n, 257);
  if (s) {
    FAIL("%s", wuffs_deflate__status__string(s));
    return;
  }

  // There is one 1st-level table (9 bits), and three 2nd-level tables (3, 3
  // and 4 bits). f_huffs[0]'s elements should be non-zero for those tables and
  // should be zero outside of those tables.
  const int n_f_huffs = sizeof(dec.private_impl.f_huffs[0]) /
                        sizeof(dec.private_impl.f_huffs[0][0]);
  for (i = 0; i < n_f_huffs; i++) {
    bool got = dec.private_impl.f_huffs[0][i] == 0;
    bool want = i >= (1 << 9) + (1 << 3) + (1 << 3) + (1 << 4);
    if (got != want) {
      FAIL("huffs[0][%d] == 0: got %d, want %d", i, got, want);
      return;
    }
  }

  // The redirects in the 1st-level table should be at:
  //  - 0b101111111 (0x017F) to the table offset 512 (0x0200), a 3-bit table.
  //  - 0b011111111 (0x00FF) to the table offset 520 (0x0208), a 3-bit table.
  //  - 0b111111111 (0x01FF) to the table offset 528 (0x0210), a 4-bit table.
  uint32_t got;
  uint32_t want;
  got = dec.private_impl.f_huffs[0][0x017F];
  want = 0x10020039;
  if (got != want) {
    FAIL("huffs[0][0x017F]: got 0x%08" PRIX32 ", want 0x%08" PRIX32, got, want);
    return;
  }
  got = dec.private_impl.f_huffs[0][0x00FF];
  want = 0x10020839;
  if (got != want) {
    FAIL("huffs[0][0x00FF]: got 0x%08" PRIX32 ", want 0x%08" PRIX32, got, want);
    return;
  }
  got = dec.private_impl.f_huffs[0][0x01FF];
  want = 0x10021049;
  if (got != want) {
    FAIL("huffs[0][0x01FF]: got 0x%08" PRIX32 ", want 0x%08" PRIX32, got, want);
    return;
  }

  // The first 2nd-level table should look like wants.
  const uint32_t wants[8] = {
      0x80000801, 0x80000903, 0x80000801, 0x80000B03,
      0x80000801, 0x80000A03, 0x80000801, 0x80000C03,
  };
  for (i = 0; i < 8; i++) {
    got = dec.private_impl.f_huffs[0][0x0200 + i];
    want = wants[i];
    if (got != want) {
      FAIL("huffs[0][0x%04" PRIX32 "]: got 0x%08" PRIX32 ", want 0x%08" PRIX32,
           (uint32_t)(0x0200 + i), got, want);
      return;
    }
  }
}

  // ---------------- Mimic Tests

#ifdef WUFFS_MIMIC

void test_mimic_deflate_decode_256_bytes() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(mimic_deflate_decode, &deflate_256_bytes_gt, 0, 0);
}

void test_mimic_deflate_decode_deflate_backref_crosses_blocks() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(mimic_deflate_decode,
                     &deflate_deflate_backref_crosses_blocks_gt, 0, 0);
}

void test_mimic_deflate_decode_deflate_distance_32768() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(mimic_deflate_decode, &deflate_deflate_distance_32768_gt,
                     0, 0);
}

void test_mimic_deflate_decode_midsummer() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(mimic_deflate_decode, &deflate_midsummer_gt, 0, 0);
}

void test_mimic_deflate_decode_pi() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(mimic_deflate_decode, &deflate_pi_gt, 0, 0);
}

void test_mimic_deflate_decode_romeo() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(mimic_deflate_decode, &deflate_romeo_gt, 0, 0);
}

void test_mimic_deflate_decode_romeo_fixed() {
  CHECK_FOCUS(__func__);
  do_test_io_buffers(mimic_deflate_decode, &deflate_romeo_fixed_gt, 0, 0);
}

#endif  // WUFFS_MIMIC

// ---------------- Deflate Benches

void bench_wuffs_deflate_decode_1k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(wuffs_deflate_decode, tc_dst, &deflate_romeo_gt, 0, 0,
                      2000);
}

void bench_wuffs_deflate_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(wuffs_deflate_decode, tc_dst, &deflate_midsummer_gt, 0, 0,
                      300);
}

void bench_wuffs_deflate_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(wuffs_deflate_decode, tc_dst, &deflate_pi_gt, 0, 0, 30);
}

  // ---------------- Mimic Benches

#ifdef WUFFS_MIMIC

void bench_mimic_deflate_decode_1k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(mimic_deflate_decode, tc_dst, &deflate_romeo_gt, 0, 0,
                      2000);
}

void bench_mimic_deflate_decode_10k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(mimic_deflate_decode, tc_dst, &deflate_midsummer_gt, 0, 0,
                      300);
}

void bench_mimic_deflate_decode_100k() {
  CHECK_FOCUS(__func__);
  do_bench_io_buffers(mimic_deflate_decode, tc_dst, &deflate_pi_gt, 0, 0, 30);
}

#endif  // WUFFS_MIMIC

// ---------------- Manifest

// The empty comments forces clang-format to place one element per line.
proc tests[] = {

    test_wuffs_deflate_decode_256_bytes,                       //
    test_wuffs_deflate_decode_deflate_backref_crosses_blocks,  //
    test_wuffs_deflate_decode_deflate_distance_32768,          //
    test_wuffs_deflate_decode_midsummer,                       //
    test_wuffs_deflate_decode_pi,                              //
    test_wuffs_deflate_decode_pi_many_big_reads,               //
    test_wuffs_deflate_decode_pi_many_medium_reads,            //
    test_wuffs_deflate_decode_pi_many_small_writes_reads,      //
    test_wuffs_deflate_decode_romeo,                           //
    test_wuffs_deflate_decode_romeo_fixed,                     //
    test_wuffs_deflate_decode_split_src,                       //
    test_wuffs_deflate_history_full,                           //
    test_wuffs_deflate_history_partial,                        //
    test_wuffs_deflate_table_redirect,                         //

#ifdef WUFFS_MIMIC

    test_mimic_deflate_decode_256_bytes,                       //
    test_mimic_deflate_decode_deflate_backref_crosses_blocks,  //
    test_mimic_deflate_decode_deflate_distance_32768,          //
    test_mimic_deflate_decode_midsummer,                       //
    test_mimic_deflate_decode_pi,                              //
    test_mimic_deflate_decode_romeo,                           //
    test_mimic_deflate_decode_romeo_fixed,                     //

#endif  // WUFFS_MIMIC

    NULL,
};

// The empty comments forces clang-format to place one element per line.
proc benches[] = {

    bench_wuffs_deflate_decode_1k,    //
    bench_wuffs_deflate_decode_10k,   //
    bench_wuffs_deflate_decode_100k,  //

#ifdef WUFFS_MIMIC

    bench_mimic_deflate_decode_1k,    //
    bench_mimic_deflate_decode_10k,   //
    bench_mimic_deflate_decode_100k,  //

#endif  // WUFFS_MIMIC

    NULL,
};

int main(int argc, char** argv) {
  proc_package_name = "std/deflate";
  return test_main(argc, argv, tests, benches);
}
