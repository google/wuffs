// After editing this file, run "go generate" in the ../data directory.

// Copyright 2017 The Wuffs Authors.
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

// ---------------- Version

// WUFFS_VERSION is the major.minor.patch version, as per https://semver.org/,
// as a uint64_t. The major number is the high 32 bits. The minor number is the
// middle 16 bits. The patch number is the low 16 bits. The pre-release label
// and build metadata are part of the string representation (such as
// "1.2.3-beta+456.20181231") but not the uint64_t representation.
//
// WUFFS_VERSION_PRE_RELEASE_LABEL (such as "", "beta" or "rc.1") being
// non-empty denotes a developer preview, not a release version, and has no
// backwards or forwards compatibility guarantees.
//
// WUFFS_VERSION_BUILD_METADATA_XXX, if non-zero, are the number of commits and
// the last commit date in the repository used to build this library. Within
// each major.minor branch, the commit count should increase monotonically.
//
// !! Some code generation programs can override WUFFS_VERSION.
#define WUFFS_VERSION 0
#define WUFFS_VERSION_MAJOR 0
#define WUFFS_VERSION_MINOR 0
#define WUFFS_VERSION_PATCH 0
#define WUFFS_VERSION_PRE_RELEASE_LABEL "work.in.progress"
#define WUFFS_VERSION_BUILD_METADATA_COMMIT_COUNT 0
#define WUFFS_VERSION_BUILD_METADATA_COMMIT_DATE 0
#define WUFFS_VERSION_STRING "0.0.0+0.00000000"

// ---------------- Configuration

// Define WUFFS_CONFIG__AVOID_CPU_ARCH to avoid any code tied to a specific CPU
// architecture, such as SSE SIMD for the x86 CPU family.
#if defined(WUFFS_CONFIG__AVOID_CPU_ARCH)  // (#if-chain ref AVOID_CPU_ARCH_0)
// No-op.
#else  // (#if-chain ref AVOID_CPU_ARCH_0)

// The "defined(__clang__)" isn't redundant. While vanilla clang defines
// __GNUC__, clang-cl (which mimics MSVC's cl.exe) does not.
#if defined(__GNUC__) || defined(__clang__)
#define WUFFS_BASE__MAYBE_ATTRIBUTE_TARGET(arg) __attribute__((target(arg)))
#else
#define WUFFS_BASE__MAYBE_ATTRIBUTE_TARGET(arg)
#endif  // defined(__GNUC__) || defined(__clang__)

#if defined(__GNUC__)  // (#if-chain ref AVOID_CPU_ARCH_1)

// To simplify Wuffs code, "cpu_arch >= arm_xxx" requires xxx but also
// unaligned little-endian load/stores.
#if defined(__ARM_FEATURE_UNALIGNED) && defined(__BYTE_ORDER__) && \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
// Not all gcc versions define __ARM_ACLE, even if they support crc32
// intrinsics. Look for __ARM_FEATURE_CRC32 instead.
#if defined(__ARM_FEATURE_CRC32)
#include <arm_acle.h>
#define WUFFS_BASE__CPU_ARCH__ARM_CRC32
#endif  // defined(__ARM_FEATURE_CRC32)
#if defined(__ARM_NEON)
#include <arm_neon.h>
#define WUFFS_BASE__CPU_ARCH__ARM_NEON
#endif  // defined(__ARM_NEON)
#endif  // defined(__ARM_FEATURE_UNALIGNED) etc

// Similarly, "cpu_arch >= x86_sse42" requires SSE4.2 but also PCLMUL and
// POPCNT. This is checked at runtime via cpuid, not at compile time.
#if defined(__x86_64__)
#include <cpuid.h>
#include <x86intrin.h>
#define WUFFS_BASE__CPU_ARCH__X86_64
#endif  // defined(__x86_64__)

#elif defined(_MSC_VER)  // (#if-chain ref AVOID_CPU_ARCH_1)

#if defined(_M_X64)
#if defined(__AVX__) || defined(__clang__)

// We need <intrin.h> for the __cpuid function.
#include <intrin.h>
// That's not enough for X64 SIMD, with clang-cl, if we want to use
// "__attribute__((target(arg)))" without e.g. "/arch:AVX".
//
// Some web pages suggest that <immintrin.h> is all you need, as it pulls in
// the earlier SIMD families like SSE4.2, but that doesn't seem to work in
// practice, possibly for the same reason that just <intrin.h> doesn't work.
#include <immintrin.h>  // AVX, AVX2, FMA, POPCNT
#include <nmmintrin.h>  // SSE4.2
#include <wmmintrin.h>  // AES, PCLMUL
#define WUFFS_BASE__CPU_ARCH__X86_64

#else  // defined(__AVX__) || defined(__clang__)

// clang-cl (which defines both __clang__ and _MSC_VER) supports
// "__attribute__((target(arg)))".
//
// For MSVC's cl.exe (unlike clang or gcc), SIMD capability is a compile-time
// property of the source file (e.g. a /arch:AVX or -mavx compiler flag), not
// of individual functions (that can be conditionally selected at runtime).
#pragma message("Wuffs with MSVC+X64 needs /arch:AVX for best performance")

