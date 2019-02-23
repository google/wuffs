// Copyright 2018 The Wuffs Authors.
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

// Silence the nested slash-star warning for the next comment's command line.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcomment"

/*
This fuzzer (the fuzz function) is typically run indirectly, by a framework
such as https://github.com/google/oss-fuzz calling LLVMFuzzerTestOneInput.

When working on the fuzz implementation, or as a sanity check, defining
WUFFS_CONFIG__FUZZLIB_MAIN will let you manually run fuzz over a set of files:

gcc -DWUFFS_CONFIG__FUZZLIB_MAIN gif_fuzzer.c
./a.out ../../../test/data/*.gif
rm -f ./a.out

It should print "PASS", amongst other information, and exit(0).
*/

#pragma clang diagnostic pop

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../fuzzlib/fuzzlib.c"

const char* fuzz(wuffs_base__io_reader src_reader, uint32_t hash) {
  const char* ret = NULL;
  wuffs_base__slice_u8 pixbuf = ((wuffs_base__slice_u8){});
  wuffs_base__slice_u8 workbuf = ((wuffs_base__slice_u8){});

  // Use a {} code block so that "goto exit" doesn't trigger "jump bypasses
  // variable initialization" warnings.
  {
    wuffs_gif__decoder dec;
    const char* status = wuffs_gif__decoder__initialize(
        &dec, sizeof dec, WUFFS_VERSION,
        (hash & 1) ? WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED
                   : 0);
    if (status) {
      ret = status;
      goto exit;
    }

    wuffs_base__image_config ic = ((wuffs_base__image_config){});
    status = wuffs_gif__decoder__decode_image_config(&dec, &ic, src_reader);
    if (status) {
      ret = status;
      goto exit;
    }
    if (!wuffs_base__image_config__is_valid(&ic)) {
      ret = "invalid image_config";
      goto exit;
    }

    // Wuffs allows either statically or dynamically allocated work buffers.
    // This program exercises dynamic allocation.
    uint64_t n = wuffs_gif__decoder__workbuf_len(&dec).max_incl;
    if (n > 64 * 1024 * 1024) {  // Don't allocate more than 64 MiB.
      ret = "image too large";
      goto exit;
    }
    if (n > 0) {
      workbuf = wuffs_base__malloc_slice_u8(malloc, n);
      if (!workbuf.ptr) {
        ret = "out of memory";
        goto exit;
      }
    }

    n = wuffs_base__pixel_config__pixbuf_len(&ic.pixcfg);
    if (n > 64 * 1024 * 1024) {  // Don't allocate more than 64 MiB.
      ret = "image too large";
      goto exit;
    }
    if (n > 0) {
      pixbuf = wuffs_base__malloc_slice_u8(malloc, n);
      if (!pixbuf.ptr) {
        ret = "out of memory";
        goto exit;
      }
    }

    wuffs_base__pixel_buffer pb = ((wuffs_base__pixel_buffer){});
    status = wuffs_base__pixel_buffer__set_from_slice(&pb, &ic.pixcfg, pixbuf);
    if (status) {
      ret = status;
      goto exit;
    }

    bool seen_ok = false;
    while (true) {
      wuffs_base__frame_config fc = ((wuffs_base__frame_config){});
      status = wuffs_gif__decoder__decode_frame_config(&dec, &fc, src_reader);
      if (status) {
        if ((status != wuffs_base__warning__end_of_data) || !seen_ok) {
          ret = status;
        }
        goto exit;
      }

      status = wuffs_gif__decoder__decode_frame(&dec, &pb, src_reader, workbuf,
                                                NULL);

      wuffs_base__rect_ie_u32 frame_rect =
          wuffs_base__frame_config__bounds(&fc);
      wuffs_base__rect_ie_u32 dirty_rect =
          wuffs_gif__decoder__frame_dirty_rect(&dec);
      if (!wuffs_base__rect_ie_u32__contains_rect(&frame_rect, dirty_rect)) {
        ret = "internal error: frame_rect does not contain dirty_rect";
        goto exit;
      }

      if (status) {
        if ((status != wuffs_base__warning__end_of_data) || !seen_ok) {
          ret = status;
        }
        goto exit;
      }
      seen_ok = true;

      if (!wuffs_base__rect_ie_u32__equals(&frame_rect, dirty_rect)) {
        ret = "internal error: frame_rect does not equal dirty_rect";
        goto exit;
      }
    }
  }

exit:
  free(workbuf.ptr);
  free(pixbuf.ptr);
  return ret;
}
