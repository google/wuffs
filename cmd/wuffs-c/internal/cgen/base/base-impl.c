// After editing this file, run "go generate" in the parent directory.

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

// !! INSERT base-public.h.

#ifdef WUFFS_IMPLEMENTATION

// !! INSERT base-private.h.

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__BASE)

// !! INSERT wuffs_base__status__string data.

const char*  //
wuffs_base__status__string(int32_t status_code) {
  uint16_t o = wuffs_base__status__string_offsets[(uint8_t)(status_code >> 24)];
  return o ? wuffs_base__status__string_data + o : "unknown status";
}

#endif  // !defined(WUFFS_CONFIG__MODULES) ||
        // defined(WUFFS_CONFIG__MODULE__BASE)

#endif  // WUFFS_IMPLEMENTATION
