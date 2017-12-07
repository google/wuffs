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

// This file contains a hand-written C implementation of gen/c/std/deflate.c's
// generated wuffs_deflate__decoder__decode_huffman_fast function.
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
// implementation, edit gen/c/std/deflate.c and find the line that says
//
// C HEADER ENDS HERE.
//
// After that, add this line:
//
// #include "../../../script/wuffs-deflate-decoder-decode-huffman.c"
//
// Then find the call to wuffs_deflate__decoder__decode_huffman_fast. It should
// be inside the wuffs_deflate__decoder__decode_blocks function body, and the
// lines of code should look something like
//
// status = wuffs_deflate__decoder__decode_huffman_fast(self, a_dst, a_src);
//
// Change the "wuffs" to "c_wuffs", i.e. add a "c_" prefix. The net result
// should look something like:

/*
$ git diff gen/c/std/deflate.c
diff --git a/gen/c/std/deflate.c b/gen/c/std/deflate.c
index 3698e98..4bee3b0 100644
--- a/gen/c/std/deflate.c
+++ b/gen/c/std/deflate.c
@@ -279,6 +279,8 @@ wuffs_deflate__status wuffs_deflate__decoder__decode(
 #endif  // WUFFS_DEFLATE_H

 // C HEADER ENDS HERE.
+#include "../../../script/wuffs-deflate-decoder-decode-huffman.c"
+

 #ifndef WUFFS_BASE_IMPL_H
 #define WUFFS_BASE_IMPL_H
@@ -1050,7 +1052,7 @@ static wuffs_deflate__status wuffs_deflate__decoder__decode_blocks(
           }
         }
       }
-      status = wuffs_deflate__decoder__decode_huffman_fast(self, a_dst, a_src);
+      status = c_wuffs_deflate__decoder__decode_huffman_fast(self, a_dst, a_src);
       if (a_src.buf) {
         b_rptr_src = a_src.buf->ptr + a_src.buf->ri;
       }
*/

// That concludes the two edits to gen/c/std/deflate.c. Run the tests and
// benchmarks with the "-skipgen" flag, otherwise the "wuffs" tool will
// re-generate the C code and override your gen/c/std/deflate.c edit:
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

// This is the generated function that we are explicitly overriding. Note that
// the function name is "wuffs_etc", not "c_wuffs_etc".
static wuffs_deflate__status wuffs_deflate__decoder__decode_huffman_fast(
    wuffs_deflate__decoder* self,
    wuffs_base__writer1 a_dst,
    wuffs_base__reader1 a_src);