#endif  // defined(__AVX__) || defined(__clang__)
#endif  // defined(_M_X64)

#endif  // (#if-chain ref AVOID_CPU_ARCH_1)
#endif  // (#if-chain ref AVOID_CPU_ARCH_0)

// --------

// Define WUFFS_CONFIG__STATIC_FUNCTIONS to make all of Wuffs' functions have
// static storage. The motivation is discussed in the "ALLOW STATIC
// IMPLEMENTATION" section of
// https://raw.githubusercontent.com/nothings/stb/master/docs/stb_howto.txt
#if defined(WUFFS_CONFIG__STATIC_FUNCTIONS)
#define WUFFS_BASE__MAYBE_STATIC static
#else
#define WUFFS_BASE__MAYBE_STATIC
#endif  // defined(WUFFS_CONFIG__STATIC_FUNCTIONS)

// ---------------- CPU Architecture

static inline bool  //
wuffs_base__cpu_arch__have_arm_crc32() {
#if defined(WUFFS_BASE__CPU_ARCH__ARM_CRC32)
  return true;
#else
  return false;
#endif  // defined(WUFFS_BASE__CPU_ARCH__ARM_CRC32)
}

static inline bool  //
wuffs_base__cpu_arch__have_arm_neon() {
#if defined(WUFFS_BASE__CPU_ARCH__ARM_NEON)
  return true;
#else
  return false;
#endif  // defined(WUFFS_BASE__CPU_ARCH__ARM_NEON)
}

static inline bool  //
wuffs_base__cpu_arch__have_x86_sse42() {
#if defined(WUFFS_BASE__CPU_ARCH__X86_64)
  // GCC defines these macros but MSVC does not.
  //  - bit_PCLMUL = (1 <<  1)
  //  - bit_POPCNT = (1 << 23)
  //  - bit_SSE4_2 = (1 << 20)
  const unsigned int sse42_ecx1 = 0x00900002;

  // clang defines __GNUC__ and clang-cl defines _MSC_VER (but not __GNUC__).
#if defined(__GNUC__)
  unsigned int eax1 = 0;
  unsigned int ebx1 = 0;
  unsigned int ecx1 = 0;
  unsigned int edx1 = 0;
  if (__get_cpuid(1, &eax1, &ebx1, &ecx1, &edx1)) {
    return (ecx1 & sse42_ecx1) == sse42_ecx1;
  }
#elif defined(_MSC_VER)  // defined(__GNUC__)
  int x[4];
  __cpuid(x, 1);
  return (((unsigned int)(x[2])) & sse42_ecx1) == sse42_ecx1;
#else
#error "WUFFS_BASE__CPU_ARCH__ETC combined with an unsupported compiler"
#endif  // defined(__GNUC__); defined(_MSC_VER)
#endif  // defined(WUFFS_BASE__CPU_ARCH__X86_64)
  return false;
}

// ---------------- Fundamentals

// Wuffs assumes that:
//  - converting a uint32_t to a size_t will never overflow.
//  - converting a size_t to a uint64_t will never overflow.
#ifdef __WORDSIZE
#if (__WORDSIZE != 32) && (__WORDSIZE != 64)
#error "Wuffs requires a word size of either 32 or 64 bits"
#endif
#endif

#if defined(__clang__)
#define WUFFS_BASE__POTENTIALLY_UNUSED_FIELD __attribute__((unused))
#else
#define WUFFS_BASE__POTENTIALLY_UNUSED_FIELD
#endif

// Clang also defines "__GNUC__".
#if defined(__GNUC__)
#define WUFFS_BASE__POTENTIALLY_UNUSED __attribute__((unused))
#define WUFFS_BASE__WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WUFFS_BASE__POTENTIALLY_UNUSED
#define WUFFS_BASE__WARN_UNUSED_RESULT
#endif

// --------

// Options (bitwise or'ed together) for wuffs_foo__bar__initialize functions.

#define WUFFS_INITIALIZE__DEFAULT_OPTIONS ((uint32_t)0x00000000)

// WUFFS_INITIALIZE__ALREADY_ZEROED means that the "self" receiver struct value
// has already been set to all zeroes.
#define WUFFS_INITIALIZE__ALREADY_ZEROED ((uint32_t)0x00000001)

// WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED means that, absent
// WUFFS_INITIALIZE__ALREADY_ZEROED, only some of the "self" receiver struct
// value will be set to all zeroes. Internal buffers, which tend to be a large
// proportion of the struct's size, will be left uninitialized. Internal means
// that the buffer is contained by the receiver struct, as opposed to being
// passed as a separately allocated "work buffer".
//
// For more detail, see:
// https://github.com/google/wuffs/blob/main/doc/note/initialization.md
#define WUFFS_INITIALIZE__LEAVE_INTERNAL_BUFFERS_UNINITIALIZED \
  ((uint32_t)0x00000002)

// --------

