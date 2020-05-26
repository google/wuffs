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

#define WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DECIMAL_POINT__RANGE 1023
#define WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION 500

// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL is the largest N
// such that ((10 << N) < (1 << 64)).
#define WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__SHIFT__MAX_INCL 60

// wuffs_base__private_implementation__high_prec_dec (abbreviated as HPD) is a
// fixed precision floating point decimal number, augmented with ±infinity
// values, but it cannot represent NaN (Not a Number).
//
// "High precision" means that the mantissa holds 500 decimal digits. 500 is
// WUFFS_BASE__PRIVATE_IMPLEMENTATION__HPD__DIGITS_PRECISION.
//
// An HPD isn't for general purpose arithmetic, only for conversions to and
// from IEEE 754 double-precision floating point, where the largest and
// smallest positive, finite values are approximately 1.8e+308 and 4.9e-324.
// HPD exponents above +1023 mean infinity, below -1023 mean zero. The ±1023
// bounds are further away from zero than ±(324 + 500), where 500 and 1023 is
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
// As above, a decimal_point higher than +1023 means that the overall value is
// infinity, lower than -1023 means zero.
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

// --------

// The wuffs_base__private_implementation__etc_powers_of_10 tables were printed
// by script/print-mpb-powers-of-10.go. That script has an optional -comments
// flag, whose output is not copied here, which prints further detail.
//
// These tables are used in
// wuffs_base__private_implementation__medium_prec_bin__assign_from_hpd.

