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

// ---------------- IEEE 754 Floating Point

#define WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE 2047
#define WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION 800

// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL is the largest N
// such that ((10 << N) < (1 << 64)).
#define WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL 60

// wuffs_base__private_implementation__high_prec_dec (abbreviated as HPD) is a
// fixed precision floating point decimal number, augmented with ±infinity
// values, but it cannot represent NaN (Not a Number).
//
// "High precision" means that the mantissa holds 800 decimal digits. 800 is
// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION.
//
// An HPD isn't for general purpose arithmetic, only for conversions to and
// from IEEE 754 double-precision floating point, where the largest and
// smallest positive, finite values are approximately 1.8e+308 and 4.9e-324.
// HPD exponents above +2047 mean infinity, below -2047 mean zero. The ±2047
// bounds are further away from zero than ±(324 + 800), where 800 and 2047 is
// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION and
// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE.
//
// digits[.. num_digits] are the number's digits in big-endian order. The
// uint8_t values are in the range [0 ..= 9], not ['0' ..= '9'], where e.g. '7'
// is the ASCII value 0x37.
//
// decimal_point is the index (within digits) of the decimal point. It may be
// negative or be larger than num_digits, in which case the explicit digits are
// padded with implicit zeroes.
//
// For example, if num_digits is 3 and digits is "\x07\x08\x09":
//   - A decimal_point of -2 means ".00789"
//   - A decimal_point of -1 means ".0789"
//   - A decimal_point of +0 means ".789"
//   - A decimal_point of +1 means "7.89"
//   - A decimal_point of +2 means "78.9"
//   - A decimal_point of +3 means "789."
//   - A decimal_point of +4 means "7890."
//   - A decimal_point of +5 means "78900."
//
// As above, a decimal_point higher than +2047 means that the overall value is
// infinity, lower than -2047 means zero.
//
// negative is a sign bit. An HPD can distinguish positive and negative zero.
//
// truncated is whether there are more than
// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION digits, and at
// least one of those extra digits are non-zero. The existence of long-tail
// digits can affect rounding.
//
// The "all fields are zero" value is valid, and represents the number +0.
typedef struct {
  uint32_t num_digits;
  int32_t decimal_point;
  bool negative;
  bool truncated;
  uint8_t digits[WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION];
} wuffs_base__private_implementation__high_prec_dec;

// wuffs_base__private_implementation__high_prec_dec__trim trims trailing
// zeroes from the h->digits[.. h->num_digits] slice. They have no benefit,
// since we explicitly track h->decimal_point.
//
// Preconditions:
//  - h is non-NULL.
static inline void  //
wuffs_base__private_implementation__high_prec_dec__trim(
    wuffs_base__private_implementation__high_prec_dec* h) {
  while ((h->num_digits > 0) && (h->digits[h->num_digits - 1] == 0)) {
    h->num_digits--;
  }
}

// wuffs_base__private_implementation__high_prec_dec__assign sets h to
// represent the number x.
//
// Preconditions:
//  - h is non-NULL.
static void  //
wuffs_base__private_implementation__high_prec_dec__assign(
    wuffs_base__private_implementation__high_prec_dec* h,
    uint64_t x,
    bool negative) {
  uint32_t n = 0;

  // Set h->digits.
  if (x > 0) {
    // Calculate the digits, working right-to-left. After we determine n (how
    // many digits there are), copy from buf to h->digits.
    //
    // UINT64_MAX, 18446744073709551615, is 20 digits long. It can be faster to
    // copy a constant number of bytes than a variable number (20 instead of
    // n). Make buf large enough (and start writing to it from the middle) so
    // that can we always copy 20 bytes: the slice buf[(20-n) .. (40-n)].
    uint8_t buf[40] = {0};
    uint8_t* ptr = &buf[20];
    do {
      uint64_t remaining = x / 10;
      x -= remaining * 10;
      ptr--;
      *ptr = (uint8_t)x;
      n++;
      x = remaining;
    } while (x > 0);
    memcpy(h->digits, ptr, 20);
  }

  // Set h's other fields.
  h->num_digits = n;
  h->decimal_point = (int32_t)n;
  h->negative = negative;
  h->truncated = false;
  wuffs_base__private_implementation__high_prec_dec__trim(h);
}

static wuffs_base__status  //
wuffs_base__private_implementation__high_prec_dec__parse(
    wuffs_base__private_implementation__high_prec_dec* h,
    wuffs_base__slice_u8 s) {
  if (!h) {
    return wuffs_base__make_status(wuffs_base__error__bad_receiver);
  }
  h->num_digits = 0;
  h->decimal_point = 0;
  h->negative = false;
  h->truncated = false;

  uint8_t* p = s.ptr;
  uint8_t* q = s.ptr + s.len;

  for (; (p < q) && (*p == '_'); p++) {
  }
  if (p >= q) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }

  // Parse sign.
  do {
    if (*p == '+') {
      p++;
    } else if (*p == '-') {
      h->negative = true;
      p++;
    } else {
      break;
    }
    for (; (p < q) && (*p == '_'); p++) {
    }
  } while (0);

  // Parse digits.
  uint32_t nd = 0;
  int32_t dp = 0;
  bool saw_digits = false;
  bool saw_non_zero_digits = false;
  bool saw_dot = false;
  for (; p < q; p++) {
    if (*p == '_') {
      // No-op.

    } else if ((*p == '.') || (*p == ',')) {
      // As per https://en.wikipedia.org/wiki/Decimal_separator, both '.' or
      // ',' are commonly used. We just parse either, regardless of LOCALE.
      if (saw_dot) {
        return wuffs_base__make_status(wuffs_base__error__bad_argument);
      }
      saw_dot = true;
      dp = (int32_t)nd;

    } else if ('0' == *p) {
      if (!saw_dot && !saw_non_zero_digits && saw_digits) {
        // We don't allow unnecessary leading zeroes: "000123" or "0644".
        return wuffs_base__make_status(wuffs_base__error__bad_argument);
      }
      saw_digits = true;
      if (nd == 0) {
        // Track leading zeroes implicitly.
        dp--;
      } else if (nd <
                 WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
        h->digits[nd++] = 0;
      } else {
        // Long-tail zeroes are ignored.
      }

    } else if (('0' < *p) && (*p <= '9')) {
      if (!saw_dot && !saw_non_zero_digits && saw_digits) {
        // We don't allow unnecessary leading zeroes: "000123" or "0644".
        return wuffs_base__make_status(wuffs_base__error__bad_argument);
      }
      saw_digits = true;
      saw_non_zero_digits = true;
      if (nd < WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
        h->digits[nd++] = (uint8_t)(*p - '0');
      } else {
        // Long-tail non-zeroes set the truncated bit.
        h->truncated = true;
      }

    } else {
      break;
    }
  }

  if (!saw_digits) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  if (!saw_dot) {
    dp = (int32_t)nd;
  }

  // Parse exponent.
  if ((p < q) && ((*p == 'E') || (*p == 'e'))) {
    p++;
    for (; (p < q) && (*p == '_'); p++) {
    }
    if (p >= q) {
      return wuffs_base__make_status(wuffs_base__error__bad_argument);
    }

    int32_t exp_sign = +1;
    if (*p == '+') {
      p++;
    } else if (*p == '-') {
      exp_sign = -1;
      p++;
    }

    int32_t exp = 0;
    const int32_t exp_large =
        WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE +
        WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION;
    bool saw_exp_digits = false;
    for (; p < q; p++) {
      if (*p == '_') {
        // No-op.
      } else if (('0' <= *p) && (*p <= '9')) {
        saw_exp_digits = true;
        if (exp < exp_large) {
          exp = (10 * exp) + ((int32_t)(*p - '0'));
        }
      } else {
        break;
      }
    }
    if (!saw_exp_digits) {
      return wuffs_base__make_status(wuffs_base__error__bad_argument);
    }
    dp += exp_sign * exp;
  }

  // Finish.
  if (p != q) {
    return wuffs_base__make_status(wuffs_base__error__bad_argument);
  }
  h->num_digits = nd;
  if (nd == 0) {
    h->decimal_point = 0;
  } else if (dp <
             -WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
    h->decimal_point =
        -WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE - 1;
  } else if (dp >
             +WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
    h->decimal_point =
        +WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE + 1;
  } else {
    h->decimal_point = dp;
  }
  wuffs_base__private_implementation__high_prec_dec__trim(h);
  return wuffs_base__make_status(NULL);
}

// --------

// The etc__hpd_left_shift and etc__powers_of_5 tables were printed by
// script/print-hpd-left-shift.go. That script has an optional -comments flag,
// whose output is not copied here, which prints further detail.
//
// These tables are used in
// wuffs_base__private_implementation__high_prec_dec__lshift_num_new_digits.

// wuffs_base__private_implementation__hpd_left_shift[i] encodes the number of
// new digits created after multiplying a positive integer by (1 << i): the
// additional length in the decimal representation. For example, shifting "234"
// by 3 (equivalent to multiplying by 8) will produce "1872". Going from a
// 3-length string to a 4-length string means that 1 new digit was added (and
// existing digits may have changed).
//
// Shifting by i can add either N or N-1 new digits, depending on whether the
// original positive integer compares >= or < to the i'th power of 5 (as 10
// equals 2 * 5). Comparison is lexicographic, not numerical.
//
// For example, shifting by 4 (i.e. multiplying by 16) can add 1 or 2 new
// digits, depending on a lexicographic comparison to (5 ** 4), i.e. "625":
//  - ("1"      << 4) is "16",       which adds 1 new digit.
//  - ("5678"   << 4) is "90848",    which adds 1 new digit.
//  - ("624"    << 4) is "9984",     which adds 1 new digit.
//  - ("62498"  << 4) is "999968",   which adds 1 new digit.
//  - ("625"    << 4) is "10000",    which adds 2 new digits.
//  - ("625001" << 4) is "10000016", which adds 2 new digits.
//  - ("7008"   << 4) is "112128",   which adds 2 new digits.
//  - ("99"     << 4) is "1584",     which adds 2 new digits.
//
// Thus, when i is 4, N is 2 and (5 ** i) is "625". This etc__hpd_left_shift
// array encodes this as:
//  - etc__hpd_left_shift[4] is 0x1006 = (2 << 11) | 0x0006.
//  - etc__hpd_left_shift[5] is 0x1009 = (? << 11) | 0x0009.
// where the ? isn't relevant for i == 4.
//
// The high 5 bits of etc__hpd_left_shift[i] is N, the higher of the two
// possible number of new digits. The low 11 bits are an offset into the
// etc__powers_of_5 array (of length 0x051C, so offsets fit in 11 bits). When i
// is 4, its offset and the next one is 6 and 9, and etc__powers_of_5[6 .. 9]
// is the string "\x06\x02\x05", so the relevant power of 5 is "625".
//
// Thanks to Ken Thompson for the original idea.
static const uint16_t wuffs_base__private_implementation__hpd_left_shift[65] = {
    0x0000, 0x0800, 0x0801, 0x0803, 0x1006, 0x1009, 0x100D, 0x1812, 0x1817,
    0x181D, 0x2024, 0x202B, 0x2033, 0x203C, 0x2846, 0x2850, 0x285B, 0x3067,
    0x3073, 0x3080, 0x388E, 0x389C, 0x38AB, 0x38BB, 0x40CC, 0x40DD, 0x40EF,
    0x4902, 0x4915, 0x4929, 0x513E, 0x5153, 0x5169, 0x5180, 0x5998, 0x59B0,
    0x59C9, 0x61E3, 0x61FD, 0x6218, 0x6A34, 0x6A50, 0x6A6D, 0x6A8B, 0x72AA,
    0x72C9, 0x72E9, 0x7B0A, 0x7B2B, 0x7B4D, 0x8370, 0x8393, 0x83B7, 0x83DC,
    0x8C02, 0x8C28, 0x8C4F, 0x9477, 0x949F, 0x94C8, 0x9CF2, 0x051C, 0x051C,
    0x051C, 0x051C,
};

