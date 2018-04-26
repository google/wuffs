# Changelog


## Work In Progress

- Renamed Puffs to Wuffs.
- Added a `skipgendeps` flag.
- Added a `use` keyword.
- Added a `yield` keyword.
- Added `std/crc32` and `std/gzip`.
- Spun `std/zlib` out of `std/flate`.
- Let the `std/zlib` decoder ignore checksums.
- Renamed `std/flate` to `std/deflate`.
- Renamed `~+` to `~mod+`; added `~mod-`, `~sat+` and `~sat-`.
- Renamed `[N] T` and `[] T` types to `array[N] T` and `slice T`.
- Renamed `iterate.N` to `iterate[N]`.
- Renamed `buf1`, `reader1`, etc to `io_buffer`, `io_reader`, etc.
- Renamed `u32`, `io_reader`, etc to `base.u32`, `base.io_reader`, etc.
- Added a `base.image_config` type.
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

Updated on April 2018.
