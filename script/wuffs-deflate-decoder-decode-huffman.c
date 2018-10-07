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

// This file contains a hand-written C implementation of
// release/c/wuffs-unsupported-snapshot.h's generated
// wuffs_deflate__decoder__decode_huffman_fast function.
//
// It is not intended to be used in production settings, on untrusted data. Its
// purpose is to give a rough upper bound on how fast Wuffs' generated C code
// can be, with a sufficiently smart Wuffs compiler.
//
// To repeat, substituting in this C implementation is NOT SAFE, and may result
// in buffer overflows. This code exists only to aid optimization of the (safe)
// std/deflate/*.wuffs code and the Wuffs compiler's code generation.
//
// ----------------
//
// Having said that, to generate the benchmark numbers with this hand-written C
// implementation, edit release/c/wuffs-unsupported-snapshot.h and find the
// lines that say
//
// // WUFFS C HEADER ENDS HERE.
// #ifdef WUFFS_IMPLEMENTATION
//
// After that, add this line:
//
// #include "../../script/wuffs-deflate-decoder-decode-huffman.c"
//
// Then find the call to wuffs_deflate__decoder__decode_huffman_fast. It should
// be inside the wuffs_deflate__decoder__decode_blocks function body, and the
// lines of code should look something like
//
// status = wuffs_deflate__decoder__decode_huffman_fast(self, a_dst, a_src);
//
// Change the "wuffs" to "c_wuffs", i.e. add a "c_" prefix. The net result
// should look something like:
//
// clang-format off
/*

$ git diff release/c/wuffs-unsupported-snapshot.h
diff --git a/release/c/wuffs-unsupported-snapshot.h b/release/c/wuffs-unsupported-snapshot.h
index f240b9b..33d7089 100644
--- a/release/c/wuffs-unsupported-snapshot.h
+++ b/release/c/wuffs-unsupported-snapshot.h
@@ -3128,6 +3128,8 @@ struct wuffs_zlib__decoder__struct {
 // WUFFS C HEADER ENDS HERE.
 #ifdef WUFFS_IMPLEMENTATION

+#include "../../script/wuffs-deflate-decoder-decode-huffman.c"
+
 // Copyright 2017 The Wuffs Authors.
 //
 // Licensed under the Apache License, Version 2.0 (the "License");
@@ -4940,7 +4942,7 @@ wuffs_deflate__decoder__decode_blocks(wuffs_deflate__decoder* self,
         a_src.private_impl.buf->meta.ri =
             iop_a_src - a_src.private_impl.buf->data.ptr;
       }
-      status = wuffs_deflate__decoder__decode_huffman_fast(self, a_dst, a_src);
+      status = c_wuffs_deflate__decoder__decode_huffman_fast(self, a_dst, a_src);
       if (a_src.private_impl.buf) {
         iop_a_src =
             a_src.private_impl.buf->data.ptr + a_src.private_impl.buf->meta.ri;

*/
// clang-format on
//
// That concludes the two edits to release/c/wuffs-unsupported-snapshot.h. Run
// the tests and benchmarks with the "-skipgen" flag, otherwise the "wuffs"
// tool will re-generate the C code and override your edits:
//
// wuffs test  -skipgen std/deflate
// wuffs bench -skipgen std/deflate
//
// You may also want to focus on one specific test, e.g.:
//
// wuffs test  -skipgen -focus=wuffs_deflate_decode_midsummer std/deflate
// wuffs bench -skipgen -focus=wuffs_deflate_decode_100k      std/deflate

#include <stddef.h>
#include <stdio.h>  // For manual printf debugging.

extern const char*
    wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state;

// Define this macro to further speed up this C implementation, using two
// techniques that are not used by the zlib-the-library C implementation (as of
// version 1.2.11, current as of November 2017). Doing so gives data for "how
// fast can Wuffs' zlib-the-format implementation be" instead of "is Wuffs'
// generated C code as fast as zlib-the-library's hand-written C code".
//
// Whether the same techniques could apply to zlib-the-library is discussed at
// https://github.com/madler/zlib/pull/292
#ifdef __x86_64__
//#define WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS
#endif