// wuffs_base__private_implementation__powers_of_5 contains the powers of 5,
// concatenated together: "5", "25", "125", "625", "3125", etc.
static const uint8_t wuffs_base__private_implementation__powers_of_5[0x051C] = {
    5, 2, 5, 1, 2, 5, 6, 2, 5, 3, 1, 2, 5, 1, 5, 6, 2, 5, 7, 8, 1, 2, 5, 3, 9,
    0, 6, 2, 5, 1, 9, 5, 3, 1, 2, 5, 9, 7, 6, 5, 6, 2, 5, 4, 8, 8, 2, 8, 1, 2,
    5, 2, 4, 4, 1, 4, 0, 6, 2, 5, 1, 2, 2, 0, 7, 0, 3, 1, 2, 5, 6, 1, 0, 3, 5,
    1, 5, 6, 2, 5, 3, 0, 5, 1, 7, 5, 7, 8, 1, 2, 5, 1, 5, 2, 5, 8, 7, 8, 9, 0,
    6, 2, 5, 7, 6, 2, 9, 3, 9, 4, 5, 3, 1, 2, 5, 3, 8, 1, 4, 6, 9, 7, 2, 6, 5,
    6, 2, 5, 1, 9, 0, 7, 3, 4, 8, 6, 3, 2, 8, 1, 2, 5, 9, 5, 3, 6, 7, 4, 3, 1,
    6, 4, 0, 6, 2, 5, 4, 7, 6, 8, 3, 7, 1, 5, 8, 2, 0, 3, 1, 2, 5, 2, 3, 8, 4,
    1, 8, 5, 7, 9, 1, 0, 1, 5, 6, 2, 5, 1, 1, 9, 2, 0, 9, 2, 8, 9, 5, 5, 0, 7,
    8, 1, 2, 5, 5, 9, 6, 0, 4, 6, 4, 4, 7, 7, 5, 3, 9, 0, 6, 2, 5, 2, 9, 8, 0,
    2, 3, 2, 2, 3, 8, 7, 6, 9, 5, 3, 1, 2, 5, 1, 4, 9, 0, 1, 1, 6, 1, 1, 9, 3,
    8, 4, 7, 6, 5, 6, 2, 5, 7, 4, 5, 0, 5, 8, 0, 5, 9, 6, 9, 2, 3, 8, 2, 8, 1,
    2, 5, 3, 7, 2, 5, 2, 9, 0, 2, 9, 8, 4, 6, 1, 9, 1, 4, 0, 6, 2, 5, 1, 8, 6,
    2, 6, 4, 5, 1, 4, 9, 2, 3, 0, 9, 5, 7, 0, 3, 1, 2, 5, 9, 3, 1, 3, 2, 2, 5,
    7, 4, 6, 1, 5, 4, 7, 8, 5, 1, 5, 6, 2, 5, 4, 6, 5, 6, 6, 1, 2, 8, 7, 3, 0,
    7, 7, 3, 9, 2, 5, 7, 8, 1, 2, 5, 2, 3, 2, 8, 3, 0, 6, 4, 3, 6, 5, 3, 8, 6,
    9, 6, 2, 8, 9, 0, 6, 2, 5, 1, 1, 6, 4, 1, 5, 3, 2, 1, 8, 2, 6, 9, 3, 4, 8,
    1, 4, 4, 5, 3, 1, 2, 5, 5, 8, 2, 0, 7, 6, 6, 0, 9, 1, 3, 4, 6, 7, 4, 0, 7,
    2, 2, 6, 5, 6, 2, 5, 2, 9, 1, 0, 3, 8, 3, 0, 4, 5, 6, 7, 3, 3, 7, 0, 3, 6,
    1, 3, 2, 8, 1, 2, 5, 1, 4, 5, 5, 1, 9, 1, 5, 2, 2, 8, 3, 6, 6, 8, 5, 1, 8,
    0, 6, 6, 4, 0, 6, 2, 5, 7, 2, 7, 5, 9, 5, 7, 6, 1, 4, 1, 8, 3, 4, 2, 5, 9,
    0, 3, 3, 2, 0, 3, 1, 2, 5, 3, 6, 3, 7, 9, 7, 8, 8, 0, 7, 0, 9, 1, 7, 1, 2,
    9, 5, 1, 6, 6, 0, 1, 5, 6, 2, 5, 1, 8, 1, 8, 9, 8, 9, 4, 0, 3, 5, 4, 5, 8,
    5, 6, 4, 7, 5, 8, 3, 0, 0, 7, 8, 1, 2, 5, 9, 0, 9, 4, 9, 4, 7, 0, 1, 7, 7,
    2, 9, 2, 8, 2, 3, 7, 9, 1, 5, 0, 3, 9, 0, 6, 2, 5, 4, 5, 4, 7, 4, 7, 3, 5,
    0, 8, 8, 6, 4, 6, 4, 1, 1, 8, 9, 5, 7, 5, 1, 9, 5, 3, 1, 2, 5, 2, 2, 7, 3,
    7, 3, 6, 7, 5, 4, 4, 3, 2, 3, 2, 0, 5, 9, 4, 7, 8, 7, 5, 9, 7, 6, 5, 6, 2,
    5, 1, 1, 3, 6, 8, 6, 8, 3, 7, 7, 2, 1, 6, 1, 6, 0, 2, 9, 7, 3, 9, 3, 7, 9,
    8, 8, 2, 8, 1, 2, 5, 5, 6, 8, 4, 3, 4, 1, 8, 8, 6, 0, 8, 0, 8, 0, 1, 4, 8,
    6, 9, 6, 8, 9, 9, 4, 1, 4, 0, 6, 2, 5, 2, 8, 4, 2, 1, 7, 0, 9, 4, 3, 0, 4,
    0, 4, 0, 0, 7, 4, 3, 4, 8, 4, 4, 9, 7, 0, 7, 0, 3, 1, 2, 5, 1, 4, 2, 1, 0,
    8, 5, 4, 7, 1, 5, 2, 0, 2, 0, 0, 3, 7, 1, 7, 4, 2, 2, 4, 8, 5, 3, 5, 1, 5,
    6, 2, 5, 7, 1, 0, 5, 4, 2, 7, 3, 5, 7, 6, 0, 1, 0, 0, 1, 8, 5, 8, 7, 1, 1,
    2, 4, 2, 6, 7, 5, 7, 8, 1, 2, 5, 3, 5, 5, 2, 7, 1, 3, 6, 7, 8, 8, 0, 0, 5,
    0, 0, 9, 2, 9, 3, 5, 5, 6, 2, 1, 3, 3, 7, 8, 9, 0, 6, 2, 5, 1, 7, 7, 6, 3,
    5, 6, 8, 3, 9, 4, 0, 0, 2, 5, 0, 4, 6, 4, 6, 7, 7, 8, 1, 0, 6, 6, 8, 9, 4,
    5, 3, 1, 2, 5, 8, 8, 8, 1, 7, 8, 4, 1, 9, 7, 0, 0, 1, 2, 5, 2, 3, 2, 3, 3,
    8, 9, 0, 5, 3, 3, 4, 4, 7, 2, 6, 5, 6, 2, 5, 4, 4, 4, 0, 8, 9, 2, 0, 9, 8,
    5, 0, 0, 6, 2, 6, 1, 6, 1, 6, 9, 4, 5, 2, 6, 6, 7, 2, 3, 6, 3, 2, 8, 1, 2,
    5, 2, 2, 2, 0, 4, 4, 6, 0, 4, 9, 2, 5, 0, 3, 1, 3, 0, 8, 0, 8, 4, 7, 2, 6,
    3, 3, 3, 6, 1, 8, 1, 6, 4, 0, 6, 2, 5, 1, 1, 1, 0, 2, 2, 3, 0, 2, 4, 6, 2,
    5, 1, 5, 6, 5, 4, 0, 4, 2, 3, 6, 3, 1, 6, 6, 8, 0, 9, 0, 8, 2, 0, 3, 1, 2,
    5, 5, 5, 5, 1, 1, 1, 5, 1, 2, 3, 1, 2, 5, 7, 8, 2, 7, 0, 2, 1, 1, 8, 1, 5,
    8, 3, 4, 0, 4, 5, 4, 1, 0, 1, 5, 6, 2, 5, 2, 7, 7, 5, 5, 5, 7, 5, 6, 1, 5,
    6, 2, 8, 9, 1, 3, 5, 1, 0, 5, 9, 0, 7, 9, 1, 7, 0, 2, 2, 7, 0, 5, 0, 7, 8,
    1, 2, 5, 1, 3, 8, 7, 7, 7, 8, 7, 8, 0, 7, 8, 1, 4, 4, 5, 6, 7, 5, 5, 2, 9,
    5, 3, 9, 5, 8, 5, 1, 1, 3, 5, 2, 5, 3, 9, 0, 6, 2, 5, 6, 9, 3, 8, 8, 9, 3,
    9, 0, 3, 9, 0, 7, 2, 2, 8, 3, 7, 7, 6, 4, 7, 6, 9, 7, 9, 2, 5, 5, 6, 7, 6,
    2, 6, 9, 5, 3, 1, 2, 5, 3, 4, 6, 9, 4, 4, 6, 9, 5, 1, 9, 5, 3, 6, 1, 4, 1,
    8, 8, 8, 2, 3, 8, 4, 8, 9, 6, 2, 7, 8, 3, 8, 1, 3, 4, 7, 6, 5, 6, 2, 5, 1,
    7, 3, 4, 7, 2, 3, 4, 7, 5, 9, 7, 6, 8, 0, 7, 0, 9, 4, 4, 1, 1, 9, 2, 4, 4,
    8, 1, 3, 9, 1, 9, 0, 6, 7, 3, 8, 2, 8, 1, 2, 5, 8, 6, 7, 3, 6, 1, 7, 3, 7,
    9, 8, 8, 4, 0, 3, 5, 4, 7, 2, 0, 5, 9, 6, 2, 2, 4, 0, 6, 9, 5, 9, 5, 3, 3,
    6, 9, 1, 4, 0, 6, 2, 5,
};

// wuffs_base__private_implementation__high_prec_dec__lshift_num_new_digits
// returns the number of additional decimal digits when left-shifting by shift.
//
// See below for preconditions.
static uint32_t  //
wuffs_base__private_implementation__high_prec_dec__lshift_num_new_digits(
    wuffs_base__private_implementation__high_prec_dec* h,
    uint32_t shift) {
  // Masking with 0x3F should be unnecessary (assuming the preconditions) but
  // it's cheap and ensures that we don't overflow the
  // wuffs_base__private_implementation__hpd_left_shift array.
  shift &= 63;

  uint32_t x_a = wuffs_base__private_implementation__hpd_left_shift[shift];
  uint32_t x_b = wuffs_base__private_implementation__hpd_left_shift[shift + 1];
  uint32_t num_new_digits = x_a >> 11;
  uint32_t pow5_a = 0x7FF & x_a;
  uint32_t pow5_b = 0x7FF & x_b;

  const uint8_t* pow5 =
      &wuffs_base__private_implementation__powers_of_5[pow5_a];
  uint32_t i = 0;
  uint32_t n = pow5_b - pow5_a;
  for (; i < n; i++) {
    if (i >= h->num_digits) {
      return num_new_digits - 1;
    } else if (h->digits[i] == pow5[i]) {
      continue;
    } else if (h->digits[i] < pow5[i]) {
      return num_new_digits - 1;
    } else {
      return num_new_digits;
    }
  }
  return num_new_digits;
}

// --------

// wuffs_base__private_implementation__high_prec_dec__rounded_integer returns
// the integral (non-fractional) part of h, provided that it is 18 or fewer
// decimal digits. For 19 or more digits, it returns UINT64_MAX. Note that:
//   - (1 << 53) is    9007199254740992, which has 16 decimal digits.
//   - (1 << 56) is   72057594037927936, which has 17 decimal digits.
//   - (1 << 59) is  576460752303423488, which has 18 decimal digits.
//   - (1 << 63) is 9223372036854775808, which has 19 decimal digits.
// and that IEEE 754 double precision has 52 mantissa bits.
//
// That integral part is rounded-to-even: rounding 7.5 or 8.5 both give 8.
//
// h's negative bit is ignored: rounding -8.6 returns 9.
//
// See below for preconditions.
static uint64_t  //
wuffs_base__private_implementation__high_prec_dec__rounded_integer(
    wuffs_base__private_implementation__high_prec_dec* h) {
  if ((h->num_digits == 0) || (h->decimal_point < 0)) {
    return 0;
  } else if (h->decimal_point > 18) {
    return UINT64_MAX;
  }

  uint32_t dp = (uint32_t)(h->decimal_point);
  uint64_t n = 0;
  uint32_t i = 0;
  for (; i < dp; i++) {
    n = (10 * n) + ((i < h->num_digits) ? h->digits[i] : 0);
  }

  bool round_up = false;
  if (dp < h->num_digits) {
    round_up = h->digits[dp] >= 5;
    if ((h->digits[dp] == 5) && (dp + 1 == h->num_digits)) {
      // We are exactly halfway. If we're truncated, round up, otherwise round
      // to even.
      round_up = h->truncated ||  //
                 ((dp > 0) && (1 & h->digits[dp - 1]));
    }
  }
  if (round_up) {
    n++;
  }

  return n;
}

// wuffs_base__private_implementation__high_prec_dec__small_xshift shifts h's
// number (where 'x' is 'l' or 'r' for left or right) by a small shift value.
//
// Preconditions:
//  - h is non-NULL.
//  - h->decimal_point is "not extreme".
//  - shift is non-zero.
//  - shift is "a small shift".
//
// "Not extreme" means within
// ±WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE.
//
// "A small shift" means not more than
// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL.
//
// wuffs_base__private_implementation__high_prec_dec__rounded_integer and
// wuffs_base__private_implementation__high_prec_dec__lshift_num_new_digits
// have the same preconditions.
//
// wuffs_base__private_implementation__high_prec_dec__lshift keeps the first
// two preconditions but not the last two. Its shift argument is signed and
// does not need to be "small": zero is a no-op, positive means left shift and
// negative means right shift.

static void  //
wuffs_base__private_implementation__high_prec_dec__small_lshift(
    wuffs_base__private_implementation__high_prec_dec* h,
    uint32_t shift) {
  if (h->num_digits == 0) {
    return;
  }
  uint32_t num_new_digits =
      wuffs_base__private_implementation__high_prec_dec__lshift_num_new_digits(
          h, shift);
  uint32_t rx = h->num_digits - 1;                   // Read  index.
  uint32_t wx = h->num_digits - 1 + num_new_digits;  // Write index.
  uint64_t n = 0;

  // Repeat: pick up a digit, put down a digit, right to left.
  while (((int32_t)rx) >= 0) {
    n += ((uint64_t)(h->digits[rx])) << shift;
    uint64_t quo = n / 10;
    uint64_t rem = n - (10 * quo);
    if (wx < WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
      h->digits[wx] = (uint8_t)rem;
    } else if (rem > 0) {
      h->truncated = true;
    }
    n = quo;
    wx--;
    rx--;
  }

  // Put down leading digits, right to left.
  while (n > 0) {
    uint64_t quo = n / 10;
    uint64_t rem = n - (10 * quo);
    if (wx < WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
      h->digits[wx] = (uint8_t)rem;
    } else if (rem > 0) {
      h->truncated = true;
    }
    n = quo;
    wx--;
  }

  // Finish.
  h->num_digits += num_new_digits;
  if (h->num_digits >
      WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
    h->num_digits = WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION;
  }
  h->decimal_point += (int32_t)num_new_digits;
  wuffs_base__private_implementation__high_prec_dec__trim(h);
}

static void  //
wuffs_base__private_implementation__high_prec_dec__small_rshift(
    wuffs_base__private_implementation__high_prec_dec* h,
    uint32_t shift) {
  uint32_t rx = 0;  // Read  index.
  uint32_t wx = 0;  // Write index.
  uint64_t n = 0;

  // Pick up enough leading digits to cover the first shift.
  while ((n >> shift) == 0) {
    if (rx < h->num_digits) {
      // Read a digit.
      n = (10 * n) + h->digits[rx++];
    } else if (n == 0) {
      // h's number used to be zero and remains zero.
      return;
    } else {
      // Read sufficient implicit trailing zeroes.
      while ((n >> shift) == 0) {
        n = 10 * n;
        rx++;
      }
      break;
    }
  }
  h->decimal_point -= ((int32_t)(rx - 1));
  if (h->decimal_point <
      -WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
    // After the shift, h's number is effectively zero.
    h->num_digits = 0;
    h->decimal_point = 0;
    h->negative = false;
    h->truncated = false;
    return;
  }

  // Repeat: pick up a digit, put down a digit, left to right.
  uint64_t mask = (((uint64_t)(1)) << shift) - 1;
  while (rx < h->num_digits) {
    uint8_t new_digit = ((uint8_t)(n >> shift));
    n = (10 * (n & mask)) + h->digits[rx++];
    h->digits[wx++] = new_digit;
  }

  // Put down trailing digits, left to right.
  while (n > 0) {
    uint8_t new_digit = ((uint8_t)(n >> shift));
    n = 10 * (n & mask);
    if (wx < WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION) {
      h->digits[wx++] = new_digit;
    } else if (new_digit > 0) {
      h->truncated = true;
    }
  }

  // Finish.
  h->num_digits = wx;
  wuffs_base__private_implementation__high_prec_dec__trim(h);
}

static void  //
wuffs_base__private_implementation__high_prec_dec__lshift(
    wuffs_base__private_implementation__high_prec_dec* h,
    int32_t shift) {
  if (shift > 0) {
    while (shift > +WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL) {
      wuffs_base__private_implementation__high_prec_dec__small_lshift(
          h, WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL);
      shift -= WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL;
    }
    wuffs_base__private_implementation__high_prec_dec__small_lshift(
        h, ((uint32_t)(+shift)));
  } else if (shift < 0) {
    while (shift < -WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL) {
      wuffs_base__private_implementation__high_prec_dec__small_rshift(
          h, WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL);
      shift += WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL;
    }
    wuffs_base__private_implementation__high_prec_dec__small_rshift(
        h, ((uint32_t)(-shift)));
  }
}

// --------

// wuffs_base__private_implementation__high_prec_dec__round_etc rounds h's
// number. For those functions that take an n argument, rounding produces at
// most n digits (which is not necessarily at most n decimal places). Negative
// n values are ignored, as well as any n greater than or equal to h's number
// of digits. The etc__round_just_enough function implicitly chooses an n to
// implement WUFFS_BASE__RENDER_NUMBER_FXX__JUST_ENOUGH_PRECISION.
//
// Preconditions:
//  - h is non-NULL.
//  - h->decimal_point is "not extreme".
//
// "Not extreme" means within
// ±WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE.

static void  //
wuffs_base__private_implementation__high_prec_dec__round_down(
    wuffs_base__private_implementation__high_prec_dec* h,
    int32_t n) {
  if ((n < 0) || (h->num_digits <= (uint32_t)n)) {
    return;
  }
  h->num_digits = (uint32_t)(n);
  wuffs_base__private_implementation__high_prec_dec__trim(h);
}

static void  //
wuffs_base__private_implementation__high_prec_dec__round_up(
    wuffs_base__private_implementation__high_prec_dec* h,
    int32_t n) {
  if ((n < 0) || (h->num_digits <= (uint32_t)n)) {
    return;
  }

  for (n--; n >= 0; n--) {
    if (h->digits[n] < 9) {
      h->digits[n]++;
      h->num_digits = (uint32_t)(n + 1);
      return;
    }
  }

  // The number is all 9s. Change to a single 1 and adjust the decimal point.
  h->digits[0] = 1;
  h->num_digits = 1;
  h->decimal_point++;
}

static void  //
wuffs_base__private_implementation__high_prec_dec__round_nearest(
    wuffs_base__private_implementation__high_prec_dec* h,
    int32_t n) {
  if ((n < 0) || (h->num_digits <= (uint32_t)n)) {
    return;
  }
  bool up = h->digits[n] >= 5;
  if ((h->digits[n] == 5) && ((n + 1) == ((int32_t)(h->num_digits)))) {
    up = h->truncated ||  //
         ((n > 0) && ((h->digits[n - 1] & 1) != 0));
  }

  if (up) {
    wuffs_base__private_implementation__high_prec_dec__round_up(h, n);
  } else {
    wuffs_base__private_implementation__high_prec_dec__round_down(h, n);
  }
}

