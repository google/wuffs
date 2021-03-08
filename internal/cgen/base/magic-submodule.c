// After editing this file, run "go generate" in the ../data directory.

// Copyright 2021 The Wuffs Authors.
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

// ---------------- Magic Numbers

WUFFS_BASE__MAYBE_STATIC int32_t  //
wuffs_base__magic_number_guess_fourcc(wuffs_base__slice_u8 prefix) {
  // table holds the 'magic numbers' (which are actually variable length
  // strings). The strings may contain NUL bytes, so the "const char* magic"
  // value starts with the length-minus-1 of the 'magic number'.
  //
  // Keep it sorted by magic[1], then magic[0] descending and finally by
  // magic[2:]. When multiple entries match, the longest one wins.
  static struct {
    int32_t fourcc;
    const char* magic;
  } table[] = {
      {0x57424D50, "\x01\x00\x00"},          // WBMP
      {0x424D5020, "\x01\x42\x4D"},          // BMP
      {0x47494620, "\x03\x47\x49\x46\x38"},  // GIF
      {0x54494646, "\x03\x49\x49\x2A\x00"},  // TIFF (little-endian)
      {0x54494646, "\x03\x4D\x4D\x00\x2A"},  // TIFF (big-endian)
      {0x52494646, "\x03\x52\x49\x46\x46"},  // RIFF (see § below)
      {0x4E494520, "\x02\x6E\xC3\xAF"},      // NIE
      {0x504E4720, "\x03\x89\x50\x4E\x47"},  // PNG
      {0x4A504547, "\x01\xFF\xD8"},          // JPEG
  };
  static const size_t table_len = sizeof(table) / sizeof(table[0]);

  if (prefix.len == 0) {
    return -1;
  }
  uint8_t pre_first_byte = prefix.ptr[0];

  int32_t fourcc = 0;
  size_t i;
  for (i = 0; i < table_len; i++) {
    uint8_t mag_first_byte = ((uint8_t)(table[i].magic[1]));
    if (pre_first_byte < mag_first_byte) {
      break;
    } else if (pre_first_byte > mag_first_byte) {
      continue;
    }
    fourcc = table[i].fourcc;

    uint8_t mag_remaining_len = ((uint8_t)(table[i].magic[0]));
    if (mag_remaining_len == 0) {
      goto match;
    }

    const char* mag_remaining_ptr = table[i].magic + 2;
    uint8_t* pre_remaining_ptr = prefix.ptr + 1;
    size_t pre_remaining_len = prefix.len - 1;
    if (pre_remaining_len < mag_remaining_len) {
      if (!memcmp(pre_remaining_ptr, mag_remaining_ptr, pre_remaining_len)) {
        return -1;
      }
    } else {
      if (!memcmp(pre_remaining_ptr, mag_remaining_ptr, mag_remaining_len)) {
        goto match;
      }
    }
  }
  return 0;

match:
  // Some FourCC values (see § above) are further specialized.
  if (fourcc == 0x52494646) {  // 'RIFF'be
    if (prefix.len < 16) {
      return -1;
    }
    uint32_t x = wuffs_base__peek_u32be__no_bounds_check(prefix.ptr + 8);
    if (x == 0x57454250) {  // 'WEBP'be
      uint32_t y = wuffs_base__peek_u32be__no_bounds_check(prefix.ptr + 12);
      if (y == 0x56503820) {         // 'VP8 'be
        return 0x57503820;           // 'WP8 'be
      } else if (y == 0x5650384C) {  // 'VP8L'be
        return 0x5750384C;           // 'WP8L'be
      }
    }
  }
  return fourcc;
}
