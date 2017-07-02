// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "gif_lib.h"

int gif_mimic_read_func(GifFileType* f, GifByteType* ptr, int len) {
  puffs_base_buf1* src = (puffs_base_buf1*)(f->UserData);
  if (len < 0) {
    return 0;
  }
  size_t n = (size_t)(len);
  size_t num_src = src->wi - src->ri;
  if (n > num_src) {
    n = num_src;
  }
  memmove(ptr, src->ptr + src->ri, n);
  src->ri += n;
  return n;
}

const char* gif_mimic_decode(puffs_base_buf1* dst, puffs_base_buf1* src) {
  const char* ret = NULL;

  // TODO: update API calls for libgif version 5 vs version 4?
  GifFileType* f = DGifOpen(src, gif_mimic_read_func);
  if (!f) {
    ret = "DGifOpen failed";
    goto cleanup0;
  }
  if (DGifSlurp(f) != GIF_OK) {
    ret = "DGifSlurp failed";
    goto cleanup1;
  }

  // TODO: have Puffs accept multi-frame (animated) GIFs.
  if (f->ImageCount != 1) {
    ret = "GIF image has more than one frame";
    goto cleanup1;
  }

  struct SavedImage* si = &f->SavedImages[0];
  size_t num_src =
      (size_t)(si->ImageDesc.Width) * (size_t)(si->ImageDesc.Height);
  size_t num_dst = dst->len - dst->wi;
  if (num_dst < num_src) {
    ret = "GIF image's pixel data won't fit in the dst buffer";
    goto cleanup1;
  }

  memmove(dst->ptr + dst->wi, si->RasterBits, num_src);
  dst->wi += num_src;

cleanup1:;
  if ((DGifCloseFile(f) != GIF_OK) && !ret) {
    ret = "DGifCloseFile failed";
  }
cleanup0:;
  return ret;
}