static void  //
wuffs_base__private_implementation__high_prec_dec__round_just_enough(
    wuffs_base__private_implementation__high_prec_dec* h,
    int32_t exp2,
    uint64_t mantissa) {
  // The magic numbers 52 and 53 in this function are because IEEE 754 double
  // precision has 52 mantissa bits.
  //
  // Let f be the floating point number represented by exp2 and mantissa (and
  // also the number in h): the number (mantissa * (2 ** (exp2 - 52))).
  //
  // If f is zero or a small integer, we can return early.
  if ((mantissa == 0) ||
      ((exp2 < 53) && (h->decimal_point >= ((int32_t)(h->num_digits))))) {
    return;
  }

  // The smallest normal f has an exp2 of -1022 and a mantissa of (1 << 52).
  // Subnormal numbers have the same exp2 but a smaller mantissa.
  static const int32_t min_incl_normal_exp2 = -1022;
  static const uint64_t min_incl_normal_mantissa = 0x0010000000000000ul;

  // Compute lower and upper bounds such that any number between them (possibly
  // inclusive) will round to f. First, the lower bound. Our number f is:
  //   ((mantissa + 0)         * (2 ** (  exp2 - 52)))
  //
  // The next lowest floating point number is:
  //   ((mantissa - 1)         * (2 ** (  exp2 - 52)))
  // unless (mantissa - 1) drops the (1 << 52) bit and exp2 is not the
  // min_incl_normal_exp2. Either way, call it:
  //   ((l_mantissa)           * (2 ** (l_exp2 - 52)))
  //
  // The lower bound is halfway between them (noting that 52 became 53):
  //   (((2 * l_mantissa) + 1) * (2 ** (l_exp2 - 53)))
  int32_t l_exp2 = exp2;
  uint64_t l_mantissa = mantissa - 1;
  if ((exp2 > min_incl_normal_exp2) && (mantissa <= min_incl_normal_mantissa)) {
    l_exp2 = exp2 - 1;
    l_mantissa = (2 * mantissa) - 1;
  }
  wuffs_base__private_implementation__high_prec_dec lower;
  wuffs_base__private_implementation__high_prec_dec__assign(
      &lower, (2 * l_mantissa) + 1, false);
  wuffs_base__private_implementation__high_prec_dec__lshift(&lower,
                                                            l_exp2 - 53);

  // Next, the upper bound. Our number f is:
  //   ((mantissa + 0)       * (2 ** (exp2 - 52)))
  //
  // The next highest floating point number is:
  //   ((mantissa + 1)       * (2 ** (exp2 - 52)))
  //
  // The upper bound is halfway between them (noting that 52 became 53):
  //   (((2 * mantissa) + 1) * (2 ** (exp2 - 53)))
  wuffs_base__private_implementation__high_prec_dec upper;
  wuffs_base__private_implementation__high_prec_dec__assign(
      &upper, (2 * mantissa) + 1, false);
  wuffs_base__private_implementation__high_prec_dec__lshift(&upper, exp2 - 53);

  // The lower and upper bounds are possible outputs only if the original
  // mantissa is even, so that IEEE round-to-even would round to the original
  // mantissa and not its neighbors.
  bool inclusive = (mantissa & 1) == 0;

  // As we walk the digits, we want to know whether rounding up would fall
  // within the upper bound. This is tracked by upper_delta:
  //  - When -1, the digits of h and upper are the same so far.
  //  - When +0, we saw a difference of 1 between h and upper on a previous
  //    digit and subsequently only 9s for h and 0s for upper. Thus, rounding
  //    up may fall outside of the bound if !inclusive.
  //  - When +1, the difference is greater than 1 and we know that rounding up
  //    falls within the bound.
  //
  // This is a state machine with three states. The numerical value for each
  // state (-1, +0 or +1) isn't important, other than their order.
  int upper_delta = -1;

  // We can now figure out the shortest number of digits required. Walk the
  // digits until h has distinguished itself from lower or upper.
  //
  // The zi and zd variables are indexes and digits, for z in l (lower), h (the
  // number) and u (upper).
  //
  // The lower, h and upper numbers may have their decimal points at different
  // places. In this case, upper is the longest, so we iterate ui starting from
  // 0 and iterate li and hi starting from either 0 or -1.
  int32_t ui = 0;
  for (;; ui++) {
    // Calculate hd, the middle number's digit.
    int32_t hi = ui - upper.decimal_point + h->decimal_point;
    if (hi >= ((int32_t)(h->num_digits))) {
      break;
    }
    uint8_t hd = (((uint32_t)hi) < h->num_digits) ? h->digits[hi] : 0;

    // Calculate ld, the lower bound's digit.
    int32_t li = ui - upper.decimal_point + lower.decimal_point;
    uint8_t ld = (((uint32_t)li) < lower.num_digits) ? lower.digits[li] : 0;

    // We can round down (truncate) if lower has a different digit than h or if
    // lower is inclusive and is exactly the result of rounding down (i.e. we
    // have reached the final digit of lower).
    bool can_round_down =
        (ld != hd) ||  //
        (inclusive && ((li + 1) == ((int32_t)(lower.num_digits))));

    // Calculate ud, the upper bound's digit, and update upper_delta.
    uint8_t ud = (((uint32_t)ui) < upper.num_digits) ? upper.digits[ui] : 0;
    if (upper_delta < 0) {
      if ((hd + 1) < ud) {
        // For example:
        // h     = 12345???
        // upper = 12347???
        upper_delta = +1;
      } else if (hd != ud) {
        // For example:
        // h     = 12345???
        // upper = 12346???
        upper_delta = +0;
      }
    } else if (upper_delta == 0) {
      if ((hd != 9) || (ud != 0)) {
        // For example:
        // h     = 1234598?
        // upper = 1234600?
        upper_delta = +1;
      }
    }

    // We can round up if upper has a different digit than h and either upper
    // is inclusive or upper is bigger than the result of rounding up.
    bool can_round_up =
        (upper_delta > 0) ||    //
        ((upper_delta == 0) &&  //
         (inclusive || ((ui + 1) < ((int32_t)(upper.num_digits)))));

    // If we can round either way, round to nearest. If we can round only one
    // way, do it. If we can't round, continue the loop.
    if (can_round_down) {
      if (can_round_up) {
        wuffs_base__private_implementation__high_prec_dec__round_nearest(
            h, hi + 1);
        return;
      } else {
        wuffs_base__private_implementation__high_prec_dec__round_down(h,
                                                                      hi + 1);
        return;
      }
    } else {
      if (can_round_up) {
        wuffs_base__private_implementation__high_prec_dec__round_up(h, hi + 1);
        return;
      }
    }
  }
}

// --------

// The wuffs_base__private_implementation__etc_powers_of_10 tables were printed
// by script/print-mpb-powers-of-10.go. That script has an optional -detail
// flag, whose output is not copied here, which prints further detail.
//
// These tables are used in
// wuffs_base__private_implementation__medium_prec_bin__assign_from_hpd.