// wuffs_base__empty_struct is used when a Wuffs function returns an empty
// struct. In C, if a function f returns void, you can't say "x = f()", but in
// Wuffs, if a function g returns empty, you can say "y = g()".
typedef struct wuffs_base__empty_struct__struct {
  // private_impl is a placeholder field. It isn't explicitly used, except that
  // without it, the sizeof a struct with no fields can differ across C/C++
  // compilers, and it is undefined behavior in C99. For example, gcc says that
  // the sizeof an empty struct is 0, and g++ says that it is 1. This leads to
  // ABI incompatibility if a Wuffs .c file is processed by one compiler and
  // its .h file with another compiler.
  //
  // Instead, we explicitly insert an otherwise unused field, so that the
  // sizeof this struct is always 1.
  uint8_t private_impl;
} wuffs_base__empty_struct;

static inline wuffs_base__empty_struct  //
wuffs_base__make_empty_struct() {
  wuffs_base__empty_struct ret;
  ret.private_impl = 0;
  return ret;
}

// wuffs_base__utility is a placeholder receiver type. It enables what Java
// calls static methods, as opposed to regular methods.
typedef struct wuffs_base__utility__struct {
  // private_impl is a placeholder field. It isn't explicitly used, except that
  // without it, the sizeof a struct with no fields can differ across C/C++
  // compilers, and it is undefined behavior in C99. For example, gcc says that
  // the sizeof an empty struct is 0, and g++ says that it is 1. This leads to
  // ABI incompatibility if a Wuffs .c file is processed by one compiler and
  // its .h file with another compiler.
  //
  // Instead, we explicitly insert an otherwise unused field, so that the
  // sizeof this struct is always 1.
  uint8_t private_impl;
} wuffs_base__utility;

typedef struct wuffs_base__vtable__struct {
  const char* vtable_name;
  const void* function_pointers;
} wuffs_base__vtable;

// --------

// See https://github.com/google/wuffs/blob/main/doc/note/statuses.md
typedef struct wuffs_base__status__struct {
  const char* repr;

#ifdef __cplusplus
  inline bool is_complete() const;
  inline bool is_error() const;
  inline bool is_note() const;
  inline bool is_ok() const;
  inline bool is_suspension() const;
  inline const char* message() const;
#endif  // __cplusplus

} wuffs_base__status;

// !! INSERT wuffs_base__status names.

static inline wuffs_base__status  //
wuffs_base__make_status(const char* repr) {
  wuffs_base__status z;
  z.repr = repr;
  return z;
}

static inline bool  //
wuffs_base__status__is_complete(const wuffs_base__status* z) {
  return (z->repr == NULL) || ((*z->repr != '$') && (*z->repr != '#'));
}

static inline bool  //
wuffs_base__status__is_error(const wuffs_base__status* z) {
  return z->repr && (*z->repr == '#');
}

static inline bool  //
wuffs_base__status__is_note(const wuffs_base__status* z) {
  return z->repr && (*z->repr != '$') && (*z->repr != '#');
}

static inline bool  //
wuffs_base__status__is_ok(const wuffs_base__status* z) {
  return z->repr == NULL;
}

static inline bool  //
wuffs_base__status__is_suspension(const wuffs_base__status* z) {
  return z->repr && (*z->repr == '$');
}

// wuffs_base__status__message strips the leading '$', '#' or '@'.
static inline const char*  //
wuffs_base__status__message(const wuffs_base__status* z) {
  if (z->repr) {
    if ((*z->repr == '$') || (*z->repr == '#') || (*z->repr == '@')) {
      return z->repr + 1;
    }
  }
  return z->repr;
}

#ifdef __cplusplus

inline bool  //
wuffs_base__status::is_complete() const {
  return wuffs_base__status__is_complete(this);
}

inline bool  //
wuffs_base__status::is_error() const {
  return wuffs_base__status__is_error(this);
}

inline bool  //
wuffs_base__status::is_note() const {
  return wuffs_base__status__is_note(this);
}

inline bool  //
wuffs_base__status::is_ok() const {
  return wuffs_base__status__is_ok(this);
}

inline bool  //
wuffs_base__status::is_suspension() const {
  return wuffs_base__status__is_suspension(this);
}

inline const char*  //
wuffs_base__status::message() const {
  return wuffs_base__status__message(this);
}

#endif  // __cplusplus

// --------

// WUFFS_BASE__RESULT is a result type: either a status (an error) or a value.
//
// A result with all fields NULL or zero is as valid as a zero-valued T.
#define WUFFS_BASE__RESULT(T)  \
  struct {                     \
    wuffs_base__status status; \
    T value;                   \
  }

typedef WUFFS_BASE__RESULT(double) wuffs_base__result_f64;
typedef WUFFS_BASE__RESULT(int64_t) wuffs_base__result_i64;
typedef WUFFS_BASE__RESULT(uint64_t) wuffs_base__result_u64;

// --------

// wuffs_base__transform__output is the result of transforming from a src slice
// to a dst slice.
typedef struct wuffs_base__transform__output__struct {
  wuffs_base__status status;
  size_t num_dst;
  size_t num_src;
} wuffs_base__transform__output;

// --------

// FourCC constants.

// !! INSERT FourCCs.

// --------

// Quirks.

// !! INSERT Quirks.

// --------

// Flicks are a unit of time. One flick (frame-tick) is 1 / 705_600_000 of a
// second. See https://github.com/OculusVR/Flicks
typedef int64_t wuffs_base__flicks;

