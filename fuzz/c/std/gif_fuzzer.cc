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

g++ -DWUFFS_CONFIG__FUZZLIB_MAIN gif_fuzzer.cc
./a.out ../../../test/data/*.gif
rm -f ./a.out

It should print "PASS", amongst other information, and exit(0).
*/

#pragma clang diagnostic pop

// If building this program in an environment that doesn't easily accomodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../gen/c/std/gif.c"
#include "../fuzzlib/fuzzlib.cc"

void fuzz(wuffs_base__reader1 src_reader, uint32_t hash) {
  void* pixbuf = NULL;

  // Use a {} code block so that "goto exit" doesn't trigger "jump bypasses
  // variable initialization" warnings.
  {
    wuffs_gif__status s;
    wuffs_gif__decoder dec;
    wuffs_gif__decoder__initialize(&dec, WUFFS_VERSION, 0);

    wuffs_base__image_config ic = {{0}};
    s = wuffs_gif__decoder__decode_config(&dec, &ic, src_reader);
    if (s || !wuffs_base__image_config__valid(&ic)) {
      goto exit;
    }

    size_t pixbuf_size = wuffs_base__image_config__pixbuf_size(&ic);
    // Don't try to allocate more than 64 MiB.
    if (pixbuf_size > 64 * 1024 * 1024) {
      goto exit;
    }
    pixbuf = malloc(pixbuf_size);
    if (!pixbuf) {
      goto exit;
    }

    wuffs_base__buf1 dst = {.ptr = (uint8_t*)(pixbuf), .len = pixbuf_size};
    wuffs_base__writer1 dst_writer = {.buf = &dst};

    while (true) {
      // TODO: handle the frame rect being larger than the image rect. The
      // GIF89a spec doesn't disallow this and the Wuffs code tolerates it, in
      // that it returns a "short write" (because the dst buffer is too small)
      // and will not e.g. write past the buffer bounds. But the Wuffs GIF API
      // should somehow pass a subset of pixbuf to decode_frame.
      dst.wi = 0;
      s = wuffs_gif__decoder__decode_frame(&dec, dst_writer, src_reader);
      if (s) {
        break;
      }
    }
  }

exit:
  if (pixbuf) {
    free(pixbuf);
  }
}