// wuffs_base__private_implementation__powers_of_10 contains approximations to
// the powers of 10, ranging from 1e-326 to 1e+310 inclusive, as 637 uint32_t
// triples (64-bit mantissa, 32-bit base-2 exponent), 637 * 3 = 1911.
//
// For example, the third approximation, for 1e-324, consists of the uint32_t
// triple (0x5DCE35EA, 0xCF42894A, 0xFFFFFB8C). The first two of that triple
// are a little-endian uint64_t value: 0xCF42894A5DCE35EA. The last one is an
// int32_t value: -1140. Together, they represent the approximation:
//   1e-324 ≈ 0xCF42894A5DCE35EA * (2 ** -1140)
// Similarly, the (0x00000000, 0x9C400000, 0xFFFFFFCE) uint32_t triple means:
//   1e+4   ≈ 0x9C40000000000000 * (2 **   -50)  // This approx'n is exact.
// Similarly, the (0xD4C4FB27, 0xED63A231, 0x000000A2) uint32_t triple means:
//   1e+68  ≈ 0xED63A231D4C4FB27 * (2 **   162)
static const uint32_t wuffs_base__private_implementation__powers_of_10[1911] = {
    0xFE98746D, 0x84A57695, 0xFFFFFB86,  // 1e-326
    0x7E3E9188, 0xA5CED43B, 0xFFFFFB89,  // 1e-325
    0x5DCE35EA, 0xCF42894A, 0xFFFFFB8C,  // 1e-324
    0x7AA0E1B2, 0x818995CE, 0xFFFFFB90,  // 1e-323
    0x19491A1F, 0xA1EBFB42, 0xFFFFFB93,  // 1e-322
    0x9F9B60A7, 0xCA66FA12, 0xFFFFFB96,  // 1e-321
    0x478238D1, 0xFD00B897, 0xFFFFFB99,  // 1e-320
    0x8CB16382, 0x9E20735E, 0xFFFFFB9D,  // 1e-319
    0x2FDDBC63, 0xC5A89036, 0xFFFFFBA0,  // 1e-318
    0xBBD52B7C, 0xF712B443, 0xFFFFFBA3,  // 1e-317
    0x55653B2D, 0x9A6BB0AA, 0xFFFFFBA7,  // 1e-316
    0xEABE89F9, 0xC1069CD4, 0xFFFFFBAA,  // 1e-315
    0x256E2C77, 0xF148440A, 0xFFFFFBAD,  // 1e-314
    0x5764DBCA, 0x96CD2A86, 0xFFFFFBB1,  // 1e-313
    0xED3E12BD, 0xBC807527, 0xFFFFFBB4,  // 1e-312
    0xE88D976C, 0xEBA09271, 0xFFFFFBB7,  // 1e-311
    0x31587EA3, 0x93445B87, 0xFFFFFBBB,  // 1e-310
    0xFDAE9E4C, 0xB8157268, 0xFFFFFBBE,  // 1e-309
    0x3D1A45DF, 0xE61ACF03, 0xFFFFFBC1,  // 1e-308
    0x06306BAC, 0x8FD0C162, 0xFFFFFBC5,  // 1e-307
    0x87BC8697, 0xB3C4F1BA, 0xFFFFFBC8,  // 1e-306
    0x29ABA83C, 0xE0B62E29, 0xFFFFFBCB,  // 1e-305
    0xBA0B4926, 0x8C71DCD9, 0xFFFFFBCF,  // 1e-304
    0x288E1B6F, 0xAF8E5410, 0xFFFFFBD2,  // 1e-303
    0x32B1A24B, 0xDB71E914, 0xFFFFFBD5,  // 1e-302
    0x9FAF056F, 0x892731AC, 0xFFFFFBD9,  // 1e-301
    0xC79AC6CA, 0xAB70FE17, 0xFFFFFBDC,  // 1e-300
    0xB981787D, 0xD64D3D9D, 0xFFFFFBDF,  // 1e-299
    0x93F0EB4E, 0x85F04682, 0xFFFFFBE3,  // 1e-298
    0x38ED2622, 0xA76C5823, 0xFFFFFBE6,  // 1e-297
    0x07286FAA, 0xD1476E2C, 0xFFFFFBE9,  // 1e-296
    0x847945CA, 0x82CCA4DB, 0xFFFFFBED,  // 1e-295
    0x6597973D, 0xA37FCE12, 0xFFFFFBF0,  // 1e-294
    0xFEFD7D0C, 0xCC5FC196, 0xFFFFFBF3,  // 1e-293
    0xBEBCDC4F, 0xFF77B1FC, 0xFFFFFBF6,  // 1e-292
    0xF73609B1, 0x9FAACF3D, 0xFFFFFBFA,  // 1e-291
    0x75038C1E, 0xC795830D, 0xFFFFFBFD,  // 1e-290
    0xD2446F25, 0xF97AE3D0, 0xFFFFFC00,  // 1e-289
    0x836AC577, 0x9BECCE62, 0xFFFFFC04,  // 1e-288
    0x244576D5, 0xC2E801FB, 0xFFFFFC07,  // 1e-287
    0xED56D48A, 0xF3A20279, 0xFFFFFC0A,  // 1e-286
    0x345644D7, 0x9845418C, 0xFFFFFC0E,  // 1e-285
    0x416BD60C, 0xBE5691EF, 0xFFFFFC11,  // 1e-284
    0x11C6CB8F, 0xEDEC366B, 0xFFFFFC14,  // 1e-283
    0xEB1C3F39, 0x94B3A202, 0xFFFFFC18,  // 1e-282
    0xA5E34F08, 0xB9E08A83, 0xFFFFFC1B,  // 1e-281
    0x8F5C22CA, 0xE858AD24, 0xFFFFFC1E,  // 1e-280
    0xD99995BE, 0x91376C36, 0xFFFFFC22,  // 1e-279
    0x8FFFFB2E, 0xB5854744, 0xFFFFFC25,  // 1e-278
    0xB3FFF9F9, 0xE2E69915, 0xFFFFFC28,  // 1e-277
    0x907FFC3C, 0x8DD01FAD, 0xFFFFFC2C,  // 1e-276
    0xF49FFB4B, 0xB1442798, 0xFFFFFC2F,  // 1e-275
    0x31C7FA1D, 0xDD95317F, 0xFFFFFC32,  // 1e-274
    0x7F1CFC52, 0x8A7D3EEF, 0xFFFFFC36,  // 1e-273
    0x5EE43B67, 0xAD1C8EAB, 0xFFFFFC39,  // 1e-272
    0x369D4A41, 0xD863B256, 0xFFFFFC3C,  // 1e-271
    0xE2224E68, 0x873E4F75, 0xFFFFFC40,  // 1e-270
    0x5AAAE202, 0xA90DE353, 0xFFFFFC43,  // 1e-269
    0x31559A83, 0xD3515C28, 0xFFFFFC46,  // 1e-268
    0x1ED58092, 0x8412D999, 0xFFFFFC4A,  // 1e-267
    0x668AE0B6, 0xA5178FFF, 0xFFFFFC4D,  // 1e-266
    0x402D98E4, 0xCE5D73FF, 0xFFFFFC50,  // 1e-265
    0x881C7F8E, 0x80FA687F, 0xFFFFFC54,  // 1e-264
    0x6A239F72, 0xA139029F, 0xFFFFFC57,  // 1e-263
    0x44AC874F, 0xC9874347, 0xFFFFFC5A,  // 1e-262
    0x15D7A922, 0xFBE91419, 0xFFFFFC5D,  // 1e-261
    0xADA6C9B5, 0x9D71AC8F, 0xFFFFFC61,  // 1e-260
    0x99107C23, 0xC4CE17B3, 0xFFFFFC64,  // 1e-259
    0x7F549B2B, 0xF6019DA0, 0xFFFFFC67,  // 1e-258
    0x4F94E0FB, 0x99C10284, 0xFFFFFC6B,  // 1e-257
    0x637A193A, 0xC0314325, 0xFFFFFC6E,  // 1e-256
    0xBC589F88, 0xF03D93EE, 0xFFFFFC71,  // 1e-255
    0x35B763B5, 0x96267C75, 0xFFFFFC75,  // 1e-254
    0x83253CA3, 0xBBB01B92, 0xFFFFFC78,  // 1e-253
    0x23EE8BCB, 0xEA9C2277, 0xFFFFFC7B,  // 1e-252
    0x7675175F, 0x92A1958A, 0xFFFFFC7F,  // 1e-251
    0x14125D37, 0xB749FAED, 0xFFFFFC82,  // 1e-250
    0x5916F485, 0xE51C79A8, 0xFFFFFC85,  // 1e-249
    0x37AE58D3, 0x8F31CC09, 0xFFFFFC89,  // 1e-248
    0x8599EF08, 0xB2FE3F0B, 0xFFFFFC8C,  // 1e-247
    0x67006AC9, 0xDFBDCECE, 0xFFFFFC8F,  // 1e-246
    0x006042BE, 0x8BD6A141, 0xFFFFFC93,  // 1e-245
    0x4078536D, 0xAECC4991, 0xFFFFFC96,  // 1e-244
    0x90966849, 0xDA7F5BF5, 0xFFFFFC99,  // 1e-243
    0x7A5E012D, 0x888F9979, 0xFFFFFC9D,  // 1e-242
    0xD8F58179, 0xAAB37FD7, 0xFFFFFCA0,  // 1e-241
    0xCF32E1D7, 0xD5605FCD, 0xFFFFFCA3,  // 1e-240
    0xA17FCD26, 0x855C3BE0, 0xFFFFFCA7,  // 1e-239
    0xC9DFC070, 0xA6B34AD8, 0xFFFFFCAA,  // 1e-238
    0xFC57B08C, 0xD0601D8E, 0xFFFFFCAD,  // 1e-237
    0x5DB6CE57, 0x823C1279, 0xFFFFFCB1,  // 1e-236
    0xB52481ED, 0xA2CB1717, 0xFFFFFCB4,  // 1e-235
    0xA26DA269, 0xCB7DDCDD, 0xFFFFFCB7,  // 1e-234
    0x0B090B03, 0xFE5D5415, 0xFFFFFCBA,  // 1e-233
    0x26E5A6E2, 0x9EFA548D, 0xFFFFFCBE,  // 1e-232
    0x709F109A, 0xC6B8E9B0, 0xFFFFFCC1,  // 1e-231
    0x8CC6D4C1, 0xF867241C, 0xFFFFFCC4,  // 1e-230
    0xD7FC44F8, 0x9B407691, 0xFFFFFCC8,  // 1e-229
    0x4DFB5637, 0xC2109436, 0xFFFFFCCB,  // 1e-228
    0xE17A2BC4, 0xF294B943, 0xFFFFFCCE,  // 1e-227
    0x6CEC5B5B, 0x979CF3CA, 0xFFFFFCD2,  // 1e-226
    0x08277231, 0xBD8430BD, 0xFFFFFCD5,  // 1e-225
    0x4A314EBE, 0xECE53CEC, 0xFFFFFCD8,  // 1e-224
    0xAE5ED137, 0x940F4613, 0xFFFFFCDC,  // 1e-223
    0x99F68584, 0xB9131798, 0xFFFFFCDF,  // 1e-222
    0xC07426E5, 0xE757DD7E, 0xFFFFFCE2,  // 1e-221
    0x3848984F, 0x9096EA6F, 0xFFFFFCE6,  // 1e-220
    0x065ABE63, 0xB4BCA50B, 0xFFFFFCE9,  // 1e-219
    0xC7F16DFC, 0xE1EBCE4D, 0xFFFFFCEC,  // 1e-218
    0x9CF6E4BD, 0x8D3360F0, 0xFFFFFCF0,  // 1e-217
    0xC4349DED, 0xB080392C, 0xFFFFFCF3,  // 1e-216
    0xF541C568, 0xDCA04777, 0xFFFFFCF6,  // 1e-215
    0xF9491B61, 0x89E42CAA, 0xFFFFFCFA,  // 1e-214
    0xB79B6239, 0xAC5D37D5, 0xFFFFFCFD,  // 1e-213
    0x25823AC7, 0xD77485CB, 0xFFFFFD00,  // 1e-212
    0xF77164BD, 0x86A8D39E, 0xFFFFFD04,  // 1e-211
    0xB54DBDEC, 0xA8530886, 0xFFFFFD07,  // 1e-210
    0x62A12D67, 0xD267CAA8, 0xFFFFFD0A,  // 1e-209
    0x3DA4BC60, 0x8380DEA9, 0xFFFFFD0E,  // 1e-208
    0x8D0DEB78, 0xA4611653, 0xFFFFFD11,  // 1e-207
    0x70516656, 0xCD795BE8, 0xFFFFFD14,  // 1e-206
    0x4632DFF6, 0x806BD971, 0xFFFFFD18,  // 1e-205
    0x97BF97F4, 0xA086CFCD, 0xFFFFFD1B,  // 1e-204
    0xFDAF7DF0, 0xC8A883C0, 0xFFFFFD1E,  // 1e-203
    0x3D1B5D6C, 0xFAD2A4B1, 0xFFFFFD21,  // 1e-202
    0xC6311A64, 0x9CC3A6EE, 0xFFFFFD25,  // 1e-201
    0x77BD60FD, 0xC3F490AA, 0xFFFFFD28,  // 1e-200
    0x15ACB93C, 0xF4F1B4D5, 0xFFFFFD2B,  // 1e-199
    0x2D8BF3C5, 0x99171105, 0xFFFFFD2F,  // 1e-198
    0x78EEF0B7, 0xBF5CD546, 0xFFFFFD32,  // 1e-197
    0x172AACE5, 0xEF340A98, 0xFFFFFD35,  // 1e-196
    0x0E7AAC0F, 0x9580869F, 0xFFFFFD39,  // 1e-195
    0xD2195713, 0xBAE0A846, 0xFFFFFD3C,  // 1e-194
    0x869FACD7, 0xE998D258, 0xFFFFFD3F,  // 1e-193
    0x5423CC06, 0x91FF8377, 0xFFFFFD43,  // 1e-192
    0x292CBF08, 0xB67F6455, 0xFFFFFD46,  // 1e-191
    0x7377EECA, 0xE41F3D6A, 0xFFFFFD49,  // 1e-190
    0x882AF53E, 0x8E938662, 0xFFFFFD4D,  // 1e-189
    0x2A35B28E, 0xB23867FB, 0xFFFFFD50,  // 1e-188
    0xF4C31F31, 0xDEC681F9, 0xFFFFFD53,  // 1e-187
    0x38F9F37F, 0x8B3C113C, 0xFFFFFD57,  // 1e-186
    0x4738705F, 0xAE0B158B, 0xFFFFFD5A,  // 1e-185
    0x19068C76, 0xD98DDAEE, 0xFFFFFD5D,  // 1e-184
    0xCFA417CA, 0x87F8A8D4, 0xFFFFFD61,  // 1e-183
    0x038D1DBC, 0xA9F6D30A, 0xFFFFFD64,  // 1e-182
    0x8470652B, 0xD47487CC, 0xFFFFFD67,  // 1e-181
    0xD2C63F3B, 0x84C8D4DF, 0xFFFFFD6B,  // 1e-180
    0xC777CF0A, 0xA5FB0A17, 0xFFFFFD6E,  // 1e-179
    0xB955C2CC, 0xCF79CC9D, 0xFFFFFD71,  // 1e-178
    0x93D599C0, 0x81AC1FE2, 0xFFFFFD75,  // 1e-177
    0x38CB0030, 0xA21727DB, 0xFFFFFD78,  // 1e-176
    0x06FDC03C, 0xCA9CF1D2, 0xFFFFFD7B,  // 1e-175
    0x88BD304B, 0xFD442E46, 0xFFFFFD7E,  // 1e-174
    0x15763E2F, 0x9E4A9CEC, 0xFFFFFD82,  // 1e-173
    0x1AD3CDBA, 0xC5DD4427, 0xFFFFFD85,  // 1e-172
    0xE188C129, 0xF7549530, 0xFFFFFD88,  // 1e-171
    0x8CF578BA, 0x9A94DD3E, 0xFFFFFD8C,  // 1e-170
    0x3032D6E8, 0xC13A148E, 0xFFFFFD8F,  // 1e-169
    0xBC3F8CA2, 0xF18899B1, 0xFFFFFD92,  // 1e-168
    0x15A7B7E5, 0x96F5600F, 0xFFFFFD96,  // 1e-167
    0xDB11A5DE, 0xBCB2B812, 0xFFFFFD99,  // 1e-166
    0x91D60F56, 0xEBDF6617, 0xFFFFFD9C,  // 1e-165
    0xBB25C996, 0x936B9FCE, 0xFFFFFDA0,  // 1e-164
    0x69EF3BFB, 0xB84687C2, 0xFFFFFDA3,  // 1e-163
    0x046B0AFA, 0xE65829B3, 0xFFFFFDA6,  // 1e-162
    0xE2C2E6DC, 0x8FF71A0F, 0xFFFFFDAA,  // 1e-161
    0xDB73A093, 0xB3F4E093, 0xFFFFFDAD,  // 1e-160
    0xD25088B8, 0xE0F218B8, 0xFFFFFDB0,  // 1e-159
    0x83725573, 0x8C974F73, 0xFFFFFDB4,  // 1e-158
    0x644EEAD0, 0xAFBD2350, 0xFFFFFDB7,  // 1e-157
    0x7D62A584, 0xDBAC6C24, 0xFFFFFDBA,  // 1e-156
    0xCE5DA772, 0x894BC396, 0xFFFFFDBE,  // 1e-155
    0x81F5114F, 0xAB9EB47C, 0xFFFFFDC1,  // 1e-154
    0xA27255A3, 0xD686619B, 0xFFFFFDC4,  // 1e-153
    0x45877586, 0x8613FD01, 0xFFFFFDC8,  // 1e-152
    0x96E952E7, 0xA798FC41, 0xFFFFFDCB,  // 1e-151
    0xFCA3A7A1, 0xD17F3B51, 0xFFFFFDCE,  // 1e-150
    0x3DE648C5, 0x82EF8513, 0xFFFFFDD2,  // 1e-149
    0x0D5FDAF6, 0xA3AB6658, 0xFFFFFDD5,  // 1e-148
    0x10B7D1B3, 0xCC963FEE, 0xFFFFFDD8,  // 1e-147
    0x94E5C620, 0xFFBBCFE9, 0xFFFFFDDB,  // 1e-146
    0xFD0F9BD4, 0x9FD561F1, 0xFFFFFDDF,  // 1e-145
    0x7C5382C9, 0xC7CABA6E, 0xFFFFFDE2,  // 1e-144
    0x1B68637B, 0xF9BD690A, 0xFFFFFDE5,  // 1e-143
    0x51213E2D, 0x9C1661A6, 0xFFFFFDE9,  // 1e-142
    0xE5698DB8, 0xC31BFA0F, 0xFFFFFDEC,  // 1e-141
    0xDEC3F126, 0xF3E2F893, 0xFFFFFDEF,  // 1e-140
    0x6B3A76B8, 0x986DDB5C, 0xFFFFFDF3,  // 1e-139
    0x86091466, 0xBE895233, 0xFFFFFDF6,  // 1e-138
    0x678B597F, 0xEE2BA6C0, 0xFFFFFDF9,  // 1e-137
    0x40B717F0, 0x94DB4838, 0xFFFFFDFD,  // 1e-136
    0x50E4DDEC, 0xBA121A46, 0xFFFFFE00,  // 1e-135
    0xE51E1566, 0xE896A0D7, 0xFFFFFE03,  // 1e-134
    0xEF32CD60, 0x915E2486, 0xFFFFFE07,  // 1e-133
    0xAAFF80B8, 0xB5B5ADA8, 0xFFFFFE0A,  // 1e-132
    0xD5BF60E6, 0xE3231912, 0xFFFFFE0D,  // 1e-131
    0xC5979C90, 0x8DF5EFAB, 0xFFFFFE11,  // 1e-130
    0xB6FD83B4, 0xB1736B96, 0xFFFFFE14,  // 1e-129
    0x64BCE4A1, 0xDDD0467C, 0xFFFFFE17,  // 1e-128
    0xBEF60EE4, 0x8AA22C0D, 0xFFFFFE1B,  // 1e-127
    0x2EB3929E, 0xAD4AB711, 0xFFFFFE1E,  // 1e-126
    0x7A607745, 0xD89D64D5, 0xFFFFFE21,  // 1e-125
    0x6C7C4A8B, 0x87625F05, 0xFFFFFE25,  // 1e-124
    0xC79B5D2E, 0xA93AF6C6, 0xFFFFFE28,  // 1e-123
    0x79823479, 0xD389B478, 0xFFFFFE2B,  // 1e-122
    0x4BF160CC, 0x843610CB, 0xFFFFFE2F,  // 1e-121
    0x1EEDB8FF, 0xA54394FE, 0xFFFFFE32,  // 1e-120
    0xA6A9273E, 0xCE947A3D, 0xFFFFFE35,  // 1e-119
    0x8829B887, 0x811CCC66, 0xFFFFFE39,  // 1e-118
    0x2A3426A9, 0xA163FF80, 0xFFFFFE3C,  // 1e-117
    0x34C13053, 0xC9BCFF60, 0xFFFFFE3F,  // 1e-116
    0x41F17C68, 0xFC2C3F38, 0xFFFFFE42,  // 1e-115
    0x2936EDC1, 0x9D9BA783, 0xFFFFFE46,  // 1e-114
    0xF384A931, 0xC5029163, 0xFFFFFE49,  // 1e-113
    0xF065D37D, 0xF64335BC, 0xFFFFFE4C,  // 1e-112
    0x163FA42E, 0x99EA0196, 0xFFFFFE50,  // 1e-111
    0x9BCF8D3A, 0xC06481FB, 0xFFFFFE53,  // 1e-110
    0x82C37088, 0xF07DA27A, 0xFFFFFE56,  // 1e-109
    0x91BA2655, 0x964E858C, 0xFFFFFE5A,  // 1e-108
    0xB628AFEB, 0xBBE226EF, 0xFFFFFE5D,  // 1e-107
    0xA3B2DBE5, 0xEADAB0AB, 0xFFFFFE60,  // 1e-106
    0x464FC96F, 0x92C8AE6B, 0xFFFFFE64,  // 1e-105
    0x17E3BBCB, 0xB77ADA06, 0xFFFFFE67,  // 1e-104
    0x9DDCAABE, 0xE5599087, 0xFFFFFE6A,  // 1e-103
    0xC2A9EAB7, 0x8F57FA54, 0xFFFFFE6E,  // 1e-102
    0xF3546564, 0xB32DF8E9, 0xFFFFFE71,  // 1e-101
    0x70297EBD, 0xDFF97724, 0xFFFFFE74,  // 1e-100
    0xC619EF36, 0x8BFBEA76, 0xFFFFFE78,  // 1e-99
    0x77A06B04, 0xAEFAE514, 0xFFFFFE7B,  // 1e-98
    0x958885C5, 0xDAB99E59, 0xFFFFFE7E,  // 1e-97
    0xFD75539B, 0x88B402F7, 0xFFFFFE82,  // 1e-96
    0xFCD2A882, 0xAAE103B5, 0xFFFFFE85,  // 1e-95
    0x7C0752A2, 0xD59944A3, 0xFFFFFE88,  // 1e-94
    0x2D8493A5, 0x857FCAE6, 0xFFFFFE8C,  // 1e-93
    0xB8E5B88F, 0xA6DFBD9F, 0xFFFFFE8F,  // 1e-92
    0xA71F26B2, 0xD097AD07, 0xFFFFFE92,  // 1e-91
    0xC8737830, 0x825ECC24, 0xFFFFFE96,  // 1e-90
    0xFA90563B, 0xA2F67F2D, 0xFFFFFE99,  // 1e-89
    0x79346BCA, 0xCBB41EF9, 0xFFFFFE9C,  // 1e-88
    0xD78186BD, 0xFEA126B7, 0xFFFFFE9F,  // 1e-87
    0xE6B0F436, 0x9F24B832, 0xFFFFFEA3,  // 1e-86
    0xA05D3144, 0xC6EDE63F, 0xFFFFFEA6,  // 1e-85
    0x88747D94, 0xF8A95FCF, 0xFFFFFEA9,  // 1e-84
    0xB548CE7D, 0x9B69DBE1, 0xFFFFFEAD,  // 1e-83
    0x229B021C, 0xC24452DA, 0xFFFFFEB0,  // 1e-82
    0xAB41C2A3, 0xF2D56790, 0xFFFFFEB3,  // 1e-81
    0x6B0919A6, 0x97C560BA, 0xFFFFFEB7,  // 1e-80
    0x05CB600F, 0xBDB6B8E9, 0xFFFFFEBA,  // 1e-79
    0x473E3813, 0xED246723, 0xFFFFFEBD,  // 1e-78
    0x0C86E30C, 0x9436C076, 0xFFFFFEC1,  // 1e-77
    0x8FA89BCF, 0xB9447093, 0xFFFFFEC4,  // 1e-76
    0x7392C2C3, 0xE7958CB8, 0xFFFFFEC7,  // 1e-75
    0x483BB9BA, 0x90BD77F3, 0xFFFFFECB,  // 1e-74
    0x1A4AA828, 0xB4ECD5F0, 0xFFFFFECE,  // 1e-73
    0x20DD5232, 0xE2280B6C, 0xFFFFFED1,  // 1e-72
    0x948A535F, 0x8D590723, 0xFFFFFED5,  // 1e-71
    0x79ACE837, 0xB0AF48EC, 0xFFFFFED8,  // 1e-70
    0x98182245, 0xDCDB1B27, 0xFFFFFEDB,  // 1e-69
    0xBF0F156B, 0x8A08F0F8, 0xFFFFFEDF,  // 1e-68
    0xEED2DAC6, 0xAC8B2D36, 0xFFFFFEE2,  // 1e-67
    0xAA879177, 0xD7ADF884, 0xFFFFFEE5,  // 1e-66
    0xEA94BAEB, 0x86CCBB52, 0xFFFFFEE9,  // 1e-65
    0xA539E9A5, 0xA87FEA27, 0xFFFFFEEC,  // 1e-64
    0x8E88640F, 0xD29FE4B1, 0xFFFFFEEF,  // 1e-63
    0xF9153E89, 0x83A3EEEE, 0xFFFFFEF3,  // 1e-62
    0xB75A8E2B, 0xA48CEAAA, 0xFFFFFEF6,  // 1e-61
    0x653131B6, 0xCDB02555, 0xFFFFFEF9,  // 1e-60
    0x5F3EBF12, 0x808E1755, 0xFFFFFEFD,  // 1e-59
    0xB70E6ED6, 0xA0B19D2A, 0xFFFFFF00,  // 1e-58
    0x64D20A8C, 0xC8DE0475, 0xFFFFFF03,  // 1e-57
    0xBE068D2F, 0xFB158592, 0xFFFFFF06,  // 1e-56
    0xB6C4183D, 0x9CED737B, 0xFFFFFF0A,  // 1e-55
    0xA4751E4D, 0xC428D05A, 0xFFFFFF0D,  // 1e-54
    0x4D9265E0, 0xF5330471, 0xFFFFFF10,  // 1e-53
    0xD07B7FAC, 0x993FE2C6, 0xFFFFFF14,  // 1e-52
    0x849A5F97, 0xBF8FDB78, 0xFFFFFF17,  // 1e-51
    0xA5C0F77D, 0xEF73D256, 0xFFFFFF1A,  // 1e-50
    0x27989AAE, 0x95A86376, 0xFFFFFF1E,  // 1e-49
    0xB17EC159, 0xBB127C53, 0xFFFFFF21,  // 1e-48
    0x9DDE71B0, 0xE9D71B68, 0xFFFFFF24,  // 1e-47
    0x62AB070E, 0x92267121, 0xFFFFFF28,  // 1e-46
    0xBB55C8D1, 0xB6B00D69, 0xFFFFFF2B,  // 1e-45
    0x2A2B3B06, 0xE45C10C4, 0xFFFFFF2E,  // 1e-44
    0x9A5B04E3, 0x8EB98A7A, 0xFFFFFF32,  // 1e-43
    0x40F1C61C, 0xB267ED19, 0xFFFFFF35,  // 1e-42
    0x912E37A3, 0xDF01E85F, 0xFFFFFF38,  // 1e-41
    0xBABCE2C6, 0x8B61313B, 0xFFFFFF3C,  // 1e-40
    0xA96C1B78, 0xAE397D8A, 0xFFFFFF3F,  // 1e-39
    0x53C72256, 0xD9C7DCED, 0xFFFFFF42,  // 1e-38
    0x545C7575, 0x881CEA14, 0xFFFFFF46,  // 1e-37
    0x697392D3, 0xAA242499, 0xFFFFFF49,  // 1e-36
    0xC3D07788, 0xD4AD2DBF, 0xFFFFFF4C,  // 1e-35
    0xDA624AB5, 0x84EC3C97, 0xFFFFFF50,  // 1e-34
    0xD0FADD62, 0xA6274BBD, 0xFFFFFF53,  // 1e-33
    0x453994BA, 0xCFB11EAD, 0xFFFFFF56,  // 1e-32
    0x4B43FCF5, 0x81CEB32C, 0xFFFFFF5A,  // 1e-31
    0x5E14FC32, 0xA2425FF7, 0xFFFFFF5D,  // 1e-30
    0x359A3B3E, 0xCAD2F7F5, 0xFFFFFF60,  // 1e-29
    0x8300CA0E, 0xFD87B5F2, 0xFFFFFF63,  // 1e-28
    0x91E07E48, 0x9E74D1B7, 0xFFFFFF67,  // 1e-27
    0x76589DDB, 0xC6120625, 0xFFFFFF6A,  // 1e-26
    0xD3EEC551, 0xF79687AE, 0xFFFFFF6D,  // 1e-25
    0x44753B53, 0x9ABE14CD, 0xFFFFFF71,  // 1e-24
    0x95928A27, 0xC16D9A00, 0xFFFFFF74,  // 1e-23
    0xBAF72CB1, 0xF1C90080, 0xFFFFFF77,  // 1e-22
    0x74DA7BEF, 0x971DA050, 0xFFFFFF7B,  // 1e-21
    0x92111AEB, 0xBCE50864, 0xFFFFFF7E,  // 1e-20
    0xB69561A5, 0xEC1E4A7D, 0xFFFFFF81,  // 1e-19
    0x921D5D07, 0x9392EE8E, 0xFFFFFF85,  // 1e-18
    0x36A4B449, 0xB877AA32, 0xFFFFFF88,  // 1e-17
    0xC44DE15B, 0xE69594BE, 0xFFFFFF8B,  // 1e-16
    0x3AB0ACD9, 0x901D7CF7, 0xFFFFFF8F,  // 1e-15
    0x095CD80F, 0xB424DC35, 0xFFFFFF92,  // 1e-14
    0x4BB40E13, 0xE12E1342, 0xFFFFFF95,  // 1e-13
    0x6F5088CC, 0x8CBCCC09, 0xFFFFFF99,  // 1e-12
    0xCB24AAFF, 0xAFEBFF0B, 0xFFFFFF9C,  // 1e-11
    0xBDEDD5BF, 0xDBE6FECE, 0xFFFFFF9F,  // 1e-10
    0x36B4A597, 0x89705F41, 0xFFFFFFA3,  // 1e-9
    0x8461CEFD, 0xABCC7711, 0xFFFFFFA6,  // 1e-8
    0xE57A42BC, 0xD6BF94D5, 0xFFFFFFA9,  // 1e-7
    0xAF6C69B6, 0x8637BD05, 0xFFFFFFAD,  // 1e-6
    0x1B478423, 0xA7C5AC47, 0xFFFFFFB0,  // 1e-5
    0xE219652C, 0xD1B71758, 0xFFFFFFB3,  // 1e-4
    0x8D4FDF3B, 0x83126E97, 0xFFFFFFB7,  // 1e-3
    0x70A3D70A, 0xA3D70A3D, 0xFFFFFFBA,  // 1e-2
    0xCCCCCCCD, 0xCCCCCCCC, 0xFFFFFFBD,  // 1e-1
    0x00000000, 0x80000000, 0xFFFFFFC1,  // 1e0
    0x00000000, 0xA0000000, 0xFFFFFFC4,  // 1e1
    0x00000000, 0xC8000000, 0xFFFFFFC7,  // 1e2
    0x00000000, 0xFA000000, 0xFFFFFFCA,  // 1e3
    0x00000000, 0x9C400000, 0xFFFFFFCE,  // 1e4
    0x00000000, 0xC3500000, 0xFFFFFFD1,  // 1e5
    0x00000000, 0xF4240000, 0xFFFFFFD4,  // 1e6
    0x00000000, 0x98968000, 0xFFFFFFD8,  // 1e7
    0x00000000, 0xBEBC2000, 0xFFFFFFDB,  // 1e8
    0x00000000, 0xEE6B2800, 0xFFFFFFDE,  // 1e9
    0x00000000, 0x9502F900, 0xFFFFFFE2,  // 1e10
    0x00000000, 0xBA43B740, 0xFFFFFFE5,  // 1e11
    0x00000000, 0xE8D4A510, 0xFFFFFFE8,  // 1e12
    0x00000000, 0x9184E72A, 0xFFFFFFEC,  // 1e13
    0x80000000, 0xB5E620F4, 0xFFFFFFEF,  // 1e14
    0xA0000000, 0xE35FA931, 0xFFFFFFF2,  // 1e15
    0x04000000, 0x8E1BC9BF, 0xFFFFFFF6,  // 1e16
    0xC5000000, 0xB1A2BC2E, 0xFFFFFFF9,  // 1e17
    0x76400000, 0xDE0B6B3A, 0xFFFFFFFC,  // 1e18
    0x89E80000, 0x8AC72304, 0x00000000,  // 1e19
    0xAC620000, 0xAD78EBC5, 0x00000003,  // 1e20
    0x177A8000, 0xD8D726B7, 0x00000006,  // 1e21
    0x6EAC9000, 0x87867832, 0x0000000A,  // 1e22
    0x0A57B400, 0xA968163F, 0x0000000D,  // 1e23
    0xCCEDA100, 0xD3C21BCE, 0x00000010,  // 1e24
    0x401484A0, 0x84595161, 0x00000014,  // 1e25
    0x9019A5C8, 0xA56FA5B9, 0x00000017,  // 1e26
    0xF4200F3A, 0xCECB8F27, 0x0000001A,  // 1e27
    0xF8940984, 0x813F3978, 0x0000001E,  // 1e28
    0x36B90BE5, 0xA18F07D7, 0x00000021,  // 1e29
    0x04674EDF, 0xC9F2C9CD, 0x00000024,  // 1e30
    0x45812296, 0xFC6F7C40, 0x00000027,  // 1e31
    0x2B70B59E, 0x9DC5ADA8, 0x0000002B,  // 1e32
    0x364CE305, 0xC5371912, 0x0000002E,  // 1e33
    0xC3E01BC7, 0xF684DF56, 0x00000031,  // 1e34
    0x3A6C115C, 0x9A130B96, 0x00000035,  // 1e35
    0xC90715B3, 0xC097CE7B, 0x00000038,  // 1e36
    0xBB48DB20, 0xF0BDC21A, 0x0000003B,  // 1e37
    0xB50D88F4, 0x96769950, 0x0000003F,  // 1e38
    0xE250EB31, 0xBC143FA4, 0x00000042,  // 1e39
    0x1AE525FD, 0xEB194F8E, 0x00000045,  // 1e40
    0xD0CF37BE, 0x92EFD1B8, 0x00000049,  // 1e41
    0x050305AE, 0xB7ABC627, 0x0000004C,  // 1e42
    0xC643C719, 0xE596B7B0, 0x0000004F,  // 1e43
    0x7BEA5C70, 0x8F7E32CE, 0x00000053,  // 1e44
    0x1AE4F38C, 0xB35DBF82, 0x00000056,  // 1e45
    0xA19E306F, 0xE0352F62, 0x00000059,  // 1e46
    0xA502DE45, 0x8C213D9D, 0x0000005D,  // 1e47
    0x0E4395D7, 0xAF298D05, 0x00000060,  // 1e48
    0x51D47B4C, 0xDAF3F046, 0x00000063,  // 1e49
    0xF324CD10, 0x88D8762B, 0x00000067,  // 1e50
    0xEFEE0054, 0xAB0E93B6, 0x0000006A,  // 1e51
    0xABE98068, 0xD5D238A4, 0x0000006D,  // 1e52
    0xEB71F041, 0x85A36366, 0x00000071,  // 1e53
    0xA64E6C52, 0xA70C3C40, 0x00000074,  // 1e54
    0xCFE20766, 0xD0CF4B50, 0x00000077,  // 1e55
    0x81ED44A0, 0x82818F12, 0x0000007B,  // 1e56
    0x226895C8, 0xA321F2D7, 0x0000007E,  // 1e57
    0xEB02BB3A, 0xCBEA6F8C, 0x00000081,  // 1e58
    0x25C36A08, 0xFEE50B70, 0x00000084,  // 1e59
    0x179A2245, 0x9F4F2726, 0x00000088,  // 1e60
    0x9D80AAD6, 0xC722F0EF, 0x0000008B,  // 1e61
    0x84E0D58C, 0xF8EBAD2B, 0x0000008E,  // 1e62
    0x330C8577, 0x9B934C3B, 0x00000092,  // 1e63
    0xFFCFA6D5, 0xC2781F49, 0x00000095,  // 1e64
    0x7FC3908B, 0xF316271C, 0x00000098,  // 1e65
    0xCFDA3A57, 0x97EDD871, 0x0000009C,  // 1e66
    0x43D0C8EC, 0xBDE94E8E, 0x0000009F,  // 1e67
    0xD4C4FB27, 0xED63A231, 0x000000A2,  // 1e68
    0x24FB1CF9, 0x945E455F, 0x000000A6,  // 1e69
    0xEE39E437, 0xB975D6B6, 0x000000A9,  // 1e70
    0xA9C85D44, 0xE7D34C64, 0x000000AC,  // 1e71
    0xEA1D3A4B, 0x90E40FBE, 0x000000B0,  // 1e72
    0xA4A488DD, 0xB51D13AE, 0x000000B3,  // 1e73
    0x4DCDAB15, 0xE264589A, 0x000000B6,  // 1e74
    0x70A08AED, 0x8D7EB760, 0x000000BA,  // 1e75
    0x8CC8ADA8, 0xB0DE6538, 0x000000BD,  // 1e76
    0xAFFAD912, 0xDD15FE86, 0x000000C0,  // 1e77
    0x2DFCC7AB, 0x8A2DBF14, 0x000000C4,  // 1e78
    0x397BF996, 0xACB92ED9, 0x000000C7,  // 1e79
    0x87DAF7FC, 0xD7E77A8F, 0x000000CA,  // 1e80
    0xB4E8DAFD, 0x86F0AC99, 0x000000CE,  // 1e81
    0x222311BD, 0xA8ACD7C0, 0x000000D1,  // 1e82
    0x2AABD62C, 0xD2D80DB0, 0x000000D4,  // 1e83
    0x1AAB65DB, 0x83C7088E, 0x000000D8,  // 1e84
    0xA1563F52, 0xA4B8CAB1, 0x000000DB,  // 1e85
    0x09ABCF27, 0xCDE6FD5E, 0x000000DE,  // 1e86
    0xC60B6178, 0x80B05E5A, 0x000000E2,  // 1e87
    0x778E39D6, 0xA0DC75F1, 0x000000E5,  // 1e88
    0xD571C84C, 0xC913936D, 0x000000E8,  // 1e89
    0x4ACE3A5F, 0xFB587849, 0x000000EB,  // 1e90
    0xCEC0E47B, 0x9D174B2D, 0x000000EF,  // 1e91
    0x42711D9A, 0xC45D1DF9, 0x000000F2,  // 1e92
    0x930D6501, 0xF5746577, 0x000000F5,  // 1e93
    0xBBE85F20, 0x9968BF6A, 0x000000F9,  // 1e94
    0x6AE276E9, 0xBFC2EF45, 0x000000FC,  // 1e95
    0xC59B14A3, 0xEFB3AB16, 0x000000FF,  // 1e96
    0x3B80ECE6, 0x95D04AEE, 0x00000103,  // 1e97
    0xCA61281F, 0xBB445DA9, 0x00000106,  // 1e98
    0x3CF97227, 0xEA157514, 0x00000109,  // 1e99
    0xA61BE758, 0x924D692C, 0x0000010D,  // 1e100
    0xCFA2E12E, 0xB6E0C377, 0x00000110,  // 1e101
    0xC38B997A, 0xE498F455, 0x00000113,  // 1e102
    0x9A373FEC, 0x8EDF98B5, 0x00000117,  // 1e103
    0x00C50FE7, 0xB2977EE3, 0x0000011A,  // 1e104
    0xC0F653E1, 0xDF3D5E9B, 0x0000011D,  // 1e105
    0x5899F46D, 0x8B865B21, 0x00000121,  // 1e106
    0xAEC07188, 0xAE67F1E9, 0x00000124,  // 1e107
    0x1A708DEA, 0xDA01EE64, 0x00000127,  // 1e108
    0x908658B2, 0x884134FE, 0x0000012B,  // 1e109
    0x34A7EEDF, 0xAA51823E, 0x0000012E,  // 1e110
    0xC1D1EA96, 0xD4E5E2CD, 0x00000131,  // 1e111
    0x9923329E, 0x850FADC0, 0x00000135,  // 1e112
    0xBF6BFF46, 0xA6539930, 0x00000138,  // 1e113
    0xEF46FF17, 0xCFE87F7C, 0x0000013B,  // 1e114
    0x158C5F6E, 0x81F14FAE, 0x0000013F,  // 1e115
    0x9AEF774A, 0xA26DA399, 0x00000142,  // 1e116
    0x01AB551C, 0xCB090C80, 0x00000145,  // 1e117
    0x02162A63, 0xFDCB4FA0, 0x00000148,  // 1e118
    0x014DDA7E, 0x9E9F11C4, 0x0000014C,  // 1e119
    0x01A1511E, 0xC646D635, 0x0000014F,  // 1e120
    0x4209A565, 0xF7D88BC2, 0x00000152,  // 1e121
    0x6946075F, 0x9AE75759, 0x00000156,  // 1e122
    0xC3978937, 0xC1A12D2F, 0x00000159,  // 1e123
    0xB47D6B85, 0xF209787B, 0x0000015C,  // 1e124
    0x50CE6333, 0x9745EB4D, 0x00000160,  // 1e125
    0xA501FC00, 0xBD176620, 0x00000163,  // 1e126
    0xCE427B00, 0xEC5D3FA8, 0x00000166,  // 1e127
    0x80E98CE0, 0x93BA47C9, 0x0000016A,  // 1e128
    0xE123F018, 0xB8A8D9BB, 0x0000016D,  // 1e129
    0xD96CEC1E, 0xE6D3102A, 0x00000170,  // 1e130
    0xC7E41393, 0x9043EA1A, 0x00000174,  // 1e131
    0x79DD1877, 0xB454E4A1, 0x00000177,  // 1e132
    0xD8545E95, 0xE16A1DC9, 0x0000017A,  // 1e133
    0x2734BB1D, 0x8CE2529E, 0x0000017E,  // 1e134
    0xB101E9E4, 0xB01AE745, 0x00000181,  // 1e135
    0x1D42645D, 0xDC21A117, 0x00000184,  // 1e136
    0x72497EBA, 0x899504AE, 0x00000188,  // 1e137
    0x0EDBDE69, 0xABFA45DA, 0x0000018B,  // 1e138
    0x9292D603, 0xD6F8D750, 0x0000018E,  // 1e139
    0x5B9BC5C2, 0x865B8692, 0x00000192,  // 1e140
    0xF282B733, 0xA7F26836, 0x00000195,  // 1e141
    0xAF2364FF, 0xD1EF0244, 0x00000198,  // 1e142
    0xED761F1F, 0x8335616A, 0x0000019C,  // 1e143
    0xA8D3A6E7, 0xA402B9C5, 0x0000019F,  // 1e144
    0x130890A1, 0xCD036837, 0x000001A2,  // 1e145
    0x6BE55A65, 0x80222122, 0x000001A6,  // 1e146
    0x06DEB0FE, 0xA02AA96B, 0x000001A9,  // 1e147
    0xC8965D3D, 0xC83553C5, 0x000001AC,  // 1e148
    0x3ABBF48D, 0xFA42A8B7, 0x000001AF,  // 1e149
    0x84B578D8, 0x9C69A972, 0x000001B3,  // 1e150
    0x25E2D70E, 0xC38413CF, 0x000001B6,  // 1e151
    0xEF5B8CD1, 0xF46518C2, 0x000001B9,  // 1e152
    0xD5993803, 0x98BF2F79, 0x000001BD,  // 1e153
    0x4AFF8604, 0xBEEEFB58, 0x000001C0,  // 1e154
    0x5DBF6785, 0xEEAABA2E, 0x000001C3,  // 1e155
    0xFA97A0B3, 0x952AB45C, 0x000001C7,  // 1e156
    0x393D88E0, 0xBA756174, 0x000001CA,  // 1e157
    0x478CEB17, 0xE912B9D1, 0x000001CD,  // 1e158
    0xCCB812EF, 0x91ABB422, 0x000001D1,  // 1e159
    0x7FE617AA, 0xB616A12B, 0x000001D4,  // 1e160
    0x5FDF9D95, 0xE39C4976, 0x000001D7,  // 1e161
    0xFBEBC27D, 0x8E41ADE9, 0x000001DB,  // 1e162
    0x7AE6B31C, 0xB1D21964, 0x000001DE,  // 1e163
    0x99A05FE3, 0xDE469FBD, 0x000001E1,  // 1e164
    0x80043BEE, 0x8AEC23D6, 0x000001E5,  // 1e165
    0x20054AEA, 0xADA72CCC, 0x000001E8,  // 1e166
    0x28069DA4, 0xD910F7FF, 0x000001EB,  // 1e167
    0x79042287, 0x87AA9AFF, 0x000001EF,  // 1e168
    0x57452B28, 0xA99541BF, 0x000001F2,  // 1e169
    0x2D1675F2, 0xD3FA922F, 0x000001F5,  // 1e170
    0x7C2E09B7, 0x847C9B5D, 0x000001F9,  // 1e171
    0xDB398C25, 0xA59BC234, 0x000001FC,  // 1e172
    0x1207EF2F, 0xCF02B2C2, 0x000001FF,  // 1e173
    0x4B44F57D, 0x8161AFB9, 0x00000203,  // 1e174
    0x9E1632DC, 0xA1BA1BA7, 0x00000206,  // 1e175
    0x859BBF93, 0xCA28A291, 0x00000209,  // 1e176
    0xE702AF78, 0xFCB2CB35, 0x0000020C,  // 1e177
    0xB061ADAB, 0x9DEFBF01, 0x00000210,  // 1e178
    0x1C7A1916, 0xC56BAEC2, 0x00000213,  // 1e179
    0xA3989F5C, 0xF6C69A72, 0x00000216,  // 1e180
    0xA63F6399, 0x9A3C2087, 0x0000021A,  // 1e181
    0x8FCF3C80, 0xC0CB28A9, 0x0000021D,  // 1e182
    0xF3C30B9F, 0xF0FDF2D3, 0x00000220,  // 1e183
    0x7859E744, 0x969EB7C4, 0x00000224,  // 1e184
    0x96706115, 0xBC4665B5, 0x00000227,  // 1e185
    0xFC0C795A, 0xEB57FF22, 0x0000022A,  // 1e186
    0xDD87CBD8, 0x9316FF75, 0x0000022E,  // 1e187
    0x54E9BECE, 0xB7DCBF53, 0x00000231,  // 1e188
    0x2A242E82, 0xE5D3EF28, 0x00000234,  // 1e189
    0x1A569D11, 0x8FA47579, 0x00000238,  // 1e190
    0x60EC4455, 0xB38D92D7, 0x0000023B,  // 1e191
    0x3927556B, 0xE070F78D, 0x0000023E,  // 1e192
    0x43B89563, 0x8C469AB8, 0x00000242,  // 1e193
    0x54A6BABB, 0xAF584166, 0x00000245,  // 1e194
    0xE9D0696A, 0xDB2E51BF, 0x00000248,  // 1e195
    0xF22241E2, 0x88FCF317, 0x0000024C,  // 1e196
    0xEEAAD25B, 0xAB3C2FDD, 0x0000024F,  // 1e197
    0x6A5586F2, 0xD60B3BD5, 0x00000252,  // 1e198
    0x62757457, 0x85C70565, 0x00000256,  // 1e199
    0xBB12D16D, 0xA738C6BE, 0x00000259,  // 1e200
    0x69D785C8, 0xD106F86E, 0x0000025C,  // 1e201
    0x0226B39D, 0x82A45B45, 0x00000260,  // 1e202
    0x42B06084, 0xA34D7216, 0x00000263,  // 1e203
    0xD35C78A5, 0xCC20CE9B, 0x00000266,  // 1e204
    0xC83396CE, 0xFF290242, 0x00000269,  // 1e205
    0xBD203E41, 0x9F79A169, 0x0000026D,  // 1e206
    0x2C684DD1, 0xC75809C4, 0x00000270,  // 1e207
    0x37826146, 0xF92E0C35, 0x00000273,  // 1e208
    0x42B17CCC, 0x9BBCC7A1, 0x00000277,  // 1e209
    0x935DDBFE, 0xC2ABF989, 0x0000027A,  // 1e210
    0xF83552FE, 0xF356F7EB, 0x0000027D,  // 1e211
    0x7B2153DF, 0x98165AF3, 0x00000281,  // 1e212
    0x59E9A8D6, 0xBE1BF1B0, 0x00000284,  // 1e213
    0x7064130C, 0xEDA2EE1C, 0x00000287,  // 1e214
    0xC63E8BE8, 0x9485D4D1, 0x0000028B,  // 1e215
    0x37CE2EE1, 0xB9A74A06, 0x0000028E,  // 1e216
    0xC5C1BA9A, 0xE8111C87, 0x00000291,  // 1e217
    0xDB9914A0, 0x910AB1D4, 0x00000295,  // 1e218
    0x127F59C8, 0xB54D5E4A, 0x00000298,  // 1e219
    0x971F303A, 0xE2A0B5DC, 0x0000029B,  // 1e220
    0xDE737E24, 0x8DA471A9, 0x0000029F,  // 1e221
    0x56105DAD, 0xB10D8E14, 0x000002A2,  // 1e222
    0x6B947519, 0xDD50F199, 0x000002A5,  // 1e223
    0xE33CC930, 0x8A5296FF, 0x000002A9,  // 1e224
    0xDC0BFB7B, 0xACE73CBF, 0x000002AC,  // 1e225
    0xD30EFA5A, 0xD8210BEF, 0x000002AF,  // 1e226
    0xE3E95C78, 0x8714A775, 0x000002B3,  // 1e227
    0x5CE3B396, 0xA8D9D153, 0x000002B6,  // 1e228
    0x341CA07C, 0xD31045A8, 0x000002B9,  // 1e229
    0x2091E44E, 0x83EA2B89, 0x000002BD,  // 1e230
    0x68B65D61, 0xA4E4B66B, 0x000002C0,  // 1e231
    0x42E3F4B9, 0xCE1DE406, 0x000002C3,  // 1e232
    0xE9CE78F4, 0x80D2AE83, 0x000002C7,  // 1e233
    0xE4421731, 0xA1075A24, 0x000002CA,  // 1e234
    0x1D529CFD, 0xC94930AE, 0x000002CD,  // 1e235
    0xA4A7443C, 0xFB9B7CD9, 0x000002D0,  // 1e236
    0x06E88AA6, 0x9D412E08, 0x000002D4,  // 1e237
    0x08A2AD4F, 0xC491798A, 0x000002D7,  // 1e238
    0x8ACB58A3, 0xF5B5D7EC, 0x000002DA,  // 1e239
    0xD6BF1766, 0x9991A6F3, 0x000002DE,  // 1e240
    0xCC6EDD3F, 0xBFF610B0, 0x000002E1,  // 1e241
    0xFF8A948F, 0xEFF394DC, 0x000002E4,  // 1e242
    0x1FB69CD9, 0x95F83D0A, 0x000002E8,  // 1e243
    0xA7A44410, 0xBB764C4C, 0x000002EB,  // 1e244
    0xD18D5514, 0xEA53DF5F, 0x000002EE,  // 1e245
    0xE2F8552C, 0x92746B9B, 0x000002F2,  // 1e246
    0xDBB66A77, 0xB7118682, 0x000002F5,  // 1e247
    0x92A40515, 0xE4D5E823, 0x000002F8,  // 1e248
    0x3BA6832D, 0x8F05B116, 0x000002FC,  // 1e249
    0xCA9023F8, 0xB2C71D5B, 0x000002FF,  // 1e250
    0xBD342CF7, 0xDF78E4B2, 0x00000302,  // 1e251
    0xB6409C1A, 0x8BAB8EEF, 0x00000306,  // 1e252
    0xA3D0C321, 0xAE9672AB, 0x00000309,  // 1e253
    0x8CC4F3E9, 0xDA3C0F56, 0x0000030C,  // 1e254
    0x17FB1871, 0x88658996, 0x00000310,  // 1e255
    0x9DF9DE8E, 0xAA7EEBFB, 0x00000313,  // 1e256
    0x85785631, 0xD51EA6FA, 0x00000316,  // 1e257
    0x936B35DF, 0x8533285C, 0x0000031A,  // 1e258
    0xB8460357, 0xA67FF273, 0x0000031D,  // 1e259
    0xA657842C, 0xD01FEF10, 0x00000320,  // 1e260
    0x67F6B29C, 0x8213F56A, 0x00000324,  // 1e261
    0x01F45F43, 0xA298F2C5, 0x00000327,  // 1e262
    0x42717713, 0xCB3F2F76, 0x0000032A,  // 1e263
    0xD30DD4D8, 0xFE0EFB53, 0x0000032D,  // 1e264
    0x63E8A507, 0x9EC95D14, 0x00000331,  // 1e265
    0x7CE2CE49, 0xC67BB459, 0x00000334,  // 1e266
    0xDC1B81DB, 0xF81AA16F, 0x00000337,  // 1e267
    0xE9913129, 0x9B10A4E5, 0x0000033B,  // 1e268
    0x63F57D73, 0xC1D4CE1F, 0x0000033E,  // 1e269
    0x3CF2DCD0, 0xF24A01A7, 0x00000341,  // 1e270
    0x8617CA02, 0x976E4108, 0x00000345,  // 1e271
    0xA79DBC82, 0xBD49D14A, 0x00000348,  // 1e272
    0x51852BA3, 0xEC9C459D, 0x0000034B,  // 1e273
    0x52F33B46, 0x93E1AB82, 0x0000034F,  // 1e274
    0xE7B00A17, 0xB8DA1662, 0x00000352,  // 1e275
    0xA19C0C9D, 0xE7109BFB, 0x00000355,  // 1e276
    0x450187E2, 0x906A617D, 0x00000359,  // 1e277
    0x9641E9DB, 0xB484F9DC, 0x0000035C,  // 1e278
    0xBBD26451, 0xE1A63853, 0x0000035F,  // 1e279
    0x55637EB3, 0x8D07E334, 0x00000363,  // 1e280
    0x6ABC5E60, 0xB049DC01, 0x00000366,  // 1e281
    0xC56B75F7, 0xDC5C5301, 0x00000369,  // 1e282
    0x1B6329BB, 0x89B9B3E1, 0x0000036D,  // 1e283
    0x623BF429, 0xAC2820D9, 0x00000370,  // 1e284
    0xBACAF134, 0xD732290F, 0x00000373,  // 1e285
    0xD4BED6C0, 0x867F59A9, 0x00000377,  // 1e286
    0x49EE8C70, 0xA81F3014, 0x0000037A,  // 1e287
    0x5C6A2F8C, 0xD226FC19, 0x0000037D,  // 1e288
    0xD9C25DB8, 0x83585D8F, 0x00000381,  // 1e289
    0xD032F526, 0xA42E74F3, 0x00000384,  // 1e290
    0xC43FB26F, 0xCD3A1230, 0x00000387,  // 1e291
    0x7AA7CF85, 0x80444B5E, 0x0000038B,  // 1e292
    0x1951C367, 0xA0555E36, 0x0000038E,  // 1e293
    0x9FA63441, 0xC86AB5C3, 0x00000391,  // 1e294
    0x878FC151, 0xFA856334, 0x00000394,  // 1e295
    0xD4B9D8D2, 0x9C935E00, 0x00000398,  // 1e296
    0x09E84F07, 0xC3B83581, 0x0000039B,  // 1e297
    0x4C6262C9, 0xF4A642E1, 0x0000039E,  // 1e298
    0xCFBD7DBE, 0x98E7E9CC, 0x000003A2,  // 1e299
    0x03ACDD2D, 0xBF21E440, 0x000003A5,  // 1e300
    0x04981478, 0xEEEA5D50, 0x000003A8,  // 1e301
    0x02DF0CCB, 0x95527A52, 0x000003AC,  // 1e302
    0x8396CFFE, 0xBAA718E6, 0x000003AF,  // 1e303
    0x247C83FD, 0xE950DF20, 0x000003B2,  // 1e304
    0x16CDD27E, 0x91D28B74, 0x000003B6,  // 1e305
    0x1C81471E, 0xB6472E51, 0x000003B9,  // 1e306
    0x63A198E5, 0xE3D8F9E5, 0x000003BC,  // 1e307
    0x5E44FF8F, 0x8E679C2F, 0x000003C0,  // 1e308
    0x35D63F73, 0xB201833B, 0x000003C3,  // 1e309
    0x034BCF50, 0xDE81E40A, 0x000003C6,  // 1e310
};