// This is the overriding implementation.
wuffs_deflate__status c_wuffs_deflate__decoder__decode_huffman_fast(
    wuffs_deflate__decoder* self,
    wuffs_base__writer1 a_dst,
    wuffs_base__reader1 a_src) {
  // Avoid the -Werror=unused-function warning for the now-unused
  // overridden wuffs_deflate__decoder__decode_huffman_fast.
  (void)(wuffs_deflate__decoder__decode_huffman_fast);

  if (!a_dst.buf || !a_src.buf) {
    return WUFFS_DEFLATE__ERROR_BAD_ARGUMENT;
  }
  wuffs_deflate__status status = WUFFS_DEFLATE__STATUS_OK;

  // Load contextual state. Prepare to check that pdst and psrc remain within
  // a_dst's and a_src's bounds.
  uint8_t* pdst = a_dst.buf->ptr + a_dst.buf->wi;
  uint8_t* qdst = a_dst.buf->ptr + a_dst.buf->len;
  if ((qdst - pdst) < 258) {
    return WUFFS_DEFLATE__STATUS_OK;
  } else {
    qdst -= 258;
  }
  uint8_t* psrc = a_src.buf->ptr + a_src.buf->ri;
  uint8_t* qsrc = a_src.buf->ptr + a_src.buf->wi;
  if ((qsrc - psrc) < 12) {
    return WUFFS_DEFLATE__STATUS_OK;
  } else {
    qsrc -= 12;
  }
#ifdef WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS
  uint64_t bits = self->private_impl.f_bits;
#else
  uint32_t bits = self->private_impl.f_bits;
#endif
  uint32_t n_bits = self->private_impl.f_n_bits;

  // Initialize other local variables.
  uint8_t* pdst0 = pdst;
  uint32_t lmask =
      ((((uint32_t)(1)) << self->private_impl.f_n_huffs_bits[0]) - 1);
  uint32_t dmask =
      ((((uint32_t)(1)) << self->private_impl.f_n_huffs_bits[1]) - 1);

outer_loop:
  while ((pdst <= qdst) && (psrc <= qsrc)) {
    // Ensure that we have at least 15 bits of input.
    if (n_bits < 15) {
#ifdef WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS
      bits |= *((uint64_t*)psrc) << n_bits;
      psrc += 6;
      n_bits += 48;
#else
      bits |= ((uint32_t)(*psrc++)) << n_bits;
      n_bits += 8;
      bits |= ((uint32_t)(*psrc++)) << n_bits;
      n_bits += 8;
#endif
    }

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
        // Back-reference; length = base number + extra bits.
        break;
      }
      if ((table_entry >> 29) != 0) {
        // End of block.
        self->private_impl.f_end_of_block = true;
        goto end;
      }
      if ((table_entry >> 24) != 0x10) {
        status =
            WUFFS_DEFLATE__ERROR_INTERNAL_ERROR_INCONSISTENT_HUFFMAN_DECODER_STATE;
        goto end;
      }
      uint32_t top = (table_entry >> 8) & 0xFFFF;
      uint32_t mask = ((((uint32_t)(1)) << ((table_entry >> 4) & 0x0F)) - 1);
      table_entry = self->private_impl.f_huffs[0][top + (bits & mask)];
    }

    // length = base number + extra bits.
    uint32_t length = (table_entry >> 8) & 0xFFFF;
    {
      uint32_t n = (table_entry >> 4) & 0x0F;
      if (n) {
        if (n_bits < n) {
#ifdef WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS
          bits |= *((uint64_t*)psrc) << n_bits;
          psrc += 6;
          n_bits += 48;
#else
          bits |= ((uint32_t)(*psrc++)) << n_bits;
          n_bits += 8;
#endif
        }
        length += bits & ((((uint32_t)(1)) << n) - 1);
        bits >>= n;
        n_bits -= n;
      }
    }

    // Ensure that we have at least 15 bits of input.
    if (n_bits < 15) {
#ifdef WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS
      bits |= *((uint64_t*)psrc) << n_bits;
      psrc += 6;
      n_bits += 48;
#else
      bits |= ((uint32_t)(*psrc++)) << n_bits;
      n_bits += 8;
      bits |= ((uint32_t)(*psrc++)) << n_bits;
      n_bits += 8;
#endif
    }

    // Decode a dcode symbol from H-D.
    table_entry = self->private_impl.f_huffs[1][bits & dmask];
    while (true) {
      uint32_t n = table_entry & 0x0F;
      bits >>= n;
      n_bits -= n;
      if ((table_entry >> 30) != 0) {
        // Back-reference; distance = base number + extra bits.
        break;
      }
      if ((table_entry >> 24) != 0x10) {
        status =
            WUFFS_DEFLATE__ERROR_INTERNAL_ERROR_INCONSISTENT_HUFFMAN_DECODER_STATE;
        goto end;
      }
      uint32_t top = (table_entry >> 8) & 0xFFFF;
      uint32_t mask = ((((uint32_t)(1)) << ((table_entry >> 4) & 0x0F)) - 1);
      table_entry = self->private_impl.f_huffs[1][top + (bits & mask)];
    }

    // distance = base number + extra bits.
    uint32_t distance = (table_entry >> 8) & 0xFFFF;
    {
      uint32_t n = (table_entry >> 4) & 0x0F;
      if (n) {
        if (n_bits < 15) {
#ifdef WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS
          bits |= *((uint64_t*)psrc) << n_bits;
          psrc += 6;
          n_bits += 48;
#else
          bits |= ((uint32_t)(*psrc++)) << n_bits;
          n_bits += 8;
          bits |= ((uint32_t)(*psrc++)) << n_bits;
          n_bits += 8;
#endif
        }
        distance += bits & ((((uint32_t)(1)) << n) - 1);
        bits >>= n;
        n_bits -= n;
      }
    }

    // TODO: look at a sliding window, not just output written so far to dst.
    if ((ptrdiff_t)(distance) > (pdst - pdst0)) {
      status = WUFFS_DEFLATE__ERROR_BAD_ARGUMENT;
      goto end;
    }

    uint8_t* pback = pdst - distance;

#ifdef WUFFS_DEFLATE__HAVE_64_BIT_UNALIGNED_LITTLE_ENDIAN_LOADS
    // Back-copy fast path, copying 8 instead of 1 bytes at a time.
    //
    // This always copies 8*N bytes (where N is the smallest integer such that
    // 8*N >= length, i.e. we round length up to a multiple of 8), instead of
    // only length bytes, but that's OK, as subsequent iterations will fix up
    // the overrun.
    if (distance >= 8) {
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
  bits &= (((uint32_t)(1)) << n_bits) - 1;

  // Save contextual state.
  a_dst.buf->wi = pdst - a_dst.buf->ptr;
  a_src.buf->ri = psrc - a_src.buf->ptr;
  self->private_impl.f_bits = bits;
  self->private_impl.f_n_bits = n_bits;

  return status;
}
