# Changelog


## Work In Progress

- Renamed Puffs to Wuffs.
- Ship as a "single file C library"
- Added a `skipgendeps` flag.
- Added a `nullptr` literal and `nptr T` type.
- Added an `io_bind` keyword.
- Added a `use` keyword.
- Added a `yield` keyword.
- Changed func out-type from struct to bare type.
- Renamed the implicit `in` variable to `args`.
- Added `std/adler32`, `std/crc32` and `std/gzip`.
- Spun `std/lzw` out of `std/gif`.
- Spun `std/zlib` out of `std/flate`.
- Let the `std/zlib` decoder ignore checksums.
- Renamed `std/flate` to `std/deflate`.
- Renamed `~+` to `~mod+`; added `~mod-`, `~sat+` and `~sat-`.
- Removed `&^`.
- Renamed `$(etc)` to `[etc]`.
- Renamed `[N] T` and `[] T` types to `array[N] T` and `slice T`.
- Renamed `buf1`, `reader1`, etc to `io_buffer`, `io_reader`, etc.
- Renamed `u32`, `io_reader`, etc to `base.u32`, `base.io_reader`, etc.
- Renamed `unread_u8?` to `undo_byte!`; added `can_undo_byte`.
- Redesigned `!` vs `?` effects.
- Redesigned iterate blocks.
- Added `{frame,image,pixel}_config` and `pixel_buffer` types.
- Added a `reset` method.
- Added `peek_uxx`, `skip_fast` and `write_fast_uxx` methods.
- Added I/O positions.
- Tweaked how marks and limits work.
- Supported animated (not just single frame) and interlaced GIFs.
- Marked the `std/gif` LZW decoder as private.
- Marked some internal status codes as private.
- Removed closed-for-read/write built-in status codes.
- Changed the string messages for built-in status codes.
- Changed `error "foo"` to `status "?foo"`.
- Added warnings as another status code category.
- Made the status type a `const char *`, not an `int32_t`.
- Disallowed `__double_underscore` prefixed names.
- Moved the base38 package from `lang/base38` to `lib/base38`.
- Added a stand-alone `lib/interval` package.
- Added fuzz tests.
- Added `WUFFS_CONFIG__MODULES`.
- Added `WUFFS_CONFIG__STATIC_FUNCTIONS`.
- Added some C++ convenience methods.
- Added some Go and Rust benchmarks.
- Made struct implementations private (opaque pointers).
- Sped up the `mimic_deflate_xxx` benchmarks.


## 2017-11-16

- [Initial open source
  release](https://groups.google.com/d/topic/puffslang/2z61mNTAMns/discussion),
  under the "Puffs" name.


---

Updated on May 2018.