// wuffs_base__private_implementation__f64_powers_of_10 holds powers of 10 that
// can be exactly represented by a float64 (what C calls a double).
static const double wuffs_base__private_implementation__f64_powers_of_10[23] = {
    1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
    1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22,
};

// --------

// wuffs_base__private_implementation__medium_prec_bin (abbreviated as MPB) is
// a fixed precision floating point binary number. Unlike IEEE 754 Floating
// Point, it cannot represent infinity or NaN (Not a Number).
//
// "Medium precision" means that the mantissa holds 64 binary digits, a little
// more than "double precision", and sizeof(MPB) > sizeof(double). 64 is
// obviously the number of bits in a uint64_t.
//
// An MPB isn't for general purpose arithmetic, only for conversions to and
// from IEEE 754 double-precision floating point.
//
// There is no implicit mantissa bit. The mantissa field is zero if and only if
// the overall floating point value is ±0. An MPB is normalized if the mantissa
// is zero or its high bit (the 1<<63 bit) is set.
//
// There is no negative bit. An MPB can only represent non-negative numbers.
//
// The "all fields are zero" value is valid, and represents the number +0.
//
// This is the "Do It Yourself Floating Point" data structure from Loitsch,
// "Printing Floating-Point Numbers Quickly and Accurately with Integers"
// (https://www.cs.tufts.edu/~nr/cs257/archive/florian-loitsch/printf.pdf).
//
// Florian Loitsch is also the primary contributor to
// https://github.com/google/double-conversion
typedef struct {
  uint64_t mantissa;
  int32_t exp2;
} wuffs_base__private_implementation__medium_prec_bin;