#define WUFFS_BASE__FLICKS_PER_SECOND ((uint64_t)705600000)
#define WUFFS_BASE__FLICKS_PER_MILLISECOND ((uint64_t)705600)

// ---------------- Numeric Types

// The helpers below are functions, instead of macros, because their arguments
// can be an expression that we shouldn't evaluate more than once.
//
// They are static, so that linking multiple wuffs .o files won't complain about
// duplicate function definitions.
//
// They are explicitly marked inline, even if modern compilers don't use the
// inline attribute to guide optimizations such as inlining, to avoid the
// -Wunused-function warning, and we like to compile with -Wall -Werror.

static inline int8_t  //
wuffs_base__i8__min(int8_t x, int8_t y) {
  return x < y ? x : y;
}

static inline int8_t  //
wuffs_base__i8__max(int8_t x, int8_t y) {
  return x > y ? x : y;
}

static inline int16_t  //
wuffs_base__i16__min(int16_t x, int16_t y) {
  return x < y ? x : y;
}

static inline int16_t  //
wuffs_base__i16__max(int16_t x, int16_t y) {
  return x > y ? x : y;
}

static inline int32_t  //
wuffs_base__i32__min(int32_t x, int32_t y) {
  return x < y ? x : y;
}

static inline int32_t  //
wuffs_base__i32__max(int32_t x, int32_t y) {
  return x > y ? x : y;
}

static inline int64_t  //
wuffs_base__i64__min(int64_t x, int64_t y) {
  return x < y ? x : y;
}

static inline int64_t  //
wuffs_base__i64__max(int64_t x, int64_t y) {
  return x > y ? x : y;
}

static inline uint8_t  //
wuffs_base__u8__min(uint8_t x, uint8_t y) {
  return x < y ? x : y;
}

static inline uint8_t  //
wuffs_base__u8__max(uint8_t x, uint8_t y) {
  return x > y ? x : y;
}

static inline uint16_t  //
wuffs_base__u16__min(uint16_t x, uint16_t y) {
  return x < y ? x : y;
}

static inline uint16_t  //
wuffs_base__u16__max(uint16_t x, uint16_t y) {
  return x > y ? x : y;
}

static inline uint32_t  //
wuffs_base__u32__min(uint32_t x, uint32_t y) {
  return x < y ? x : y;
}

static inline uint32_t  //
wuffs_base__u32__max(uint32_t x, uint32_t y) {
  return x > y ? x : y;
}

static inline uint64_t  //
wuffs_base__u64__min(uint64_t x, uint64_t y) {
  return x < y ? x : y;
}

static inline uint64_t  //
wuffs_base__u64__max(uint64_t x, uint64_t y) {
  return x > y ? x : y;
}

// --------

// Saturating arithmetic (sat_add, sat_sub) branchless bit-twiddling algorithms
// are per https://locklessinc.com/articles/sat_arithmetic/
//
// It is important that the underlying types are unsigned integers, as signed
// integer arithmetic overflow is undefined behavior in C.

static inline uint8_t  //
wuffs_base__u8__sat_add(uint8_t x, uint8_t y) {
  uint8_t res = (uint8_t)(x + y);
  res |= (uint8_t)(-(res < x));
  return res;
}

static inline uint8_t  //
wuffs_base__u8__sat_sub(uint8_t x, uint8_t y) {
  uint8_t res = (uint8_t)(x - y);
  res &= (uint8_t)(-(res <= x));
  return res;
}

static inline uint16_t  //
wuffs_base__u16__sat_add(uint16_t x, uint16_t y) {
  uint16_t res = (uint16_t)(x + y);
  res |= (uint16_t)(-(res < x));
  return res;
}

static inline uint16_t  //
wuffs_base__u16__sat_sub(uint16_t x, uint16_t y) {
  uint16_t res = (uint16_t)(x - y);
  res &= (uint16_t)(-(res <= x));
  return res;
}

static inline uint32_t  //
wuffs_base__u32__sat_add(uint32_t x, uint32_t y) {
  uint32_t res = (uint32_t)(x + y);
  res |= (uint32_t)(-(res < x));
  return res;
}

static inline uint32_t  //
wuffs_base__u32__sat_sub(uint32_t x, uint32_t y) {
  uint32_t res = (uint32_t)(x - y);
  res &= (uint32_t)(-(res <= x));
  return res;
}

static inline uint64_t  //
wuffs_base__u64__sat_add(uint64_t x, uint64_t y) {
  uint64_t res = (uint64_t)(x + y);
  res |= (uint64_t)(-(res < x));
  return res;
}

static inline uint64_t  //
wuffs_base__u64__sat_sub(uint64_t x, uint64_t y) {
  uint64_t res = (uint64_t)(x - y);
  res &= (uint64_t)(-(res <= x));
  return res;
}

// --------

typedef struct wuffs_base__multiply_u64__output__struct {
  uint64_t lo;
  uint64_t hi;
} wuffs_base__multiply_u64__output;

