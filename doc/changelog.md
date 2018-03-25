# Changelog


## Work In Progress

- Renamed Puffs to Wuffs.
- Added a `skipgendeps` flag.
- Added a `use` keyword.
- Added a `yield` keyword.
- Added an image\_config built-in concept.
- Added `std/crc32`.
- Spun `std/zlib` out of `std/flate`.
- Let the `std/zlib` decoder ignore checksums.
- Renamed `std/flate` to `std/deflate`.
- Supported animated (not just single frame) GIFs.
- Marked the `std/gif` LZW decoder as private.
- Marked some internal status codes as private.
- Changed the string messages for built-in status codes.
- Made a packageid declaration mandatory.
- Disallowed certain packageid's like `base` or `config`.
- Disallowed `__double_underscore` prefixed names.
- Added a stand-alone lang/interval package.
- Added fuzz tests.
- Added some Go and Rust benchmarks.
- Sped up the mimic\_deflate\_xxx benchmarks.


## 2017-11-16

- [Initial open source
  release](https://groups.google.com/d/topic/puffslang/2z61mNTAMns/discussion),
  under the "Puffs" name.


---

Updated on March 2018.