static uint32_t  //
wuffs_base__private_implementation__medium_prec_bin__normalize(
    wuffs_base__private_implementation__medium_prec_bin* m) {
  if (m->mantissa == 0) {
    return 0;
  }
  uint32_t shift = wuffs_base__count_leading_zeroes_u64(m->mantissa);
  m->mantissa <<= shift;
  m->exp2 -= (int32_t)shift;
  return shift;
}

// wuffs_base__private_implementation__medium_prec_bin__mul_pow_10 sets m to be
// (m * pow), where pow comes from an etc__powers_of_10 triple starting at p.
//
// The result is rounded, but not necessarily normalized.
//
// Preconditions:
//  - m is non-NULL.
//  - m->mantissa is non-zero.
//  - m->mantissa's high bit is set (i.e. m is normalized).
//
// The etc__powers_of_10 triple is already normalized.
static void  //
wuffs_base__private_implementation__medium_prec_bin__mul_pow_10(
    wuffs_base__private_implementation__medium_prec_bin* m,
    const uint32_t* p) {
  uint64_t p_mantissa = ((uint64_t)p[0]) | (((uint64_t)p[1]) << 32);
  int32_t p_exp2 = (int32_t)p[2];

  wuffs_base__multiply_u64__output o =
      wuffs_base__multiply_u64(m->mantissa, p_mantissa);
  // Round the mantissa up. It cannot overflow because the maximum possible
  // value of o.hi is 0xFFFFFFFFFFFFFFFE.
  m->mantissa = o.hi + (o.lo >> 63);
  m->exp2 = m->exp2 + p_exp2 + 64;
}