// wuffs_base__multiply_u64 returns x*y as a 128-bit value.
//
// The maximum inclusive output hi_lo is 0xFFFFFFFFFFFFFFFE_0000000000000001.
static inline wuffs_base__multiply_u64__output  //
wuffs_base__multiply_u64(uint64_t x, uint64_t y) {
#if defined(__SIZEOF_INT128__)
  __uint128_t z = ((__uint128_t)x) * ((__uint128_t)y);
  wuffs_base__multiply_u64__output o;
  o.lo = ((uint64_t)(z));
  o.hi = ((uint64_t)(z >> 64));
  return o;
#else
  // TODO: consider using the _mul128 intrinsic if defined(_MSC_VER).
  uint64_t x0 = x & 0xFFFFFFFF;
  uint64_t x1 = x >> 32;
  uint64_t y0 = y & 0xFFFFFFFF;
  uint64_t y1 = y >> 32;
  uint64_t w0 = x0 * y0;
  uint64_t t = (x1 * y0) + (w0 >> 32);
  uint64_t w1 = t & 0xFFFFFFFF;
  uint64_t w2 = t >> 32;
  w1 += x0 * y1;
  wuffs_base__multiply_u64__output o;
  o.lo = x * y;
  o.hi = (x1 * y1) + w2 + (w1 >> 32);
  return o;
#endif
}

// --------

#if defined(__GNUC__) && (__SIZEOF_LONG__ == 8)

static inline uint32_t  //
wuffs_base__count_leading_zeroes_u64(uint64_t u) {
  return u ? ((uint32_t)(__builtin_clzl(u))) : 64u;
}

#else
// TODO: consider using the _BitScanReverse intrinsic if defined(_MSC_VER).

static inline uint32_t  //
wuffs_base__count_leading_zeroes_u64(uint64_t u) {
  if (u == 0) {
    return 64;
  }

  uint32_t n = 0;
  if ((u >> 32) == 0) {
    n |= 32;
    u <<= 32;
  }
  if ((u >> 48) == 0) {
    n |= 16;
    u <<= 16;
  }
  if ((u >> 56) == 0) {
    n |= 8;
    u <<= 8;
  }
  if ((u >> 60) == 0) {
    n |= 4;
    u <<= 4;
  }
  if ((u >> 62) == 0) {
    n |= 2;
    u <<= 2;
  }
  if ((u >> 63) == 0) {
    n |= 1;
    u <<= 1;
  }
  return n;
}

#endif  // defined(__GNUC__) && (__SIZEOF_LONG__ == 8)

// --------

#define wuffs_base__peek_u8be__no_bounds_check \
  wuffs_base__peek_u8__no_bounds_check
#define wuffs_base__peek_u8le__no_bounds_check \
  wuffs_base__peek_u8__no_bounds_check

static inline uint8_t  //
wuffs_base__peek_u8__no_bounds_check(const uint8_t* p) {
  return p[0];
}

static inline uint16_t  //
wuffs_base__peek_u16be__no_bounds_check(const uint8_t* p) {
  return (uint16_t)(((uint16_t)(p[0]) << 8) | ((uint16_t)(p[1]) << 0));
}

static inline uint16_t  //
wuffs_base__peek_u16le__no_bounds_check(const uint8_t* p) {
  return (uint16_t)(((uint16_t)(p[0]) << 0) | ((uint16_t)(p[1]) << 8));
}

static inline uint32_t  //
wuffs_base__peek_u24be__no_bounds_check(const uint8_t* p) {
  return ((uint32_t)(p[0]) << 16) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 0);
}

static inline uint32_t  //
wuffs_base__peek_u24le__no_bounds_check(const uint8_t* p) {
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16);
}

static inline uint32_t  //
wuffs_base__peek_u32be__no_bounds_check(const uint8_t* p) {
  return ((uint32_t)(p[0]) << 24) | ((uint32_t)(p[1]) << 16) |
         ((uint32_t)(p[2]) << 8) | ((uint32_t)(p[3]) << 0);
}

static inline uint32_t  //
wuffs_base__peek_u32le__no_bounds_check(const uint8_t* p) {
  return ((uint32_t)(p[0]) << 0) | ((uint32_t)(p[1]) << 8) |
         ((uint32_t)(p[2]) << 16) | ((uint32_t)(p[3]) << 24);
}

static inline uint64_t  //
wuffs_base__peek_u40be__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 32) | ((uint64_t)(p[1]) << 24) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 8) |
         ((uint64_t)(p[4]) << 0);
}

static inline uint64_t  //
wuffs_base__peek_u40le__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32);
}

static inline uint64_t  //
wuffs_base__peek_u48be__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 40) | ((uint64_t)(p[1]) << 32) |
         ((uint64_t)(p[2]) << 24) | ((uint64_t)(p[3]) << 16) |
         ((uint64_t)(p[4]) << 8) | ((uint64_t)(p[5]) << 0);
}

static inline uint64_t  //
wuffs_base__peek_u48le__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32) | ((uint64_t)(p[5]) << 40);
}

static inline uint64_t  //
wuffs_base__peek_u56be__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 48) | ((uint64_t)(p[1]) << 40) |
         ((uint64_t)(p[2]) << 32) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 16) | ((uint64_t)(p[5]) << 8) |
         ((uint64_t)(p[6]) << 0);
}

static inline uint64_t  //
wuffs_base__peek_u56le__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32) | ((uint64_t)(p[5]) << 40) |
         ((uint64_t)(p[6]) << 48);
}