// wuffs_base__width_to_mask_table is the width_to_mask_table from
// https://fgiesen.wordpress.com/2018/02/19/reading-bits-in-far-too-many-ways-part-1/
//
// Look for "It may feel ridiculous" on that page for the rationale.
static const uint32_t wuffs_base__width_to_mask_table[33] = {
    0x00000000u,

    0x00000001u, 0x00000003u, 0x00000007u, 0x0000000Fu,
    0x0000001Fu, 0x0000003Fu, 0x0000007Fu, 0x000000FFu,

    0x000001FFu, 0x000003FFu, 0x000007FFu, 0x00000FFFu,
    0x00001FFFu, 0x00003FFFu, 0x00007FFFu, 0x0000FFFFu,

    0x0001FFFFu, 0x0003FFFFu, 0x0007FFFFu, 0x000FFFFFu,
    0x001FFFFFu, 0x003FFFFFu, 0x007FFFFFu, 0x00FFFFFFu,

    0x01FFFFFFu, 0x03FFFFFFu, 0x07FFFFFFu, 0x0FFFFFFFu,
    0x1FFFFFFFu, 0x3FFFFFFFu, 0x7FFFFFFFu, 0xFFFFFFFFu,
};

// This is the generated function that we are explicitly overriding. Note that
// the function name is "wuffs_etc", not "c_wuffs_etc".
static wuffs_base__status wuffs_deflate__decoder__decode_huffman_fast(
    wuffs_deflate__decoder* self,
    wuffs_base__io_writer a_dst,
    wuffs_base__io_reader a_src);