// wuffs_base__private_implementation__medium_prec_bin__as_f64 converts m to a
// double (what C calls a double-precision float64).
//
// Preconditions:
//  - m is non-NULL.
//  - m->mantissa is non-zero.
//  - m->mantissa's high bit is set (i.e. m is normalized).
static double  //
wuffs_base__private_implementation__medium_prec_bin__as_f64(
    const wuffs_base__private_implementation__medium_prec_bin* m,
    bool negative) {
  uint64_t mantissa64 = m->mantissa;
  // An mpb's mantissa has the implicit (binary) decimal point at the right
  // hand end of the mantissa's explicit digits. A double-precision's mantissa
  // has that decimal point near the left hand end. There's also an explicit
  // versus implicit leading 1 bit (binary digit). Together, the difference in
  // semantics corresponds to adding 63.
  int32_t exp2 = m->exp2 + 63;

  // Ensure that exp2 is at least -1022, the minimum double-precision exponent
  // for normal (as opposed to subnormal) numbers.
  if (-1022 > exp2) {
    uint32_t n = (uint32_t)(-1022 - exp2);
    mantissa64 >>= n;
    exp2 += (int32_t)n;
  }

  // Extract the (1 + 52) bits from the 64-bit mantissa64. 52 is the number of
  // explicit mantissa bits in a double-precision f64.
  //
  // Before, we have 64 bits and due to normalization, the high bit 'H' is 1.
  // 63        55        47       etc     15        7
  // H210_9876_5432_1098_7654_etc_etc_etc_5432_1098_7654_3210
  // ++++_++++_++++_++++_++++_etc_etc_etc_++++_+..._...._....  Kept bits.
  // ...._...._...H_2109_8765_etc_etc_etc_6543_2109_8765_4321  After shifting.
  // After, we have 53 bits (and bit #52 is this 'H' bit).
  uint64_t mantissa53 = mantissa64 >> 11;

  // Round up if the old bit #10 (the highest bit dropped by shifting) was set.
  // We also fix any overflow from rounding up.
  if (mantissa64 & 1024) {
    mantissa53++;
    if ((mantissa53 >> 53) != 0) {
      mantissa53 >>= 1;
      exp2++;
    }
  }

  // Handle double-precision infinity (a nominal exponent of 1024) and
  // subnormals (an exponent of -1023 and no implicit mantissa bit, bit #52).
  if (exp2 >= 1024) {
    mantissa53 = 0;
    exp2 = 1024;
  } else if ((mantissa53 >> 52) == 0) {
    exp2 = -1023;
  }

  // Pack the bits and return.
  const int32_t f64_bias = -1023;
  uint64_t exp2_bits =
      (uint64_t)((exp2 - f64_bias) & 0x07FF);           // (1 << 11) - 1.
  uint64_t bits = (mantissa53 & 0x000FFFFFFFFFFFFF) |   // (1 << 52) - 1.
                  (exp2_bits << 52) |                   //
                  (negative ? 0x8000000000000000 : 0);  // (1 << 63).
  return wuffs_base__ieee_754_bit_representation__to_f64(bits);
}

// wuffs_base__private_implementation__medium_prec_bin__parse_number_f64
// converts from an HPD to a double, using an MPB as scratch space. It returns
// a NULL status.repr if there is no ambiguity in the truncation or rounding to
// a float64 (an IEEE 754 double-precision floating point value).
//
// It may modify m even if it returns a non-NULL status.repr.
static wuffs_base__result_f64  //
wuffs_base__private_implementation__medium_prec_bin__parse_number_f64(
    wuffs_base__private_implementation__medium_prec_bin* m,
    const wuffs_base__private_implementation__high_prec_dec* h,
    bool skip_fast_path_for_tests) {
  do {
    // m->mantissa is a uint64_t, which is an integer approximation to a
    // rational value - h's underlying digits after m's normalization. This
    // error is an upper bound on the difference between the approximate and
    // actual value.
    //
    // The DiyFpStrtod function in https://github.com/google/double-conversion
    // uses a finer grain (1/8th of the ULP, Unit in the Last Place) when
    // tracking error. This implementation is coarser (1 ULP) but simpler.
    //
    // It is an error in the "numerical approximation" sense, not in the
    // typical programming sense (as in "bad input" or "a result type").
    uint64_t error = 0;

    // Convert up to 19 decimal digits (in h->digits) to 64 binary digits (in
    // m->mantissa): (1e19 < (1<<64)) and ((1<<64) < 1e20). If we have more
    // than 19 digits, we're truncating (with error).
    uint32_t i;
    uint32_t i_end = h->num_digits;
    if (i_end > 19) {
      i_end = 19;
      error = 1;
    }
    uint64_t mantissa = 0;
    for (i = 0; i < i_end; i++) {
      mantissa = (10 * mantissa) + h->digits[i];
    }
    m->mantissa = mantissa;
    m->exp2 = 0;

    // Check that exp10 lies in the etc__powers_of_10 range (637 triples).
    int32_t exp10 = h->decimal_point - ((int32_t)(i_end));
    if ((exp10 < -326) || (+310 < exp10)) {
      goto fail;
    }

    // Try a fast path, if float64 math would be exact.
    //
    // 15 is such that 1e15 can be losslessly represented in a float64
    // mantissa: (1e15 < (1<<53)) and ((1<<53) < 1e16).
    //
    // 22 is the maximum valid index for the
    // wuffs_base__private_implementation__f64_powers_of_10 array.
    do {
      if (skip_fast_path_for_tests || ((mantissa >> 52) != 0)) {
        break;
      }
      double d = (double)mantissa;

      if (exp10 == 0) {
        wuffs_base__result_f64 ret;
        ret.status.repr = NULL;
        ret.value = h->negative ? -d : +d;
        return ret;

      } else if (exp10 > 0) {
        if (exp10 > 22) {
          if (exp10 > (15 + 22)) {
            break;
          }
          // If exp10 is in the range 23 ..= 37, try moving a few of the zeroes
          // from the exponent to the mantissa. If we're still under 1e15, we
          // haven't truncated any mantissa bits.
          d *= wuffs_base__private_implementation__f64_powers_of_10[exp10 - 22];
          exp10 = 22;
          if (d >= 1e15) {
            break;
          }
        }
        d *= wuffs_base__private_implementation__f64_powers_of_10[exp10];
        wuffs_base__result_f64 ret;
        ret.status.repr = NULL;
        ret.value = h->negative ? -d : +d;
        return ret;

      } else {  // "if (exp10 < 0)" is effectively "if (true)" here.
        if (exp10 < -22) {
          break;
        }
        d /= wuffs_base__private_implementation__f64_powers_of_10[-exp10];
        wuffs_base__result_f64 ret;
        ret.status.repr = NULL;
        ret.value = h->negative ? -d : +d;
        return ret;
      }
    } while (0);

    // Normalize (and scale the error).
    error <<= wuffs_base__private_implementation__medium_prec_bin__normalize(m);

    // Multiplying two MPB values nominally multiplies two mantissas, call them
    // A and B, which are integer approximations to the precise values (A+a)
    // and (B+b) for some error terms a and b.
    //
    // MPB multiplication calculates (((A+a) * (B+b)) >> 64) to be ((A*B) >>
    // 64). Shifting (truncating) and rounding introduces further error. The
    // difference between the calculated result:
    //  ((A*B                  ) >> 64)
    // and the true result:
    //  ((A*B + A*b + a*B + a*b) >> 64)   + rounding_error
    // is:
    //  ((      A*b + a*B + a*b) >> 64)   + rounding_error
    // which can be re-grouped as:
    //  ((A*b) >> 64) + ((a*(B+b)) >> 64) + rounding_error
    //
    // Now, let A and a be "m->mantissa" and "error", and B and b be the
    // pre-calculated power of 10. A and B are both less than (1 << 64), a is
    // the "error" local variable and b is less than 1.
    //
    // An upper bound (in absolute value) on ((A*b) >> 64) is therefore 1.
    //
    // An upper bound on ((a*(B+b)) >> 64) is a, also known as error.
    //
    // Finally, the rounding_error is at most 1.
    //
    // In total, calling mpb__mul_pow_10 will raise the worst-case error by 2.
    // The subsequent re-normalization can multiply that by a further factor.

    // Multiply by powers_of_10[etc].
    wuffs_base__private_implementation__medium_prec_bin__mul_pow_10(
        m,
        &wuffs_base__private_implementation__powers_of_10[3 * (exp10 + 326)]);
    error += 2;
    error <<= wuffs_base__private_implementation__medium_prec_bin__normalize(m);

    // We have a good approximation of h, but we still have to check whether
    // the error is small enough. Equivalently, whether the number of surplus
    // mantissa bits (the bits dropped when going from m's 64 mantissa bits to
    // the smaller number of double-precision mantissa bits) would always round
    // up or down, even when perturbed by ±error. We start at 11 surplus bits
    // (m has 64, double-precision has 1+52), but it can be higher for
    // subnormals.
    //
    // In many cases, the error is small enough and we return true.
    const int32_t f64_bias = -1023;
    int32_t subnormal_exp2 = f64_bias - 63;
    uint32_t surplus_bits = 11;
    if (subnormal_exp2 >= m->exp2) {
      surplus_bits += 1 + ((uint32_t)(subnormal_exp2 - m->exp2));
    }

    uint64_t surplus_mask =
        (((uint64_t)1) << surplus_bits) - 1;  // e.g. 0x07FF.
    uint64_t surplus = m->mantissa & surplus_mask;
    uint64_t halfway = ((uint64_t)1) << (surplus_bits - 1);  // e.g. 0x0400.

    // Do the final calculation in *signed* arithmetic.
    int64_t i_surplus = (int64_t)surplus;
    int64_t i_halfway = (int64_t)halfway;
    int64_t i_error = (int64_t)error;

    if ((i_surplus > (i_halfway - i_error)) &&
        (i_surplus < (i_halfway + i_error))) {
      goto fail;
    }

    wuffs_base__result_f64 ret;
    ret.status.repr = NULL;
    ret.value = wuffs_base__private_implementation__medium_prec_bin__as_f64(
        m, h->negative);
    return ret;
  } while (0);

fail:
  do {
    wuffs_base__result_f64 ret;
    ret.status.repr = "#base: mpb__parse_number_f64 failed";
    ret.value = 0;
    return ret;
  } while (0);
}

// --------

static wuffs_base__result_f64  //
wuffs_base__parse_number_f64_special(wuffs_base__slice_u8 s,
                                     const char* fallback_status_repr) {
  do {
    uint8_t* p = s.ptr;
    uint8_t* q = s.ptr + s.len;

    for (; (p < q) && (*p == '_'); p++) {
    }
    if (p >= q) {
      goto fallback;
    }

    // Parse sign.
    bool negative = false;
    do {
      if (*p == '+') {
        p++;
      } else if (*p == '-') {
        negative = true;
        p++;
      } else {
        break;
      }
      for (; (p < q) && (*p == '_'); p++) {
      }
    } while (0);
    if (p >= q) {
      goto fallback;
    }

    bool nan = false;
    switch (p[0]) {
      case 'I':
      case 'i':
        if (((q - p) < 3) ||                     //
            ((p[1] != 'N') && (p[1] != 'n')) ||  //
            ((p[2] != 'F') && (p[2] != 'f'))) {
          goto fallback;
        }
        p += 3;

        if ((p >= q) || (*p == '_')) {
          break;
        } else if (((q - p) < 5) ||                     //
                   ((p[0] != 'I') && (p[0] != 'i')) ||  //
                   ((p[1] != 'N') && (p[1] != 'n')) ||  //
                   ((p[2] != 'I') && (p[2] != 'i')) ||  //
                   ((p[3] != 'T') && (p[3] != 't')) ||  //
                   ((p[4] != 'Y') && (p[4] != 'y'))) {
          goto fallback;
        }
        p += 5;

        if ((p >= q) || (*p == '_')) {
          break;
        }
        goto fallback;

      case 'N':
      case 'n':
        if (((q - p) < 3) ||                     //
            ((p[1] != 'A') && (p[1] != 'a')) ||  //
            ((p[2] != 'N') && (p[2] != 'n'))) {
          goto fallback;
        }
        p += 3;

        if ((p >= q) || (*p == '_')) {
          nan = true;
          break;
        }
        goto fallback;

      default:
        goto fallback;
    }

    // Finish.
    for (; (p < q) && (*p == '_'); p++) {
    }
    if (p != q) {
      goto fallback;
    }
    wuffs_base__result_f64 ret;
    ret.status.repr = NULL;
    ret.value = wuffs_base__ieee_754_bit_representation__to_f64(
        (nan ? 0x7FFFFFFFFFFFFFFF : 0x7FF0000000000000) |
        (negative ? 0x8000000000000000 : 0));
    return ret;
  } while (0);

fallback:
  do {
    wuffs_base__result_f64 ret;
    ret.status.repr = fallback_status_repr;
    ret.value = 0;
    return ret;
  } while (0);
}

