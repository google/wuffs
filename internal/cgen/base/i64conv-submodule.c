// After editing this file, run "go generate" in the parent directory.

// Copyright 2020 The Wuffs Authors.
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

// ---------------- Integer

// wuffs_base__parse_number__foo_digits entries are 0x00 for invalid digits,
// and (0x80 | v) for valid digits, where v is the 4 bit value.

static const uint8_t wuffs_base__parse_number__decimal_digits[256] = {
    // 0     1     2     3     4     5     6     7
    // 8     9     A     B     C     D     E     F
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x00 ..= 0x07.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x08 ..= 0x0F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x10 ..= 0x17.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x18 ..= 0x1F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x20 ..= 0x27.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x28 ..= 0x2F.
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,  // 0x30 ..= 0x37. '0'-'7'.
    0x88, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x38 ..= 0x3F. '8'-'9'.

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x40 ..= 0x47.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x48 ..= 0x4F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x50 ..= 0x57.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x58 ..= 0x5F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x60 ..= 0x67.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x68 ..= 0x6F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x70 ..= 0x77.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x78 ..= 0x7F.

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x80 ..= 0x87.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x88 ..= 0x8F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x90 ..= 0x97.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x98 ..= 0x9F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xA0 ..= 0xA7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xA8 ..= 0xAF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xB0 ..= 0xB7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xB8 ..= 0xBF.

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xC0 ..= 0xC7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xC8 ..= 0xCF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xD0 ..= 0xD7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xD8 ..= 0xDF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xE0 ..= 0xE7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xE8 ..= 0xEF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xF0 ..= 0xF7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xF8 ..= 0xFF.
    // 0     1     2     3     4     5     6     7
    // 8     9     A     B     C     D     E     F
};

static const uint8_t wuffs_base__parse_number__hexadecimal_digits[256] = {
    // 0     1     2     3     4     5     6     7
    // 8     9     A     B     C     D     E     F
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x00 ..= 0x07.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x08 ..= 0x0F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x10 ..= 0x17.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x18 ..= 0x1F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x20 ..= 0x27.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x28 ..= 0x2F.
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,  // 0x30 ..= 0x37. '0'-'7'.
    0x88, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x38 ..= 0x3F. '8'-'9'.

    0x00, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x00,  // 0x40 ..= 0x47. 'A'-'F'.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x48 ..= 0x4F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x50 ..= 0x57.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x58 ..= 0x5F.
    0x00, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x00,  // 0x60 ..= 0x67. 'a'-'f'.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x68 ..= 0x6F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x70 ..= 0x77.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x78 ..= 0x7F.

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x80 ..= 0x87.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x88 ..= 0x8F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x90 ..= 0x97.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x98 ..= 0x9F.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xA0 ..= 0xA7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xA8 ..= 0xAF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xB0 ..= 0xB7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xB8 ..= 0xBF.

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xC0 ..= 0xC7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xC8 ..= 0xCF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xD0 ..= 0xD7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xD8 ..= 0xDF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xE0 ..= 0xE7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xE8 ..= 0xEF.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xF0 ..= 0xF7.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0xF8 ..= 0xFF.
    // 0     1     2     3     4     5     6     7
    // 8     9     A     B     C     D     E     F
};

// --------