// This is the overriding implementation.
wuffs_base__status c_wuffs_deflate__decoder__decode_huffman_fast(
    wuffs_deflate__decoder* self,
    wuffs_base__io_writer a_dst,
    wuffs_base__io_reader a_src) {
  // Avoid the -Werror=unused-function warning for the now-unused
  // overridden wuffs_deflate__decoder__decode_huffman_fast.
  (void)(wuffs_deflate__decoder__decode_huffman_fast);

  if (!a_dst.private_impl.buf || !a_src.private_impl.buf) {
    return wuffs_base__error__bad_argument;
  }
  wuffs_base__status z = NULL;

  // Load contextual state. Prepare to check that pdst and psrc remain within
  // a_dst's and a_src's bounds.
  uint8_t* pdst =
      a_dst.private_impl.buf->data.ptr + a_dst.private_impl.buf->meta.wi;
  uint8_t* qdst =
      a_dst.private_impl.buf->data.ptr + a_dst.private_impl.buf->data.len;
  if ((qdst - pdst) < 258) {
    return NULL;
  } else {
    qdst -= 258;
  }
  uint8_t* psrc =
      a_src.private_impl.buf->data.ptr + a_src.private_impl.buf->meta.ri;
  uint8_t* qsrc =
      a_src.private_impl.buf->data.ptr + a_src.private_impl.buf->meta.wi;
  if ((qsrc - psrc) < 12) {
    return NULL;
  } else {
    qsrc -= 12;
  }
#if defined(WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS)
  uint64_t bits = self->private_impl.f_bits;
#else
  uint32_t bits = self->private_impl.f_bits;
#endif
  uint32_t n_bits = self->private_impl.f_n_bits;

  // Initialize other local variables.
  uint8_t* pdst0 = pdst;
  uint32_t lmask =
      wuffs_base__width_to_mask_table[self->private_impl.f_n_huffs_bits[0]];
  uint32_t dmask =
      wuffs_base__width_to_mask_table[self->private_impl.f_n_huffs_bits[1]];

outer_loop:
  while ((pdst <= qdst) && (psrc <= qsrc)) {
#if defined(WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS)
    // Ensure that we have at least 56 bits of input.
    //
    // This is "Variant 4" of
    // https://fgiesen.wordpress.com/2018/02/20/reading-bits-in-far-too-many-ways-part-2/
    //
    // 56, the number of bits in 7 bytes, is a property of that "Variant 4"
    // bit-reading technique, and not related to the DEFLATE format per se.
    //
    // Specifically for DEFLATE, we need only up to 48 bits per outer_loop
    // iteration. The maximum input bits used by a length/distance pair is 15
    // bits for the length code, 5 bits for the length extra, 15 bits for the
    // distance code, and 13 bits for the distance extra. This totals 48 bits.
    //
    // The fact that the 48 we need is less than the 56 we get is a happy
    // coincidence. It lets us eliminate any other loads in the loop body.
    bits |= *((uint64_t*)psrc) << n_bits;
    psrc += (63 - n_bits) >> 3;
    n_bits |= 56;
#else
    // Ensure that we have at least 15 bits of input.
    if (n_bits < 15) {
      bits |= ((uint32_t)(*psrc++)) << n_bits;
      n_bits += 8;
      bits |= ((uint32_t)(*psrc++)) << n_bits;
      n_bits += 8;
    }
#endif

    // Decode an lcode symbol from H-L.
    uint32_t table_entry = self->private_impl.f_huffs[0][bits & lmask];
    while (true) {
      uint32_t n = table_entry & 0x0F;
      bits >>= n;
      n_bits -= n;
      if ((table_entry >> 31) != 0) {
        // Literal.
        *pdst++ = (uint8_t)(table_entry >> 8);
        goto outer_loop;
      }
      if ((table_entry >> 30) != 0) {
        // Back-reference; length = base_number + extra_bits.
        break;
      }
      if ((table_entry >> 29) != 0) {
        // End of block.
        self->private_impl.f_end_of_block = true;
        goto end;
      }
      if ((table_entry >> 24) != 0x10) {
        z = wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state;
        goto end;
      }
      uint32_t top = (table_entry >> 8) & 0xFFFF;
      uint32_t mask =
          wuffs_base__width_to_mask_table[(table_entry >> 4) & 0x0F];
      table_entry = self->private_impl.f_huffs[0][top + (bits & mask)];
    }

    // length = base_number + extra_bits.
    uint32_t length = (table_entry >> 8) & 0xFFFF;
    {
      uint32_t n = (table_entry >> 4) & 0x0F;
      if (n) {
#if !defined(WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS)
        if (n_bits < n) {
          bits |= ((uint32_t)(*psrc++)) << n_bits;
          n_bits += 8;
        }
#endif
        length += bits & wuffs_base__width_to_mask_table[n];
        bits >>= n;
        n_bits -= n;
      }
    }

#if !defined(WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS)
    // Ensure that we have at least 15 bits of input.
    if (n_bits < 15) {
      bits |= ((uint32_t)(*psrc++)) << n_bits;
      n_bits += 8;
      bits |= ((uint32_t)(*psrc++)) << n_bits;
      n_bits += 8;
    }
#endif

    // Decode a dcode symbol from H-D.
    table_entry = self->private_impl.f_huffs[1][bits & dmask];
    while (true) {
      uint32_t n = table_entry & 0x0F;
      bits >>= n;
      n_bits -= n;
      if ((table_entry >> 30) != 0) {
        // Back-reference; distance = base_number + extra_bits.
        break;
      }
      if ((table_entry >> 24) != 0x10) {
        z = wuffs_deflate__error__internal_error_inconsistent_huffman_decoder_state;
        goto end;
      }
      uint32_t top = (table_entry >> 8) & 0xFFFF;
      uint32_t mask =
          wuffs_base__width_to_mask_table[(table_entry >> 4) & 0x0F];
      table_entry = self->private_impl.f_huffs[1][top + (bits & mask)];
    }

    // dist_minus_1 = base_number_minus_1 + extra_bits + 1.
    // distance = dist_minus_1 + 1.
    uint32_t dist_minus_1 = (table_entry >> 8) & 0xFFFF;
    {
      uint32_t n = (table_entry >> 4) & 0x0F;
#if !defined(WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS)
      // Ensure that we have at least 15 bits of input.
      if (n_bits < n) {
        bits |= ((uint32_t)(*psrc++)) << n_bits;
        n_bits += 8;
        bits |= ((uint32_t)(*psrc++)) << n_bits;
        n_bits += 8;
      }
#endif
      dist_minus_1 += bits & wuffs_base__width_to_mask_table[n];
      bits >>= n;
      n_bits -= n;
    }

    // TODO: look at a sliding window, not just output written so far to dst.
    if ((ptrdiff_t)(dist_minus_1 + 1) > (pdst - pdst0)) {
      z = wuffs_base__error__bad_argument;
      goto end;
    }

    uint8_t* pback = pdst - (dist_minus_1 + 1);

#if defined(WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS)
    // Back-copy fast path, copying 8 instead of 1 bytes at a time.
    //
    // This always copies 8*N bytes (where N is the smallest integer such that
    // 8*N >= length, i.e. we round length up to a multiple of 8), instead of
    // only length bytes, but that's OK, as subsequent iterations will fix up
    // the overrun.
    if (dist_minus_1 + 1 >= 8) {
      while (1) {
        *((uint64_t*)(pdst)) = *((uint64_t*)(pback));
        if (length <= 8) {
          pdst += length;
          break;
        }
        pdst += 8;
        pback += 8;
        length -= 8;
      }
      continue;
    }
#endif

    // Back-copy slow path.
    for (; length >= 3; length -= 3) {
      *pdst++ = *pback++;
      *pdst++ = *pback++;
      *pdst++ = *pback++;
    }
    for (; length; length--) {
      *pdst++ = *pback++;
    }
  }

end:
  // Return unused input bytes.
  for (; n_bits >= 8; n_bits -= 8) {
    psrc--;
  }
  bits &= wuffs_base__width_to_mask_table[n_bits];

  // Save contextual state.
  a_dst.private_impl.buf->meta.wi = pdst - a_dst.private_impl.buf->data.ptr;
  a_src.private_impl.buf->meta.ri = psrc - a_src.private_impl.buf->data.ptr;
  self->private_impl.f_bits = bits;
  self->private_impl.f_n_bits = n_bits;

  return z;
}
