# Changelog


## Work In Progress

The headline feature is that the GIF decoder is now of production quality.
There is now API for overall metadata (e.g. ICCP color profiles) and to
recreate each frame (width, height, BGRA pixels, timing, etc.) of a GIF
animation, instead of version 0.1's proof-of-concept GIF decoder API, which
just gave you a one-dimensional stream of palette indexes. It also now accepts
a variety of GIF images that are invalid, when strictly following the GIF
specifiction, but are nonetheless accepted by other real world GIF
implementations. The Wuffs GIF decoder has also been optimized to be about 1.5x
faster than Wuffs version 0.1 and about 2x faster than giflib (the C library).

- Renamed Puffs to Wuffs.
- Ship as a "single file C library"
- Added a `skipgendeps` flag.
- Added a `nullptr` literal and `nptr T` type.
- Added `io_bind` and `io_limit` keywords.
- Added a `use` keyword.
- Added a `yield` keyword.
- Made the `return` value mandatory; added `ok` literal.
- Restricted `var` statements to the top of functions.
- Dropped the `= RHS` out of `var x T = RHS`.
- Changed func out-type from struct to bare type.
- Renamed the implicit `in` variable to `args`.
- Added an implicit `coroutine_resumed` variable.
- Added `std/adler32`, `std/crc32` and `std/gzip`.
- Added `std/gif` quirks.
- Spun `std/lzw` out of `std/gif`.
- Spun `std/zlib` out of `std/flate`.
- Let the `std/gzip` and `std/zlib` decoder ignore checksums.
- Renamed `std/flate` to `std/deflate`.
- Renamed `!=` to `<>`; `!` is now only for impure functions.
- Renamed `~+` to `~mod+`; added `~mod-`, `~sat+` and `~sat-`.
- Removed `&^`.
- Renamed `$(etc)` to `[etc]`.
- Renamed `[i..j]` to `[i ..= j]`, consistent with Rust syntax.
- Renamed `[i:j]` to `[i .. j]`, consistent with Rust syntax.
- Renamed `x T` to `x: T`, consistent with Rust syntax.
- Renamed `[N] T` and `[] T` types to `array[N] T` and `slice T`.
- Renamed `while:label` to `while.label`.
- Renamed `u32`, `buf1`, etc to `base.u32`, `base.io_buffer`, etc.
- Renamed `unread_u8?` to `undo_byte!`; added `can_undo_byte`.
- Renamed some `decode?` methods to `decode_io_writer?`.
- Replaced `= try foo` with `=? foo`.
- Prohibited effect-ful subexpressions.
- Redesigned iterate blocks.
- Added `{frame,image,pixel}_config` and `pixel_buffer` types.
- Added a `reset` method.
- Added `peek_uxx`, `skip_fast` and `write_fast_uxx` methods.
- Renamed some `read_uxx` methods as `read_uxx_as_uyy`.
- Report image metadata such as ICCP and XMP.
- Added I/O positions.
- Tweaked how I/O marks and limits work.
- Tweaked `io_buffer` / `io_reader` distinction in C and Wuffs.
- Added extra fields (uninitialized internal buffers) to structs.
- Supported animated (not just single frame) and interlaced GIFs.
- Marked some internal status codes as private.
- Removed closed-for-read/write built-in status codes.
- Changed the string messages for built-in status codes.
- Changed `error "foo"` to `"#foo"` or `base."#bar"`.
- Added warnings as another status code category.
- Made the status type a `const char *`, not an `int32_t`.
- Disallowed `__double_underscore` prefixed names.
- Added fuzz tests.
- Added `WUFFS_CONFIG__MODULES`.
- Added `WUFFS_CONFIG__STATIC_FUNCTIONS`.
- Added some C++ convenience methods.
- Added some Go and Rust benchmarks.
- Made struct implementations private (opaque pointers).
- Sped up the `mimic_deflate_xxx` benchmarks.
- Fix compile errors under MSVC (Microsoft Visual C/C++).
- Moved the base38 package from `lang/base38` to `lib/base38`.
- Added a stand-alone `lib/interval` package.
- Added NIE file format spec.
- Added RAC file format spec and Go implementation.


## 2017-11-16

- [Initial open source
  release](https://groups.google.com/d/topic/puffslang/2z61mNTAMns/discussion),
  under the "Puffs" name.


---

Updated on August 2019.
