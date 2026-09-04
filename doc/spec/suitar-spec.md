# Simple Uncompressed Interchange Tape Archive: SUITAR

Status: Draft (as of September 2026). There is no compatibility guarantee yet.


## Overview

SUITAR is a common-denominator, trivial-to-parse interchange format for "the
result of decompressing a compressed file or unpacking an archive file", such
as ZIP, RAR, 7Z, TAR, GZIP, BZIP2, XZ, ZSTD, etc.

SUITAR is to archive files (including `foobar.dat.xz` compressed files as an
archive containing 1 file) as [Farbfeld](https://tools.suckless.org/farbfeld/)
or [NIE](./nie-spec.md) is to image files: an uncompressed, "designed for Unix
pipes" format that is trivial to read or write in a few hundred lines of code,
ideally in a memory-safe programming language. It's a format for what the
Chromium web browser's "Rule of 2" security advice calls
[https://chromium.googlesource.com/chromium/src/+/master/docs/security/rule-of-2.md#normalization](Normalization).


## Subset of TAR

SUITAR is a subset of the well-known and widely-used TAR (Tape Archive, with
GNU extensions) archive file format. Every valid SUITAR file is also a valid
TAR file. These files work with popular tools like `/usr/bin/tar` and with
popular TAR-reading libraries in a variety of programming languages.

SUITAR is a subset of "pure TAR", not of "TAR wrapped in GZIP", "TAR wrapped in
BZIP2", etc. A separate compression step wrapping SUITAR is feasible, just like
wrapping TAR, but is out of scope of this document.

Like TAR, SUITAR files are a sequence of independent entries (files or
directories). Independent means that, like JSON map keys, duplicate names are
valid, although some decoders may choose to reject them.

A file's entry does not need to be preceded by explicit entries for that file's
parent directories.


### Further Restrictions

Compared to plain TAR (and refer to [the GNU TAR
manual](https://ftp.gnu.org/old-gnu/Manuals/tar-1.12/html_node/tar_123.html)),
SUITAR has further restrictions:

- Entries are either regular files (`REGTYPE`), sparse files (`GNUTYPE_SPARSE`)
  or directories (`DIRTYPE`). There is no support for hard links, symlinks,
  device files or other non-standard files.
- Sparse files must be completely sparse. Their content must be one contiguous
  span of NUL (zero) bytes that covers the entire file.
- File and directory names must obey the "File Name Validity" rules, below.
- These names must be encoded by a `GNUTYPE_LONGNAME` header block, regardless
  of whether the name's length is over or under 100 or 255 bytes.
- File size and modTime (modification time, seconds since Unix epoch) integers
  must use base-256 encoding (not base-8 octal) and must be non-negative and
  less than `(1 << 53)`, which is `9007_199254_740992`.
- Mode bits are either 0o644 (`rw-r--r--`) or 0o755 (`rwxr-xr-x`), encoded in
  base-8 octal (not base-256).
- UID and GID are hard-coded to 65534 (as a number, equivalent to octal
  0o0177776) and "nobody" (as a string).
- Any other fields are unused and must be NUL bytes.
- Padding bytes (as TAR uses 512-byte blocks) must be NUL bytes.

There is no support for various TAR variants, such as "the PAX extensions to
TAR" or "the USTAR extensions to TAR", other than what's implied by the subset
of the GNU extensions that SUITAR explicitly uses.

Encoders have no meaningful choices, bar one exception. There is only one valid
SUITAR encoding (unlike full TAR's backwards-compatible choice between base-8
or base-256 encoding of various sufficiently small numbers) for any given file
or directory entry (its combination of type, name, size, mode, modTime and
contents).

The one exception is that, if a file's contents are all NUL bytes (including
zero-sized files), an encoder can choose between a `REGTYPE` regular file (with
explicit NULs) or a `GNUTYPE_SPARSE` sparse file (with implicit NULs).


### File Name Validity

These rules apply to both file names and directory names.

- Names must be valid UTF-8.
- Names must not contain any ASCII control characters, including `'\n'` or
  `'\x00'`, the "new line" or NUL bytes.
- Names must not contain the `'\x7F'` ASCII DEL byte.
- Names must be no longer than 4095 bytes, excluding a trailing NUL.
- Names must not be `""`, `"."` or `".."`.
- Names must not start with `"/"`, `"./"` or `"../"`.
- Names must not end with `"/"`, `"/."` or `"/.."`.
- Names must not contain `"//"`, `"/./"` or `"/../"` as substrings.

For example, when converting from ZIP (with Japanese file names) to SUITAR, it
is the SUITAR producer's responsibility, not the SUITAR consumer's, to detect
and transform Shift-JIS encoded names to equivalent and valid UTF-8.


## File Structure

SUITAR files are a sequence of entries, followed by a 1024-byte "End Of File"
marker. The EOF marker's bytes are all NUL. Each entry occupies an integer
number of 512-byte blocks:

- 1 `GNUTYPE_LONGNAME` header block.
- 1 or more payload blocks containing the file or directory name.
- 1 `REGTYPE`, `GNUTYPE_SPARSE` or `DIRTYPE` header block.
- If `REGTYPE`, 0 or more payload blocks containing the file contents.
- If not `REGTYPE`, no further blocks.


### Header Blocks

Like all blocks, each header block is 512 bytes long. Each header block also
starts with a 12-byte magic signature (that is not valid UTF-8), identifying
SUITAR version 1. There are no other versions at this time.

The first 384 out of 512 bytes must match this template (arranged as 24 rows of
16 bytes per row, plus commentary):

    @@@@@@@@@@@@....   @@@@@@@@@@@@ = "\x13sUItAR\x00\xFE\xFDv1".
    ................
    ................
    ................
    ................
    ................
    ....0000???.0177   ??? = mode.
    776.0177776.$...
    ????????$...????   ???????? = physical size, ???????? = modTime.
    ??????????. ?...   ?????? = checksum, ? = type.
    ................
    ................
    ................
    ................
    ................
    ................
    .ustar  .nobody.
    ................
    .........nobody.
    ................
    ................
    ................
    ................
    ................

In this template, `@` indicates the magic signature, `.` indicates a `0x00` NUL
byte, `$` indicates a `0x80` byte and `?` indicates parts of the template that
are variable, not hard-coded.

These `?` bytes are the 3-byte mode (`"644"` or `"755"`), physical size or
modTime as an 8-byte big-endian `uint64`, 6-byte checksum (see below) or 1-byte
type, which must be one of:

- `'0'` for `REGTYPE`.
- `'5'` for `DIRTYPE`, in which case mode must be `"755"` and physical size
  must be all zeroes.
- `'L'` for `GNUTYPE_LONGNAME`, in which case mode must be `"644"` and modTime
  must be all zeroes.
- `'S'` for `GNUTYPE_SPARSE`, in which case physical size must be all zeroes
  and offset and logical size (see below) must be the same number and, again,
  within the half-open range `0 .. (1 << 53)`.

The last 128 out of 512 bytes (8 rows of 16 bytes per row) must be all NUL
bytes unless the type is `GNUTYPE_SPARSE`, in which case it must match this
template (and `?` again indicates an 8-byte big-endian `uint64`):

    ..$...????????$.   ???????? = sparse offset.
    ................
    ................
    ................
    ................
    ................
    ...$...????????.   ???????? = sparse logical size.
    ................


### Header Checksum

A 512-byte header block's checksum value is simply the sum of each byte (after
converting from `uint8` to `uint32`, to avoid overflow) in the block, at
offsets in the two half-open ranges `0 .. 148` and `156 .. 512`, which excludes
the 8 bytes for the 6-byte checksum itself plus another two hard-coded bytes
`"\x00\x20"`.

That checksum value is written as a 6-byte ASCII octal number in the header.
For example, `4853` (decimal) would be encoded as `"011365"` (octal).


### Payload Blocks

Each entry has one or more payload blocks, between its two header blocks,
containing the file or directory name. The name length (including a trailing
NUL byte) is the first header block's physical size value, and must be within
the half-open range `2 .. 4096`, and so the excluding-a-trailing-NUL length
must range within `1 .. 4095`. Rounding up that including-a-trailing-NUL length
to a multiple of 512 gives the number of 512-byte payload blocks that contain
the name. All padding bytes in the name's final payload block must be NUL.

For `REGTYPE` entries, the second header block's physical size value gives the
reconstructed file's size and rounding that up to a multiple of 512 gives the
number of 512-byte payload blocks that contain the file contents. Again, all
padding bytes in the contents' final payload block must be NUL.

For other entries (`DIRTYPE` or `GNUTYPE_SPARSE`), there are no further payload
blocks after the second header block.

For `GNUTYPE_SPARSE` entries, the second header block's logical size value
gives the reconstructed file's size and its contents are all NUL bytes.


# Reference Implementation

The [google/wuffs](https://github.com/google/wuffs) repository, which holds
this specification document, also holds a
[suitar](https://godoc.org/github.com/google/wuffs/lib/suitar) Go package and
some `test/data/*.suitar` example files, readable by that Go package but also
by `/usr/bin/tar`.


---

Updated on September 2026.