// wuffs_base__private_implementation__big_powers_of_10 contains approximations
// to the powers of 10, ranging from 1e-348 to 1e+340, with the exponent
// stepping by 8: -348, -340, -332, ..., -12, -4, +4, +12, ..., +340. Each step
// consists of three uint32_t elements. There are 87 triples, 87 * 3 = 261.
//
// For example, the third approximation, for 1e-332, consists of the uint32_t
// triple (0x3055AC76, 0x8B16FB20, 0xFFFFFB72). The first two of that triple
// are a little-endian uint64_t value: 0x8B16FB203055AC76. The last one is an
// int32_t value: -1166. Together, they represent the approximation:
//   1e-332 ≈ 0x8B16FB203055AC76 * (2 ** -1166)
// Similarly, the (0x00000000, 0x9C400000, 0xFFFFFFCE) uint32_t triple means:
//   1e+4   ≈ 0x9C40000000000000 * (2 **   -50)  // This approx'n is exact.
// Similarly, the (0xD4C4FB27, 0xED63A231, 0x000000A2) uint32_t triple means:
//   1e+68  ≈ 0xED63A231D4C4FB27 * (2 **   162)
static const uint32_t
    wuffs_base__private_implementation__big_powers_of_10[261] = {
        0x081C0288, 0xFA8FD5A0, 0xFFFFFB3C, 0xA23EBF76, 0xBAAEE17F, 0xFFFFFB57,
        0x3055AC76, 0x8B16FB20, 0xFFFFFB72, 0x5DCE35EA, 0xCF42894A, 0xFFFFFB8C,
        0x55653B2D, 0x9A6BB0AA, 0xFFFFFBA7, 0x3D1A45DF, 0xE61ACF03, 0xFFFFFBC1,
        0xC79AC6CA, 0xAB70FE17, 0xFFFFFBDC, 0xBEBCDC4F, 0xFF77B1FC, 0xFFFFFBF6,
        0x416BD60C, 0xBE5691EF, 0xFFFFFC11, 0x907FFC3C, 0x8DD01FAD, 0xFFFFFC2C,
        0x31559A83, 0xD3515C28, 0xFFFFFC46, 0xADA6C9B5, 0x9D71AC8F, 0xFFFFFC61,
        0x23EE8BCB, 0xEA9C2277, 0xFFFFFC7B, 0x4078536D, 0xAECC4991, 0xFFFFFC96,
        0x5DB6CE57, 0x823C1279, 0xFFFFFCB1, 0x4DFB5637, 0xC2109436, 0xFFFFFCCB,
        0x3848984F, 0x9096EA6F, 0xFFFFFCE6, 0x25823AC7, 0xD77485CB, 0xFFFFFD00,
        0x97BF97F4, 0xA086CFCD, 0xFFFFFD1B, 0x172AACE5, 0xEF340A98, 0xFFFFFD35,
        0x2A35B28E, 0xB23867FB, 0xFFFFFD50, 0xD2C63F3B, 0x84C8D4DF, 0xFFFFFD6B,
        0x1AD3CDBA, 0xC5DD4427, 0xFFFFFD85, 0xBB25C996, 0x936B9FCE, 0xFFFFFDA0,
        0x7D62A584, 0xDBAC6C24, 0xFFFFFDBA, 0x0D5FDAF6, 0xA3AB6658, 0xFFFFFDD5,
        0xDEC3F126, 0xF3E2F893, 0xFFFFFDEF, 0xAAFF80B8, 0xB5B5ADA8, 0xFFFFFE0A,
        0x6C7C4A8B, 0x87625F05, 0xFFFFFE25, 0x34C13053, 0xC9BCFF60, 0xFFFFFE3F,
        0x91BA2655, 0x964E858C, 0xFFFFFE5A, 0x70297EBD, 0xDFF97724, 0xFFFFFE74,
        0xB8E5B88F, 0xA6DFBD9F, 0xFFFFFE8F, 0x88747D94, 0xF8A95FCF, 0xFFFFFEA9,
        0x8FA89BCF, 0xB9447093, 0xFFFFFEC4, 0xBF0F156B, 0x8A08F0F8, 0xFFFFFEDF,
        0x653131B6, 0xCDB02555, 0xFFFFFEF9, 0xD07B7FAC, 0x993FE2C6, 0xFFFFFF14,
        0x2A2B3B06, 0xE45C10C4, 0xFFFFFF2E, 0x697392D3, 0xAA242499, 0xFFFFFF49,
        0x8300CA0E, 0xFD87B5F2, 0xFFFFFF63, 0x92111AEB, 0xBCE50864, 0xFFFFFF7E,
        0x6F5088CC, 0x8CBCCC09, 0xFFFFFF99, 0xE219652C, 0xD1B71758, 0xFFFFFFB3,
        0x00000000, 0x9C400000, 0xFFFFFFCE, 0x00000000, 0xE8D4A510, 0xFFFFFFE8,
        0xAC620000, 0xAD78EBC5, 0x00000003, 0xF8940984, 0x813F3978, 0x0000001E,
        0xC90715B3, 0xC097CE7B, 0x00000038, 0x7BEA5C70, 0x8F7E32CE, 0x00000053,
        0xABE98068, 0xD5D238A4, 0x0000006D, 0x179A2245, 0x9F4F2726, 0x00000088,
        0xD4C4FB27, 0xED63A231, 0x000000A2, 0x8CC8ADA8, 0xB0DE6538, 0x000000BD,
        0x1AAB65DB, 0x83C7088E, 0x000000D8, 0x42711D9A, 0xC45D1DF9, 0x000000F2,
        0xA61BE758, 0x924D692C, 0x0000010D, 0x1A708DEA, 0xDA01EE64, 0x00000127,
        0x9AEF774A, 0xA26DA399, 0x00000142, 0xB47D6B85, 0xF209787B, 0x0000015C,
        0x79DD1877, 0xB454E4A1, 0x00000177, 0x5B9BC5C2, 0x865B8692, 0x00000192,
        0xC8965D3D, 0xC83553C5, 0x000001AC, 0xFA97A0B3, 0x952AB45C, 0x000001C7,
        0x99A05FE3, 0xDE469FBD, 0x000001E1, 0xDB398C25, 0xA59BC234, 0x000001FC,
        0xA3989F5C, 0xF6C69A72, 0x00000216, 0x54E9BECE, 0xB7DCBF53, 0x00000231,
        0xF22241E2, 0x88FCF317, 0x0000024C, 0xD35C78A5, 0xCC20CE9B, 0x00000266,
        0x7B2153DF, 0x98165AF3, 0x00000281, 0x971F303A, 0xE2A0B5DC, 0x0000029B,
        0x5CE3B396, 0xA8D9D153, 0x000002B6, 0xA4A7443C, 0xFB9B7CD9, 0x000002D0,
        0xA7A44410, 0xBB764C4C, 0x000002EB, 0xB6409C1A, 0x8BAB8EEF, 0x00000306,
        0xA657842C, 0xD01FEF10, 0x00000320, 0xE9913129, 0x9B10A4E5, 0x0000033B,
        0xA19C0C9D, 0xE7109BFB, 0x00000355, 0x623BF429, 0xAC2820D9, 0x00000370,
        0x7AA7CF85, 0x80444B5E, 0x0000038B, 0x03ACDD2D, 0xBF21E440, 0x000003A5,
        0x5E44FF8F, 0x8E679C2F, 0x000003C0, 0x9C8CB841, 0xD433179D, 0x000003DA,
        0xB4E31BA9, 0x9E19DB92, 0x000003F5, 0xBADF77D9, 0xEB96BF6E, 0x0000040F,
        0x9BF0EE6B, 0xAF87023B, 0x0000042A,
};

