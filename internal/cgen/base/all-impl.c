// After editing this file, run "go generate" in the parent directory.

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

#ifndef WUFFS_INCLUDE_GUARD__BASE
#define WUFFS_INCLUDE_GUARD__BASE

#if defined(WUFFS_IMPLEMENTATION) && !defined(WUFFS_CONFIG__MODULES)
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#endif

// !! WUFFS MONOLITHIC RELEASE DISCARDS EVERYTHING ABOVE.

// !! INSERT base/copyright

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// GCC does not warn for unused *static inline* functions, but clang does.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// !! INSERT base/all-public.h.

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// WUFFS C HEADER ENDS HERE.
#ifdef WUFFS_IMPLEMENTATION

// GCC does not warn for unused *static inline* functions, but clang does.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// !! INSERT base/all-private.h.

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__BASE)

const uint8_t wuffs_base__low_bits_mask__u8[9] = {
    0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF,
};

const uint16_t wuffs_base__low_bits_mask__u16[17] = {
    0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF,
    0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
};

const uint32_t wuffs_base__low_bits_mask__u32[33] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000F, 0x0000001F,
    0x0000003F, 0x0000007F, 0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
    0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF, 0x0001FFFF,
    0x0003FFFF, 0x0007FFFF, 0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
    0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF, 0x1FFFFFFF,
    0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

const uint64_t wuffs_base__low_bits_mask__u64[65] = {
    0x0000000000000000, 0x0000000000000001, 0x0000000000000003,
    0x0000000000000007, 0x000000000000000F, 0x000000000000001F,
    0x000000000000003F, 0x000000000000007F, 0x00000000000000FF,
    0x00000000000001FF, 0x00000000000003FF, 0x00000000000007FF,
    0x0000000000000FFF, 0x0000000000001FFF, 0x0000000000003FFF,
    0x0000000000007FFF, 0x000000000000FFFF, 0x000000000001FFFF,
    0x000000000003FFFF, 0x000000000007FFFF, 0x00000000000FFFFF,
    0x00000000001FFFFF, 0x00000000003FFFFF, 0x00000000007FFFFF,
    0x0000000000FFFFFF, 0x0000000001FFFFFF, 0x0000000003FFFFFF,
    0x0000000007FFFFFF, 0x000000000FFFFFFF, 0x000000001FFFFFFF,
    0x000000003FFFFFFF, 0x000000007FFFFFFF, 0x00000000FFFFFFFF,
    0x00000001FFFFFFFF, 0x00000003FFFFFFFF, 0x00000007FFFFFFFF,
    0x0000000FFFFFFFFF, 0x0000001FFFFFFFFF, 0x0000003FFFFFFFFF,
    0x0000007FFFFFFFFF, 0x000000FFFFFFFFFF, 0x000001FFFFFFFFFF,
    0x000003FFFFFFFFFF, 0x000007FFFFFFFFFF, 0x00000FFFFFFFFFFF,
    0x00001FFFFFFFFFFF, 0x00003FFFFFFFFFFF, 0x00007FFFFFFFFFFF,
    0x0000FFFFFFFFFFFF, 0x0001FFFFFFFFFFFF, 0x0003FFFFFFFFFFFF,
    0x0007FFFFFFFFFFFF, 0x000FFFFFFFFFFFFF, 0x001FFFFFFFFFFFFF,
    0x003FFFFFFFFFFFFF, 0x007FFFFFFFFFFFFF, 0x00FFFFFFFFFFFFFF,
    0x01FFFFFFFFFFFFFF, 0x03FFFFFFFFFFFFFF, 0x07FFFFFFFFFFFFFF,
    0x0FFFFFFFFFFFFFFF, 0x1FFFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFFFF,
    0x7FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF,
};

// !! INSERT wuffs_base__status strings.

// !! INSERT base/image-impl.c.

#endif  // !defined(WUFFS_CONFIG__MODULES) ||
        // defined(WUFFS_CONFIG__MODULE__BASE)

#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif  // WUFFS_IMPLEMENTATION

// !! WUFFS MONOLITHIC RELEASE DISCARDS EVERYTHING BELOW.

#endif  // WUFFS_INCLUDE_GUARD__BASE