static inline uint64_t  //
wuffs_base__peek_u64be__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 56) | ((uint64_t)(p[1]) << 48) |
         ((uint64_t)(p[2]) << 40) | ((uint64_t)(p[3]) << 32) |
         ((uint64_t)(p[4]) << 24) | ((uint64_t)(p[5]) << 16) |
         ((uint64_t)(p[6]) << 8) | ((uint64_t)(p[7]) << 0);
}

static inline uint64_t  //
wuffs_base__peek_u64le__no_bounds_check(const uint8_t* p) {
  return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) |
         ((uint64_t)(p[2]) << 16) | ((uint64_t)(p[3]) << 24) |
         ((uint64_t)(p[4]) << 32) | ((uint64_t)(p[5]) << 40) |
         ((uint64_t)(p[6]) << 48) | ((uint64_t)(p[7]) << 56);
}

// --------

#define wuffs_base__poke_u8be__no_bounds_check \
  wuffs_base__poke_u8__no_bounds_check
#define wuffs_base__poke_u8le__no_bounds_check \
  wuffs_base__poke_u8__no_bounds_check

static inline void  //
wuffs_base__poke_u8__no_bounds_check(uint8_t* p, uint8_t x) {
  p[0] = x;
}

static inline void  //
wuffs_base__poke_u16be__no_bounds_check(uint8_t* p, uint16_t x) {
  p[0] = (uint8_t)(x >> 8);
  p[1] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__poke_u16le__no_bounds_check(uint8_t* p, uint16_t x) {
#if defined(__GNUC__) && !defined(__clang__) && defined(__x86_64__)
  // This seems to perform better on gcc 10 (but not clang 9). Clang also
  // defines "__GNUC__".
  memcpy(p, &x, 2);
#else
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
#endif
}

static inline void  //
wuffs_base__poke_u24be__no_bounds_check(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 16);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__poke_u24le__no_bounds_check(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
}

static inline void  //
wuffs_base__poke_u32be__no_bounds_check(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x >> 24);
  p[1] = (uint8_t)(x >> 16);
  p[2] = (uint8_t)(x >> 8);
  p[3] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__poke_u32le__no_bounds_check(uint8_t* p, uint32_t x) {
#if defined(__GNUC__) && !defined(__clang__) && defined(__x86_64__)
  // This seems to perform better on gcc 10 (but not clang 9). Clang also
  // defines "__GNUC__".
  memcpy(p, &x, 4);
#else
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
#endif
}

static inline void  //
wuffs_base__poke_u40be__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 32);
  p[1] = (uint8_t)(x >> 24);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 8);
  p[4] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__poke_u40le__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
}

static inline void  //
wuffs_base__poke_u48be__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 40);
  p[1] = (uint8_t)(x >> 32);
  p[2] = (uint8_t)(x >> 24);
  p[3] = (uint8_t)(x >> 16);
  p[4] = (uint8_t)(x >> 8);
  p[5] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__poke_u48le__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
  p[5] = (uint8_t)(x >> 40);
}

static inline void  //
wuffs_base__poke_u56be__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 48);
  p[1] = (uint8_t)(x >> 40);
  p[2] = (uint8_t)(x >> 32);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 16);
  p[5] = (uint8_t)(x >> 8);
  p[6] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__poke_u56le__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
  p[5] = (uint8_t)(x >> 40);
  p[6] = (uint8_t)(x >> 48);
}

static inline void  //
wuffs_base__poke_u64be__no_bounds_check(uint8_t* p, uint64_t x) {
  p[0] = (uint8_t)(x >> 56);
  p[1] = (uint8_t)(x >> 48);
  p[2] = (uint8_t)(x >> 40);
  p[3] = (uint8_t)(x >> 32);
  p[4] = (uint8_t)(x >> 24);
  p[5] = (uint8_t)(x >> 16);
  p[6] = (uint8_t)(x >> 8);
  p[7] = (uint8_t)(x >> 0);
}

static inline void  //
wuffs_base__poke_u64le__no_bounds_check(uint8_t* p, uint64_t x) {
#if defined(__GNUC__) && !defined(__clang__) && defined(__x86_64__)
  // This seems to perform better on gcc 10 (but not clang 9). Clang also
  // defines "__GNUC__".
  memcpy(p, &x, 8);
#else
  p[0] = (uint8_t)(x >> 0);
  p[1] = (uint8_t)(x >> 8);
  p[2] = (uint8_t)(x >> 16);
  p[3] = (uint8_t)(x >> 24);
  p[4] = (uint8_t)(x >> 32);
  p[5] = (uint8_t)(x >> 40);
  p[6] = (uint8_t)(x >> 48);
  p[7] = (uint8_t)(x >> 56);
#endif
}

// --------

// Load and Store functions are deprecated. Use Peek and Poke instead.

#define wuffs_base__load_u8__no_bounds_check \
  wuffs_base__peek_u8__no_bounds_check
#define wuffs_base__load_u16be__no_bounds_check \
  wuffs_base__peek_u16be__no_bounds_check
#define wuffs_base__load_u16le__no_bounds_check \
  wuffs_base__peek_u16le__no_bounds_check
#define wuffs_base__load_u24be__no_bounds_check \
  wuffs_base__peek_u24be__no_bounds_check
