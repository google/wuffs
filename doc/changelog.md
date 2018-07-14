# Changelog


## Work In Progress

- Renamed Puffs to Wuffs.
- Ship as a "single file C library"
- Added a `skipgendeps` flag.
- Added an `io_bind` keyword.
- Added a `use` keyword.
- Added a `yield` keyword.
- Added `std/adler32`, `std/crc32` and `std/gzip`.
- Spun `std/lzw` out of `std/gif`.
- Spun `std/zlib` out of `std/flate`.
- Let the `std/zlib` decoder ignore checksums.
- Renamed `std/flate` to `std/deflate`.
- Renamed `~+` to `~mod+`; added `~mod-`, `~sat+` and `~sat-`.
- Removed `&^`.
- Renamed `[N] T` and `[] T` types to `array[N] T` and `slice T`.
- Renamed `buf1`, `reader1`, etc to `io_buffer`, `io_reader`, etc.
- Renamed `u32`, `io_reader`, etc to `base.u32`, `base.io_reader`, etc.
- Renamed `unread_u8?` to `undo_byte!`; added `can_undo_byte`.
- Redesigned iterate blocks.
- Added a `base.image_config` type.
- Added a `reset` method.
- Added `peek_uxx`, `skip_fast` and `write_fast_uxx` methods.
- Tweaked how marks and limits work.
- Supported animated (not just single frame) GIFs.
- Marked the `std/gif` LZW decoder as private.
- Marked some internal status codes as private.
- Changed the string messages for built-in status codes.
- Made a packageid declaration mandatory.
- Disallowed certain packageid's like `base` or `config`.
- Disallowed `__double_underscore` prefixed names.
- Added a stand-alone lang/interval package.
- Added fuzz tests.
- Added `WUFFS_CONFIG__MODULES`.
- Added `WUFFS_CONFIG__STATIC_FUNCTIONS`.
- Added some C++ convenience methods.
- Added some Go and Rust benchmarks.
- Sped up the `mimic_deflate_xxx` benchmarks.


## 2017-11-16

- [Initial open source
  release](https://groups.google.com/d/topic/puffslang/2z61mNTAMns/discussion),
  under the "Puffs" name.


---

Updated on May 2018.
