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

#ifndef WUFFS_INCLUDE_GUARD__BASE
#define WUFFS_INCLUDE_GUARD__BASE

#if defined(WUFFS_IMPLEMENTATION) && !defined(WUFFS_CONFIG__MODULES)
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#endif

// !! WUFFS MONOLITHIC RELEASE DISCARDS EVERYTHING ABOVE.

// !! INSERT base-public.h.

// WUFFS C HEADER ENDS HERE.
#ifdef WUFFS_IMPLEMENTATION

// !! INSERT base-private.h.

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__BASE)

// !! INSERT wuffs_base__status strings.

#endif  // !defined(WUFFS_CONFIG__MODULES) ||
        // defined(WUFFS_CONFIG__MODULE__BASE)

#endif  // WUFFS_IMPLEMENTATION

// !! WUFFS MONOLITHIC RELEASE DISCARDS EVERYTHING BELOW.

#endif  // WUFFS_INCLUDE_GUARD__BASE