#define wuffs_base__load_u24le__no_bounds_check \
  wuffs_base__peek_u24le__no_bounds_check
#define wuffs_base__load_u32be__no_bounds_check \
  wuffs_base__peek_u32be__no_bounds_check
#define wuffs_base__load_u32le__no_bounds_check \
  wuffs_base__peek_u32le__no_bounds_check
#define wuffs_base__load_u40be__no_bounds_check \
  wuffs_base__peek_u40be__no_bounds_check
#define wuffs_base__load_u40le__no_bounds_check \
  wuffs_base__peek_u40le__no_bounds_check
#define wuffs_base__load_u48be__no_bounds_check \
  wuffs_base__peek_u48be__no_bounds_check
#define wuffs_base__load_u48le__no_bounds_check \
  wuffs_base__peek_u48le__no_bounds_check
#define wuffs_base__load_u56be__no_bounds_check \
  wuffs_base__peek_u56be__no_bounds_check
#define wuffs_base__load_u56le__no_bounds_check \
  wuffs_base__peek_u56le__no_bounds_check
#define wuffs_base__load_u64be__no_bounds_check \
  wuffs_base__peek_u64be__no_bounds_check
#define wuffs_base__load_u64le__no_bounds_check \
  wuffs_base__peek_u64le__no_bounds_check

#define wuffs_base__store_u8__no_bounds_check \
  wuffs_base__poke_u8__no_bounds_check
#define wuffs_base__store_u16be__no_bounds_check \
  wuffs_base__poke_u16be__no_bounds_check
#define wuffs_base__store_u16le__no_bounds_check \
  wuffs_base__poke_u16le__no_bounds_check
#define wuffs_base__store_u24be__no_bounds_check \
  wuffs_base__poke_u24be__no_bounds_check
#define wuffs_base__store_u24le__no_bounds_check \
  wuffs_base__poke_u24le__no_bounds_check
#define wuffs_base__store_u32be__no_bounds_check \
  wuffs_base__poke_u32be__no_bounds_check
#define wuffs_base__store_u32le__no_bounds_check \
  wuffs_base__poke_u32le__no_bounds_check
#define wuffs_base__store_u40be__no_bounds_check \
  wuffs_base__poke_u40be__no_bounds_check
#define wuffs_base__store_u40le__no_bounds_check \
  wuffs_base__poke_u40le__no_bounds_check
#define wuffs_base__store_u48be__no_bounds_check \
  wuffs_base__poke_u48be__no_bounds_check
#define wuffs_base__store_u48le__no_bounds_check \
  wuffs_base__poke_u48le__no_bounds_check
#define wuffs_base__store_u56be__no_bounds_check \
  wuffs_base__poke_u56be__no_bounds_check
#define wuffs_base__store_u56le__no_bounds_check \
  wuffs_base__poke_u56le__no_bounds_check
#define wuffs_base__store_u64be__no_bounds_check \
  wuffs_base__poke_u64be__no_bounds_check
#define wuffs_base__store_u64le__no_bounds_check \
  wuffs_base__poke_u64le__no_bounds_check

// ---------------- Slices and Tables

// WUFFS_BASE__SLICE is a 1-dimensional buffer.
//
// len measures a number of elements, not necessarily a size in bytes.
//
// A value with all fields NULL or zero is a valid, empty slice.
#define WUFFS_BASE__SLICE(T) \
  struct {                   \
    T* ptr;                  \
    size_t len;              \
  }

// WUFFS_BASE__TABLE is a 2-dimensional buffer.
//
// width height, and stride measure a number of elements, not necessarily a
// size in bytes.
//
// A value with all fields NULL or zero is a valid, empty table.
#define WUFFS_BASE__TABLE(T) \
  struct {                   \
    T* ptr;                  \
    size_t width;            \
    size_t height;           \
    size_t stride;           \
  }

typedef WUFFS_BASE__SLICE(uint8_t) wuffs_base__slice_u8;
typedef WUFFS_BASE__SLICE(uint16_t) wuffs_base__slice_u16;
typedef WUFFS_BASE__SLICE(uint32_t) wuffs_base__slice_u32;
typedef WUFFS_BASE__SLICE(uint64_t) wuffs_base__slice_u64;

typedef WUFFS_BASE__TABLE(uint8_t) wuffs_base__table_u8;
typedef WUFFS_BASE__TABLE(uint16_t) wuffs_base__table_u16;
typedef WUFFS_BASE__TABLE(uint32_t) wuffs_base__table_u32;
typedef WUFFS_BASE__TABLE(uint64_t) wuffs_base__table_u64;

static inline wuffs_base__slice_u8  //
wuffs_base__make_slice_u8(uint8_t* ptr, size_t len) {
  wuffs_base__slice_u8 ret;
  ret.ptr = ptr;
  ret.len = len;
  return ret;
}

static inline wuffs_base__slice_u16  //
wuffs_base__make_slice_u16(uint16_t* ptr, size_t len) {
  wuffs_base__slice_u16 ret;
  ret.ptr = ptr;
  ret.len = len;
  return ret;
}

static inline wuffs_base__slice_u32  //
wuffs_base__make_slice_u32(uint32_t* ptr, size_t len) {
  wuffs_base__slice_u32 ret;
  ret.ptr = ptr;
  ret.len = len;
  return ret;
}