// wuffs_base__private_implementation__small_powers_of_10 contains
// approximations to the powers of 10, ranging from 1e+0 to 1e+7, with the
// exponent stepping by 1. Each step consists of three uint32_t elements.
//
// For example, the third approximation, for 1e+2, consists of the uint32_t
// triple (0x00000000, 0xC8000000, 0xFFFFFFC7). The first two of that triple
// are a little-endian uint64_t value: 0xC800000000000000. The last one is an
// int32_t value: -57. Together, they represent the approximation:
//   1e+2   ≈ 0xC800000000000000 * (2 **   -57)  // This approx'n is exact.
// Similarly, the (0x00000000, 0x9C400000, 0xFFFFFFCE) uint32_t triple means:
//   1e+4   ≈ 0x9C40000000000000 * (2 **   -50)  // This approx'n is exact.
static const uint32_t
    wuffs_base__private_implementation__small_powers_of_10[24] = {
        0x00000000, 0x80000000, 0xFFFFFFC1, 0x00000000, 0xA0000000, 0xFFFFFFC4,
        0x00000000, 0xC8000000, 0xFFFFFFC7, 0x00000000, 0xFA000000, 0xFFFFFFCA,
        0x00000000, 0x9C400000, 0xFFFFFFCE, 0x00000000, 0xC3500000, 0xFFFFFFD1,
        0x00000000, 0xF4240000, 0xFFFFFFD4, 0x00000000, 0x98968000, 0xFFFFFFD8,
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
// (m * pow), where pow comes from an etc_powers_of_10 triple starting at p.
//
// The result is rounded, but not necessarily normalized.
//
// Preconditions:
//  - m is non-NULL.
//  - m->mantissa is non-zero.
//  - m->mantissa's high bit is set (i.e. m is normalized).
//
// The etc_powers_of_10 triple is already normalized.
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

    // Check that exp10 lies in the (big_powers_of_10 + small_powers_of_10)
    // range, -348 ..= +347, stepping big_powers_of_10 by 8 (which is 87
    // triples) and small_powers_of_10 by 1 (which is 8 triples).
    int32_t exp10 = h->decimal_point - ((int32_t)(i_end));
    if (exp10 < -348) {
      goto fail;
    }
    uint32_t bpo10 = ((uint32_t)(exp10 + 348)) / 8;
    uint32_t spo10 = ((uint32_t)(exp10 + 348)) % 8;
    if (bpo10 >= 87) {
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
          if (exp10 > 22) {
            d *= wuffs_base__private_implementation__f64_powers_of_10[exp10 -
                                                                      22];
            exp10 = 22;
            if (d >= 1e15) {
              break;
            }
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

    // Multiply by small_powers_of_10[etc].
    wuffs_base__private_implementation__medium_prec_bin__mul_pow_10(
        m, &wuffs_base__private_implementation__small_powers_of_10[3 * spo10]);
    error += 2;
    error <<= wuffs_base__private_implementation__medium_prec_bin__normalize(m);

    // Multiply by big_powers_of_10[etc].
    wuffs_base__private_implementation__medium_prec_bin__mul_pow_10(
        m, &wuffs_base__private_implementation__big_powers_of_10[3 * bpo10]);
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
