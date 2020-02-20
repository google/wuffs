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

// ---------------- String Conversions

// wuffs_base__parse_number_u64 parses the ASCII integer in s. For example, if
// s contains the bytes "123" then it will return the uint64_t 123.
//
// It returns an error if s does not contain an integer or if the integer
// within would overflow a uint64_t.
//
// It is similar to the C standard library's strtoull function, but:
//  - Errors are returned in-band (in a result type), not out-of-band (errno).
//  - It takes a slice (a pointer and length), not a NUL-terminated C string.
//  - It does not take an optional endptr argument. It does not allow a partial
//    parse: it returns an error unless all of s is consumed.
//  - It does not allow whitespace, leading or otherwise.
//  - It does not allow a leading '+' or '-'.
//  - It does not allow multiple leading zeroes: "0" is valid but "00" is not.
//  - It does not take a base argument (e.g. base 10 vs base 16). Instead, it
//    always accepts both decimal (e.g "1234", "0d5678") and hexadecimal (e.g.
//    "0x9aBC"). The caller is responsible for prior filtering of e.g. hex
//    numbers if they are unwanted. For example, Wuffs' JSON decoder will only
//    produce a wuffs_base__token for decimal numbers, not hexadecimal.
//  - It is not affected by i18n / l10n settings.
//  - It does allow arbitrary underscores, except inside the optional 2-byte
//    opening "0d" or "0X" that denotes base-10 or base-16. For example,
//    "__0D_1_002" would successfully parse as "one thousand and two".
wuffs_base__result_u64  //
wuffs_base__parse_number_u64(wuffs_base__slice_u8 s);
