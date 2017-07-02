// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include <stdlib.h>

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

// gif_mimic_DGifSlurp is based on the DGifSlurp function in giflib-4.1.6 (the
// version in Ubunty 14.04 LTS "Trusty"). It is modified to avoid malloc'ing a
// temporary buffer. Instead, the destination buffer is an additional argument.
int gif_mimic_DGifSlurp(GifFileType* GifFile, puffs_base_buf1* dst) {
  int ImageSize;
  GifRecordType RecordType;
  SavedImage* sp;
  GifByteType* ExtData;
  SavedImage temp_save;

  temp_save.ExtensionBlocks = NULL;
  temp_save.ExtensionBlockCount = 0;

  do {
    if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR)
      return (GIF_ERROR);

    switch (RecordType) {
      case IMAGE_DESC_RECORD_TYPE:
        if (DGifGetImageDesc(GifFile) == GIF_ERROR)
          return (GIF_ERROR);

        sp = &GifFile->SavedImages[GifFile->ImageCount - 1];
        ImageSize = sp->ImageDesc.Width * sp->ImageDesc.Height;

        // Puffs modification: avoid a memmove by decompressing straight into
        // the destination buffer.
        //
        // Original DGifSlurp code:
        // sp->RasterBits =
        //   (unsigned char*)malloc(ImageSize * sizeof(GifPixelType));
        size_t num_src =
            (size_t)(sp->ImageDesc.Width) * (size_t)(sp->ImageDesc.Height);
        size_t num_dst = dst->len - dst->wi;
        if (num_dst < num_src) {
          return (GIF_ERROR);
        }
        sp->RasterBits = dst->ptr + dst->wi;
        dst->wi += num_src;
        // End Puffs modification.

        if (sp->RasterBits == NULL) {
          return GIF_ERROR;
        }
        if (DGifGetLine(GifFile, sp->RasterBits, ImageSize) == GIF_ERROR)
          return (GIF_ERROR);
        if (temp_save.ExtensionBlocks) {
          sp->ExtensionBlocks = temp_save.ExtensionBlocks;
          sp->ExtensionBlockCount = temp_save.ExtensionBlockCount;

          temp_save.ExtensionBlocks = NULL;
          temp_save.ExtensionBlockCount = 0;

          /* FIXME: The following is wrong.  It is left in only for
           * backwards compatibility.  Someday it should go away. Use
           * the sp->ExtensionBlocks->Function variable instead. */
          sp->Function = sp->ExtensionBlocks[0].Function;
        }
        break;

      case EXTENSION_RECORD_TYPE:
        if (DGifGetExtension(GifFile, &temp_save.Function, &ExtData) ==
            GIF_ERROR)
          return (GIF_ERROR);
        while (ExtData != NULL) {
          /* Create an extension block with our data */
          if (AddExtensionBlock(&temp_save, ExtData[0], &ExtData[1]) ==
              GIF_ERROR)
            return (GIF_ERROR);

          if (DGifGetExtensionNext(GifFile, &ExtData) == GIF_ERROR)
            return (GIF_ERROR);
          temp_save.Function = 0;
        }
        break;

      case TERMINATE_RECORD_TYPE:
        break;

      default: /* Should be trapped by DGifGetRecordType */
        break;
    }
  } while (RecordType != TERMINATE_RECORD_TYPE);

  /* Just in case the Gif has an extension block without an associated
   * image... (Should we save this into a savefile structure with no image
   * instead? Have to check if the present writing code can handle that as
   * well.... */
  if (temp_save.ExtensionBlocks)
    FreeExtension(&temp_save);

  return (GIF_OK);
}

const char* gif_mimic_decode(puffs_base_buf1* dst, puffs_base_buf1* src) {
  const char* ret = NULL;

  // TODO: update API calls for libgif version 5 vs version 4?
  GifFileType* f = DGifOpen(src, gif_mimic_read_func);
  if (!f) {
    ret = "DGifOpen failed";
    goto cleanup0;
  }

  if (gif_mimic_DGifSlurp(f, dst) != GIF_OK) {
    ret = "DGifSlurp failed";
    goto cleanup1;
  }

  // TODO: have Puffs accept multi-frame (animated) GIFs.
  if (f->ImageCount != 1) {
    ret = "GIF image has more than one frame";
    goto cleanup1;
  }

cleanup1:;
  // See "Puffs modification" in gif_mimic_DGifSlurp above.
  SavedImage* sp;
  for (sp = f->SavedImages; sp < f->SavedImages + f->ImageCount; sp++) {
    sp->RasterBits = NULL;
  }

  if ((DGifCloseFile(f) != GIF_OK) && !ret) {
    ret = "DGifCloseFile failed";
  }
cleanup0:;
  return ret;
}
