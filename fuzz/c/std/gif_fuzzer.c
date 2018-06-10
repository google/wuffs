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

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../gen/c/base.c"
#include "../../../gen/c/std/gif.c"
#include "../../../gen/c/std/lzw.c"
#include "../fuzzlib/fuzzlib.c"

const char* fuzz(wuffs_base__io_reader src_reader, uint32_t hash) {
  const char* ret = NULL;
  void* pixbuf = NULL;

  // Use a {} code block so that "goto exit" doesn't trigger "jump bypasses
  // variable initialization" warnings.
  {
    wuffs_base__status s = 0;
    wuffs_gif__decoder dec = ((wuffs_gif__decoder){});
    wuffs_gif__decoder__check_wuffs_version(&dec, sizeof dec, WUFFS_VERSION);

    wuffs_base__image_buffer ib = ((wuffs_base__image_buffer){});
    wuffs_base__image_config ic = ((wuffs_base__image_config){});
    s = wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
    if (s) {
      ret = wuffs_gif__status__string(s);
      goto exit;
    }
    if (!wuffs_base__image_config__is_valid(&ic)) {
      ret = "invalid image_config";
      goto exit;
    }

    size_t pixbuf_size = wuffs_base__image_config__pixbuf_size(&ic);
    // Don't try to allocate more than 64 MiB.
    if (pixbuf_size > 64 * 1024 * 1024) {
      ret = "image too large";
      goto exit;
    }
    pixbuf = malloc(pixbuf_size);
    if (!pixbuf) {
      ret = "out of memory";
      goto exit;
    }
    // TODO: check wuffs_base__image_buffer__set_from_slice errors?
    wuffs_base__image_buffer__set_from_slice(
        &ib, ic, ((wuffs_base__slice_u8){.ptr = pixbuf, .len = pixbuf_size}));

    bool seen_ok = false;
    while (true) {
      s = wuffs_gif__decoder__decode_frame(&dec, &ib, src_reader,
                                           ((wuffs_base__slice_u8){}));
      if (s) {
        if ((s == WUFFS_BASE__SUSPENSION_END_OF_DATA) && seen_ok) {
          s = WUFFS_BASE__STATUS_OK;
        }
        ret = wuffs_gif__status__string(s);
        goto exit;
      }
      seen_ok = true;
    }
  }

exit:
  if (pixbuf) {
    free(pixbuf);
  }
  return ret;
}