WUFFS_BASE__MAYBE_STATIC wuffs_base__result_i64  //
wuffs_base__parse_number_i64(wuffs_base__slice_u8 s) {
  uint8_t* p = s.ptr;
  uint8_t* q = s.ptr + s.len;

  for (; (p < q) && (*p == '_'); p++) {
  }

  bool negative = false;
  if (p >= q) {
    goto fail_bad_argument;
  } else if (*p == '-') {
    p++;
    negative = true;
  } else if (*p == '+') {
    p++;
  }

  do {
    wuffs_base__result_u64 r = wuffs_base__parse_number_u64(
        wuffs_base__make_slice_u8(p, (size_t)(q - p)));
    if (r.status.repr != NULL) {
      wuffs_base__result_i64 ret;
      ret.status.repr = r.status.repr;
      ret.value = 0;
      return ret;
    } else if (negative) {
      if (r.value > 0x8000000000000000) {
        goto fail_out_of_bounds;
      }
      wuffs_base__result_i64 ret;
      ret.status.repr = NULL;
      ret.value = -(int64_t)(r.value);
      return ret;
    } else if (r.value > 0x7FFFFFFFFFFFFFFF) {
      goto fail_out_of_bounds;
    } else {
      wuffs_base__result_i64 ret;
      ret.status.repr = NULL;
      ret.value = +(int64_t)(r.value);
      return ret;
    }
  } while (0);

fail_bad_argument:
  do {
    wuffs_base__result_i64 ret;
    ret.status.repr = wuffs_base__error__bad_argument;
    ret.value = 0;
    return ret;
  } while (0);

fail_out_of_bounds:
  do {
    wuffs_base__result_i64 ret;
    ret.status.repr = wuffs_base__error__out_of_bounds;
    ret.value = 0;
    return ret;
  } while (0);
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__result_u64  //
wuffs_base__parse_number_u64(wuffs_base__slice_u8 s) {
  uint8_t* p = s.ptr;
  uint8_t* q = s.ptr + s.len;

  for (; (p < q) && (*p == '_'); p++) {
  }

  if (p >= q) {
    goto fail_bad_argument;

  } else if (*p == '0') {
    p++;
    if (p >= q) {
      goto ok_zero;
    }
    if (*p == '_') {
      p++;
      for (; p < q; p++) {
        if (*p != '_') {
          goto fail_bad_argument;
        }
      }
      goto ok_zero;
    }

    if ((*p == 'x') || (*p == 'X')) {
      p++;
      for (; (p < q) && (*p == '_'); p++) {
      }
      if (p < q) {
        goto hexadecimal;
      }

    } else if ((*p == 'd') || (*p == 'D')) {
      p++;
      for (; (p < q) && (*p == '_'); p++) {
      }
      if (p < q) {
        goto decimal;
      }
    }

    goto fail_bad_argument;
  }

decimal:
  do {
    uint64_t v = wuffs_base__parse_number__decimal_digits[*p++];
    if (v == 0) {
      goto fail_bad_argument;
    }
    v &= 0x0F;

    // UINT64_MAX is 18446744073709551615, which is ((10 * max10) + max1).
    const uint64_t max10 = 1844674407370955161u;
    const uint8_t max1 = 5;

    for (; p < q; p++) {
      if (*p == '_') {
        continue;
      }
      uint8_t digit = wuffs_base__parse_number__decimal_digits[*p];
      if (digit == 0) {
        goto fail_bad_argument;
      }
      digit &= 0x0F;
      if ((v > max10) || ((v == max10) && (digit > max1))) {
        goto fail_out_of_bounds;
      }
      v = (10 * v) + ((uint64_t)(digit));
    }

    wuffs_base__result_u64 ret;
    ret.status.repr = NULL;
    ret.value = v;
    return ret;
  } while (0);

hexadecimal:
  do {
    uint64_t v = wuffs_base__parse_number__hexadecimal_digits[*p++];
    if (v == 0) {
      goto fail_bad_argument;
    }
    v &= 0x0F;

    for (; p < q; p++) {
      if (*p == '_') {
        continue;
      }
      uint8_t digit = wuffs_base__parse_number__hexadecimal_digits[*p];
      if (digit == 0) {
        goto fail_bad_argument;
      }
      digit &= 0x0F;
      if ((v >> 60) != 0) {
        goto fail_out_of_bounds;
      }
      v = (v << 4) | ((uint64_t)(digit));
    }

    wuffs_base__result_u64 ret;
    ret.status.repr = NULL;
    ret.value = v;
    return ret;
  } while (0);

ok_zero:
  do {
    wuffs_base__result_u64 ret;
    ret.status.repr = NULL;
    ret.value = 0;
    return ret;
  } while (0);

fail_bad_argument:
  do {
    wuffs_base__result_u64 ret;
    ret.status.repr = wuffs_base__error__bad_argument;
    ret.value = 0;
    return ret;
  } while (0);

fail_out_of_bounds:
  do {
    wuffs_base__result_u64 ret;
    ret.status.repr = wuffs_base__error__out_of_bounds;
    ret.value = 0;
    return ret;
  } while (0);
}

// --------

// wuffs_base__render_number__first_hundred contains the decimal encodings of
// the first one hundred numbers [0 ..= 99].
static const uint8_t wuffs_base__render_number__first_hundred[200] = {
    '0', '0', '0', '1', '0', '2', '0', '3', '0', '4',  //
    '0', '5', '0', '6', '0', '7', '0', '8', '0', '9',  //
    '1', '0', '1', '1', '1', '2', '1', '3', '1', '4',  //
    '1', '5', '1', '6', '1', '7', '1', '8', '1', '9',  //
    '2', '0', '2', '1', '2', '2', '2', '3', '2', '4',  //
    '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',  //
    '3', '0', '3', '1', '3', '2', '3', '3', '3', '4',  //
    '3', '5', '3', '6', '3', '7', '3', '8', '3', '9',  //
    '4', '0', '4', '1', '4', '2', '4', '3', '4', '4',  //
    '4', '5', '4', '6', '4', '7', '4', '8', '4', '9',  //
    '5', '0', '5', '1', '5', '2', '5', '3', '5', '4',  //
    '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',  //
    '6', '0', '6', '1', '6', '2', '6', '3', '6', '4',  //
    '6', '5', '6', '6', '6', '7', '6', '8', '6', '9',  //
    '7', '0', '7', '1', '7', '2', '7', '3', '7', '4',  //
    '7', '5', '7', '6', '7', '7', '7', '8', '7', '9',  //
    '8', '0', '8', '1', '8', '2', '8', '3', '8', '4',  //
    '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',  //
    '9', '0', '9', '1', '9', '2', '9', '3', '9', '4',  //
    '9', '5', '9', '6', '9', '7', '9', '8', '9', '9',  //
};

static size_t  //
wuffs_base__private_implementation__render_number_u64(wuffs_base__slice_u8 dst,
                                                      uint64_t x,
                                                      uint32_t options,
                                                      bool neg) {
  uint8_t buf[WUFFS_BASE__U64__BYTE_LENGTH__MAX_INCL];
  uint8_t* ptr = &buf[0] + sizeof(buf);

  while (x >= 100) {
    size_t index = (x % 100) * 2;
    x /= 100;
    uint8_t s0 = wuffs_base__render_number__first_hundred[index + 0];
    uint8_t s1 = wuffs_base__render_number__first_hundred[index + 1];
    ptr -= 2;
    ptr[0] = s0;
    ptr[1] = s1;
  }

  if (x < 10) {
    ptr -= 1;
    ptr[0] = (uint8_t)('0' + x);
  } else {
    size_t index = x * 2;
    uint8_t s0 = wuffs_base__render_number__first_hundred[index + 0];
    uint8_t s1 = wuffs_base__render_number__first_hundred[index + 1];
    ptr -= 2;
    ptr[0] = s0;
    ptr[1] = s1;
  }

  if (neg) {
    ptr -= 1;
    ptr[0] = '-';
  } else if (options & WUFFS_BASE__RENDER_NUMBER_XXX__LEADING_PLUS_SIGN) {
    ptr -= 1;
    ptr[0] = '+';
  }

  size_t n = sizeof(buf) - ((size_t)(ptr - &buf[0]));
  if (n > dst.len) {
    return 0;
  }
  memcpy(dst.ptr + ((options & WUFFS_BASE__RENDER_NUMBER_XXX__ALIGN_RIGHT)
                        ? (dst.len - n)
                        : 0),
         ptr, n);
  return n;
}

WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__render_number_i64(wuffs_base__slice_u8 dst,
                              int64_t x,
                              uint32_t options) {
  uint64_t u = (uint64_t)x;
  bool neg = x < 0;
  if (neg) {
    u = 1 + ~u;
  }
  return wuffs_base__private_implementation__render_number_u64(dst, u, options,
                                                               neg);
}

WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__render_number_u64(wuffs_base__slice_u8 dst,
                              uint64_t x,
                              uint32_t options) {
  return wuffs_base__private_implementation__render_number_u64(dst, x, options,
                                                               false);
}

// ---------------- Hexadecimal

WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__hexadecimal__decode2(wuffs_base__slice_u8 dst,
                                 wuffs_base__slice_u8 src) {
  size_t src_len2 = src.len / 2;
  size_t len = dst.len < src_len2 ? dst.len : src_len2;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  while (n--) {
    *d = (uint8_t)((wuffs_base__parse_number__hexadecimal_digits[s[0]] << 4) |
                   (wuffs_base__parse_number__hexadecimal_digits[s[1]] & 0x0F));
    d += 1;
    s += 2;
  }

  return len;
}

WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__hexadecimal__decode4(wuffs_base__slice_u8 dst,
                                 wuffs_base__slice_u8 src) {
  size_t src_len4 = src.len / 4;
  size_t len = dst.len < src_len4 ? dst.len : src_len4;
  uint8_t* d = dst.ptr;
  uint8_t* s = src.ptr;
  size_t n = len;

  while (n--) {
    *d = (uint8_t)((wuffs_base__parse_number__hexadecimal_digits[s[2]] << 4) |
                   (wuffs_base__parse_number__hexadecimal_digits[s[3]] & 0x0F));
    d += 1;
    s += 4;
  }

  return len;
}