WUFFS_BASE__MAYBE_STATIC wuffs_base__result_f64  //
wuffs_base__parse_number_f64(wuffs_base__slice_u8 s) {
  wuffs_base__private_implementation__medium_prec_bin m;
  wuffs_base__private_implementation__high_prec_dec h;

  do {
    // powers converts decimal powers of 10 to binary powers of 2. For example,
    // (10000 >> 13) is 1. It stops before the elements exceed 60, also known
    // as WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL.
    static const uint32_t num_powers = 19;
    static const uint8_t powers[19] = {
        0,  3,  6,  9,  13, 16, 19, 23, 26, 29,  //
        33, 36, 39, 43, 46, 49, 53, 56, 59,      //
    };

    wuffs_base__status status =
        wuffs_base__private_implementation__high_prec_dec__parse(&h, s);
    if (status.repr) {
      return wuffs_base__parse_number_f64_special(s, status.repr);
    }

    // Handle zero and obvious extremes. The largest and smallest positive
    // finite f64 values are approximately 1.8e+308 and 4.9e-324.
    if ((h.num_digits == 0) || (h.decimal_point < -326)) {
      goto zero;
    } else if (h.decimal_point > 310) {
      goto infinity;
    }

    wuffs_base__result_f64 mpb_result =
        wuffs_base__private_implementation__medium_prec_bin__parse_number_f64(
            &m, &h, false);
    if (mpb_result.status.repr == NULL) {
      return mpb_result;
    }

    // Scale by powers of 2 until we're in the range [½ .. 1], which gives us
    // our exponent (in base-2). First we shift right, possibly a little too
    // far, ending with a value certainly below 1 and possibly below ½...
    const int32_t f64_bias = -1023;
    int32_t exp2 = 0;
    while (h.decimal_point > 0) {
      uint32_t n = (uint32_t)(+h.decimal_point);
      uint32_t shift =
          (n < num_powers)
              ? powers[n]
              : WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL;

      wuffs_base__private_implementation__high_prec_dec__small_rshift(&h,
                                                                      shift);
      if (h.decimal_point <
          -WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
        goto zero;
      }
      exp2 += (int32_t)shift;
    }
    // ...then we shift left, putting us in [½ .. 1].
    while (h.decimal_point <= 0) {
      uint32_t shift;
      if (h.decimal_point == 0) {
        if (h.digits[0] >= 5) {
          break;
        }
        shift = (h.digits[0] <= 2) ? 2 : 1;
      } else {
        uint32_t n = (uint32_t)(-h.decimal_point);
        shift = (n < num_powers)
                    ? powers[n]
                    : WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL;
      }

      wuffs_base__private_implementation__high_prec_dec__small_lshift(&h,
                                                                      shift);
      if (h.decimal_point >
          +WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE) {
        goto infinity;
      }
      exp2 -= (int32_t)shift;
    }

    // We're in the range [½ .. 1] but f64 uses [1 .. 2].
    exp2--;

    // The minimum normal exponent is (f64_bias + 1).
    while ((f64_bias + 1) > exp2) {
      uint32_t n = (uint32_t)((f64_bias + 1) - exp2);
      if (n > WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL) {
        n = WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL;
      }
      wuffs_base__private_implementation__high_prec_dec__small_rshift(&h, n);
      exp2 += (int32_t)n;
    }

    // Check for overflow.
    if ((exp2 - f64_bias) >= 0x07FF) {  // (1 << 11) - 1.
      goto infinity;
    }

    // Extract 53 bits for the mantissa (in base-2).
    wuffs_base__private_implementation__high_prec_dec__small_lshift(&h, 53);
    uint64_t man2 =
        wuffs_base__private_implementation__high_prec_dec__rounded_integer(&h);

    // Rounding might have added one bit. If so, shift and re-check overflow.
    if ((man2 >> 53) != 0) {
      man2 >>= 1;
      exp2++;
      if ((exp2 - f64_bias) >= 0x07FF) {  // (1 << 11) - 1.
        goto infinity;
      }
    }

    // Handle subnormal numbers.
    if ((man2 >> 52) == 0) {
      exp2 = f64_bias;
    }

    // Pack the bits and return.
    uint64_t exp2_bits =
        (uint64_t)((exp2 - f64_bias) & 0x07FF);             // (1 << 11) - 1.
    uint64_t bits = (man2 & 0x000FFFFFFFFFFFFF) |           // (1 << 52) - 1.
                    (exp2_bits << 52) |                     //
                    (h.negative ? 0x8000000000000000 : 0);  // (1 << 63).

    wuffs_base__result_f64 ret;
    ret.status.repr = NULL;
    ret.value = wuffs_base__ieee_754_bit_representation__to_f64(bits);
    return ret;
  } while (0);

zero:
  do {
    uint64_t bits = h.negative ? 0x8000000000000000 : 0;

    wuffs_base__result_f64 ret;
    ret.status.repr = NULL;
    ret.value = wuffs_base__ieee_754_bit_representation__to_f64(bits);
    return ret;
  } while (0);

infinity:
  do {
    uint64_t bits = h.negative ? 0xFFF0000000000000 : 0x7FF0000000000000;

    wuffs_base__result_f64 ret;
    ret.status.repr = NULL;
    ret.value = wuffs_base__ieee_754_bit_representation__to_f64(bits);
    return ret;
  } while (0);
}

// --------

static inline size_t  //
wuffs_base__private_implementation__render_inf(wuffs_base__slice_u8 dst,
                                               bool neg,
                                               uint32_t options) {
  if (neg) {
    if (dst.len < 4) {
      return 0;
    }
    wuffs_base__store_u32le__no_bounds_check(dst.ptr, 0x666E492D);  // '-Inf'le.
    return 4;
  }

  if (options & WUFFS_BASE__RENDER_NUMBER_XXX__LEADING_PLUS_SIGN) {
    if (dst.len < 4) {
      return 0;
    }
    wuffs_base__store_u32le__no_bounds_check(dst.ptr, 0x666E492B);  // '+Inf'le.
    return 4;
  }

  if (dst.len < 3) {
    return 0;
  }
  wuffs_base__store_u24le__no_bounds_check(dst.ptr, 0x666E49);  // 'Inf'le.
  return 3;
}

static inline size_t  //
wuffs_base__private_implementation__render_nan(wuffs_base__slice_u8 dst) {
  if (dst.len < 3) {
    return 0;
  }
  wuffs_base__store_u24le__no_bounds_check(dst.ptr, 0x4E614E);  // 'NaN'le.
  return 3;
}

static size_t  //
wuffs_base__private_implementation__high_prec_dec__render_exponent_absent(
    wuffs_base__slice_u8 dst,
    wuffs_base__private_implementation__high_prec_dec* h,
    uint32_t precision,
    uint32_t options) {
  size_t n = (h->negative ||
              (options & WUFFS_BASE__RENDER_NUMBER_XXX__LEADING_PLUS_SIGN))
                 ? 1
                 : 0;
  if (h->decimal_point <= 0) {
    n += 1;
  } else {
    n += (size_t)(h->decimal_point);
  }
  if (precision > 0) {
    n += precision + 1;  // +1 for the '.'.
  }

  // Don't modify dst if the formatted number won't fit.
  if (n > dst.len) {
    return 0;
  }

  // Align-left or align-right.
  uint8_t* ptr = (options & WUFFS_BASE__RENDER_NUMBER_XXX__ALIGN_RIGHT)
                     ? &dst.ptr[dst.len - n]
                     : &dst.ptr[0];

  // Leading "±".
  if (h->negative) {
    *ptr++ = '-';
  } else if (options & WUFFS_BASE__RENDER_NUMBER_XXX__LEADING_PLUS_SIGN) {
    *ptr++ = '+';
  }

  // Integral digits.
  if (h->decimal_point <= 0) {
    *ptr++ = '0';
  } else {
    uint32_t m =
        wuffs_base__u32__min(h->num_digits, (uint32_t)(h->decimal_point));
    uint32_t i = 0;
    for (; i < m; i++) {
      *ptr++ = (uint8_t)('0' | h->digits[i]);
    }
    for (; i < (uint32_t)(h->decimal_point); i++) {
      *ptr++ = '0';
    }
  }

  // Separator and then fractional digits.
  if (precision > 0) {
    *ptr++ =
        (options & WUFFS_BASE__RENDER_NUMBER_FXX__DECIMAL_SEPARATOR_IS_A_COMMA)
            ? ','
            : '.';
    uint32_t i = 0;
    for (; i < precision; i++) {
      uint32_t j = ((uint32_t)(h->decimal_point)) + i;
      *ptr++ = (uint8_t)('0' | ((j < h->num_digits) ? h->digits[j] : 0));
    }
  }

  return n;
}

static size_t  //
wuffs_base__private_implementation__high_prec_dec__render_exponent_present(
    wuffs_base__slice_u8 dst,
    wuffs_base__private_implementation__high_prec_dec* h,
    uint32_t precision,
    uint32_t options) {
  int32_t exp = 0;
  if (h->num_digits > 0) {
    exp = h->decimal_point - 1;
  }
  bool negative_exp = exp < 0;
  if (negative_exp) {
    exp = -exp;
  }

  size_t n = (h->negative ||
              (options & WUFFS_BASE__RENDER_NUMBER_XXX__LEADING_PLUS_SIGN))
                 ? 4
                 : 3;  // Mininum 3 bytes: first digit and then "e±".
  if (precision > 0) {
    n += precision + 1;  // +1 for the '.'.
  }
  n += (exp < 100) ? 2 : 3;

  // Don't modify dst if the formatted number won't fit.
  if (n > dst.len) {
    return 0;
  }

  // Align-left or align-right.
  uint8_t* ptr = (options & WUFFS_BASE__RENDER_NUMBER_XXX__ALIGN_RIGHT)
                     ? &dst.ptr[dst.len - n]
                     : &dst.ptr[0];

  // Leading "±".
  if (h->negative) {
    *ptr++ = '-';
  } else if (options & WUFFS_BASE__RENDER_NUMBER_XXX__LEADING_PLUS_SIGN) {
    *ptr++ = '+';
  }

  // Integral digit.
  if (h->num_digits > 0) {
    *ptr++ = (uint8_t)('0' | h->digits[0]);
  } else {
    *ptr++ = '0';
  }

  // Separator and then fractional digits.
  if (precision > 0) {
    *ptr++ =
        (options & WUFFS_BASE__RENDER_NUMBER_FXX__DECIMAL_SEPARATOR_IS_A_COMMA)
            ? ','
            : '.';
    uint32_t i = 1;
    uint32_t j = wuffs_base__u32__min(h->num_digits, precision + 1);
    for (; i < j; i++) {
      *ptr++ = (uint8_t)('0' | h->digits[i]);
    }
    for (; i <= precision; i++) {
      *ptr++ = '0';
    }
  }

  // Exponent: "e±" and then 2 or 3 digits.
  *ptr++ = 'e';
  *ptr++ = negative_exp ? '-' : '+';
  if (exp < 10) {
    *ptr++ = '0';
    *ptr++ = (uint8_t)('0' | exp);
  } else if (exp < 100) {
    *ptr++ = (uint8_t)('0' | (exp / 10));
    *ptr++ = (uint8_t)('0' | (exp % 10));
  } else {
    int32_t e = exp / 100;
    exp -= e * 100;
    *ptr++ = (uint8_t)('0' | e);
    *ptr++ = (uint8_t)('0' | (exp / 10));
    *ptr++ = (uint8_t)('0' | (exp % 10));
  }

  return n;
}

WUFFS_BASE__MAYBE_STATIC size_t  //
wuffs_base__render_number_f64(wuffs_base__slice_u8 dst,
                              double x,
                              uint32_t precision,
                              uint32_t options) {
  // Decompose x (64 bits) into negativity (1 bit), base-2 exponent (11 bits
  // with a -1023 bias) and mantissa (52 bits).
  uint64_t bits = wuffs_base__ieee_754_bit_representation__from_f64(x);
  bool neg = (bits >> 63) != 0;
  int32_t exp2 = ((int32_t)(bits >> 52)) & 0x7FF;
  uint64_t man = bits & 0x000FFFFFFFFFFFFFul;

  // Apply the exponent bias and set the implicit top bit of the mantissa,
  // unless x is subnormal. Also take care of Inf and NaN.
  if (exp2 == 0x7FF) {
    if (man != 0) {
      return wuffs_base__private_implementation__render_nan(dst);
    }
    return wuffs_base__private_implementation__render_inf(dst, neg, options);
  } else if (exp2 == 0) {
    exp2 = -1022;
  } else {
    exp2 -= 1023;
    man |= 0x0010000000000000ul;
  }

  // Ensure that precision isn't too large.
  if (precision > 4095) {
    precision = 4095;
  }

  // Convert from the (neg, exp2, man) tuple to an HPD.
  wuffs_base__private_implementation__high_prec_dec h;
  wuffs_base__private_implementation__high_prec_dec__assign(&h, man, neg);
  if (h.num_digits > 0) {
    wuffs_base__private_implementation__high_prec_dec__lshift(
        &h, exp2 - 52);  // 52 mantissa bits.
  }

  // Handle the "%e" and "%f" formats.
  switch (options & (WUFFS_BASE__RENDER_NUMBER_FXX__EXPONENT_ABSENT |
                     WUFFS_BASE__RENDER_NUMBER_FXX__EXPONENT_PRESENT)) {
    case WUFFS_BASE__RENDER_NUMBER_FXX__EXPONENT_ABSENT:  // The "%"f" format.
      if (options & WUFFS_BASE__RENDER_NUMBER_FXX__JUST_ENOUGH_PRECISION) {
        wuffs_base__private_implementation__high_prec_dec__round_just_enough(
            &h, exp2, man);
        int32_t p = ((int32_t)(h.num_digits)) - h.decimal_point;
        precision = ((uint32_t)(wuffs_base__i32__max(0, p)));
      } else {
        wuffs_base__private_implementation__high_prec_dec__round_nearest(
            &h, ((int32_t)precision) + h.decimal_point);
      }
      return wuffs_base__private_implementation__high_prec_dec__render_exponent_absent(
          dst, &h, precision, options);

    case WUFFS_BASE__RENDER_NUMBER_FXX__EXPONENT_PRESENT:  // The "%e" format.
      if (options & WUFFS_BASE__RENDER_NUMBER_FXX__JUST_ENOUGH_PRECISION) {
        wuffs_base__private_implementation__high_prec_dec__round_just_enough(
            &h, exp2, man);
        precision = (h.num_digits > 0) ? (h.num_digits - 1) : 0;
      } else {
        wuffs_base__private_implementation__high_prec_dec__round_nearest(
            &h, ((int32_t)precision) + 1);
      }
      return wuffs_base__private_implementation__high_prec_dec__render_exponent_present(
          dst, &h, precision, options);
  }

  // We have the "%g" format and so precision means the number of significant
  // digits, not the number of digits after the decimal separator. Perform
  // rounding and determine whether to use "%e" or "%f".
  int32_t e_threshold = 0;
  if (options & WUFFS_BASE__RENDER_NUMBER_FXX__JUST_ENOUGH_PRECISION) {
    wuffs_base__private_implementation__high_prec_dec__round_just_enough(
        &h, exp2, man);
    precision = h.num_digits;
    e_threshold = 6;
  } else {
    if (precision == 0) {
      precision = 1;
    }
    wuffs_base__private_implementation__high_prec_dec__round_nearest(
        &h, ((int32_t)precision));
    e_threshold = ((int32_t)precision);
    int32_t nd = ((int32_t)(h.num_digits));
    if ((e_threshold > nd) && (nd >= h.decimal_point)) {
      e_threshold = nd;
    }
  }

  // Use the "%e" format if the exponent is large.
  int32_t e = h.decimal_point - 1;
  if ((e < -4) || (e_threshold <= e)) {
    uint32_t p = wuffs_base__u32__min(precision, h.num_digits);
    return wuffs_base__private_implementation__high_prec_dec__render_exponent_present(
        dst, &h, (p > 0) ? (p - 1) : 0, options);
  }

  // Use the "%f" format otherwise.
  int32_t p = ((int32_t)precision);
  if (p > h.decimal_point) {
    p = ((int32_t)(h.num_digits));
  }
  precision = ((uint32_t)(wuffs_base__i32__max(0, p - h.decimal_point)));
  return wuffs_base__private_implementation__high_prec_dec__render_exponent_absent(
      dst, &h, precision, options);
}