static inline wuffs_base__slice_u64  //
wuffs_base__make_slice_u64(uint64_t* ptr, size_t len) {
  wuffs_base__slice_u64 ret;
  ret.ptr = ptr;
  ret.len = len;
  return ret;
}

static inline wuffs_base__slice_u8  //
wuffs_base__empty_slice_u8() {
  wuffs_base__slice_u8 ret;
  ret.ptr = NULL;
  ret.len = 0;
  return ret;
}

static inline wuffs_base__slice_u16  //
wuffs_base__empty_slice_u16() {
  wuffs_base__slice_u16 ret;
  ret.ptr = NULL;
  ret.len = 0;
  return ret;
}

static inline wuffs_base__slice_u32  //
wuffs_base__empty_slice_u32() {
  wuffs_base__slice_u32 ret;
  ret.ptr = NULL;
  ret.len = 0;
  return ret;
}

static inline wuffs_base__slice_u64  //
wuffs_base__empty_slice_u64() {
  wuffs_base__slice_u64 ret;
  ret.ptr = NULL;
  ret.len = 0;
  return ret;
}

static inline wuffs_base__table_u8  //
wuffs_base__empty_table_u8() {
  wuffs_base__table_u8 ret;
  ret.ptr = NULL;
  ret.width = 0;
  ret.height = 0;
  ret.stride = 0;
  return ret;
}

static inline wuffs_base__table_u16  //
wuffs_base__empty_table_u16() {
  wuffs_base__table_u16 ret;
  ret.ptr = NULL;
  ret.width = 0;
  ret.height = 0;
  ret.stride = 0;
  return ret;
}

static inline wuffs_base__table_u32  //
wuffs_base__empty_table_u32() {
  wuffs_base__table_u32 ret;
  ret.ptr = NULL;
  ret.width = 0;
  ret.height = 0;
  ret.stride = 0;
  return ret;
}

static inline wuffs_base__table_u64  //
wuffs_base__empty_table_u64() {
  wuffs_base__table_u64 ret;
  ret.ptr = NULL;
  ret.width = 0;
  ret.height = 0;
  ret.stride = 0;
  return ret;
}

static inline bool  //
wuffs_base__slice_u8__overlaps(wuffs_base__slice_u8 s, wuffs_base__slice_u8 t) {
  return ((s.ptr <= t.ptr) && (t.ptr < (s.ptr + s.len))) ||
         ((t.ptr <= s.ptr) && (s.ptr < (t.ptr + t.len)));
}

// wuffs_base__slice_u8__subslice_i returns s[i:].
//
// It returns an empty slice if i is out of bounds.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__subslice_i(wuffs_base__slice_u8 s, uint64_t i) {
  if ((i <= SIZE_MAX) && (i <= s.len)) {
    return wuffs_base__make_slice_u8(s.ptr + i, ((size_t)(s.len - i)));
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

// wuffs_base__slice_u8__subslice_j returns s[:j].
//
// It returns an empty slice if j is out of bounds.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__subslice_j(wuffs_base__slice_u8 s, uint64_t j) {
  if ((j <= SIZE_MAX) && (j <= s.len)) {
    return wuffs_base__make_slice_u8(s.ptr, ((size_t)j));
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

// wuffs_base__slice_u8__subslice_ij returns s[i:j].
//
// It returns an empty slice if i or j is out of bounds.
static inline wuffs_base__slice_u8  //
wuffs_base__slice_u8__subslice_ij(wuffs_base__slice_u8 s,
                                  uint64_t i,
                                  uint64_t j) {
  if ((i <= j) && (j <= SIZE_MAX) && (j <= s.len)) {
    return wuffs_base__make_slice_u8(s.ptr + i, ((size_t)(j - i)));
  }
  return wuffs_base__make_slice_u8(NULL, 0);
}

// ---------------- Magic Numbers

// wuffs_base__magic_number_guess_fourcc guesses the file format of some data,
// given its opening bytes. It returns a positive FourCC value on success.
//
// It returns zero if nothing matches its hard-coded list of 'magic numbers'.
//
// It returns a negative value if a longer prefix is required for a conclusive
// result. For example, seeing a single 'B' byte is not enough to discriminate
// the BMP and BPG image file formats.
//
// It does not do a full validity check. Like any guess made from a short
// prefix of the data, it may return false positives. Data that starts with 99
// bytes of valid JPEG followed by corruption or truncation is an invalid JPEG
// image overall, but this function will still return WUFFS_BASE__FOURCC__JPEG.
//
// Another source of false positives is that some 'magic numbers' are valid
// ASCII data. A file starting with "GIF87a and GIF89a are the two versions of
// GIF" will match GIF's 'magic number' even if it's plain text, not an image.
//
// For modular builds that divide the base module into sub-modules, using this
// function requires the WUFFS_CONFIG__MODULE__BASE__MAGIC sub-module, not just
// WUFFS_CONFIG__MODULE__BASE__CORE.
WUFFS_BASE__MAYBE_STATIC int32_t  //
wuffs_base__magic_number_guess_fourcc(wuffs_base__slice_u8 prefix);
