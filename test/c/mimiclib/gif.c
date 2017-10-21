// Copyright 2017 The Puffs Authors.
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

#include "gif_lib.h"

int mimic_gif_read_func(GifFileType* f, GifByteType* ptr, int len) {
  puffs_base__buf1* src = (puffs_base__buf1*)(f->UserData);
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

const char* mimic_gif_decode(puffs_base__buf1* dst, puffs_base__buf1* src) {
  const char* ret = NULL;

  // TODO: #ifdef API calls for libgif version 4 vs version 5? Note that:
  //  - Version 4 ships with Ubunty 14.04 LTS "Trusty".
  //  - Version 5 ships with Ubunty 16.04 LTS "Xenial".
  GifFileType* f = DGifOpen(src, mimic_gif_read_func);
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

  // Copy the pixel data from the GifFileType* f to the dst buffer, since the
  // former is free'd at the end of this function.
  //
  // In theory, this mimic_gif_decode function might be faster overall if the
  // DGifSlurp call above decoded the pixel data directly into dst instead of
  // into an intermediate buffer that needed to be malloc'ed and then free'd.
  // In practice, doing so did not seem to show a huge difference. (See commit
  // ab7e0ae "Add a custom gif_mimic_DGifSlurp function.") It also further
  // complicates supporting both versions 4 and 5 of giflib. That commit was
  // therefore rolled back.
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
