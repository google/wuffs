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

When working on the fuzz implementation, or as a coherence check, defining
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

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c choose which parts of Wuffs to build. That file contains the
// entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__GIF
#define WUFFS_CONFIG__MODULE__LZW

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../../release/c/wuffs-unsupported-snapshot.c"
#include "../fuzzlib/fuzzlib.c"
#include "../fuzzlib/fuzzlib_image_decoder.c"

void set_quirks(wuffs_gif__decoder* dec, uint64_t hash) {
  uint32_t quirks[] = {
      WUFFS_GIF__QUIRK_DELAY_NUM_DECODED_FRAMES,
      WUFFS_GIF__QUIRK_FIRST_FRAME_LOCAL_PALETTE_MEANS_BLACK_BACKGROUND,
      WUFFS_GIF__QUIRK_HONOR_BACKGROUND_COLOR,
      WUFFS_GIF__QUIRK_IGNORE_TOO_MUCH_PIXEL_DATA,
      WUFFS_GIF__QUIRK_IMAGE_BOUNDS_ARE_STRICT,
      WUFFS_GIF__QUIRK_REJECT_EMPTY_FRAME,
      WUFFS_GIF__QUIRK_REJECT_EMPTY_PALETTE,
      0,
  };

  uint32_t i;
  for (i = 0; quirks[i]; i++) {
    uint64_t bit = 1 << (i & 63);
    if (hash & bit) {
      wuffs_gif__decoder__set_quirk_enabled(dec, quirks[i], true);
    }
  }
}

const char*  //
fuzz(wuffs_base__io_buffer* src, uint64_t hash) {
  wuffs_gif__decoder dec;
  wuffs_base__status status = wuffs_gif__decoder__initialize(
      &dec, sizeof dec, WUFFS_VERSION,
      (hash & 1) ? WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED : 0);
  hash = wuffs_base__u64__rotate_right(hash, 1);
  uint64_t hash_8_bits = hash & 0xFF;
  hash = wuffs_base__u64__rotate_right(hash, 8);
  if (!wuffs_base__status__is_ok(&status)) {
    return wuffs_base__status__message(&status);
  }
  set_quirks(&dec, hash);
  return fuzz_image_decoder(
      src, hash_8_bits,
      wuffs_gif__decoder__upcast_as__wuffs_base__image_decoder(&dec));
}
