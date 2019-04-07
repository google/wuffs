# Random Access Compression: RAC

Status: Draft (as of March 2019). There is no compatibility guarantee yet.


## Overview

The goal of the RAC file format is to compress a source file (the "decompressed
file") such that, starting from the compressed file, it is possible to
reconstruct the half-open byte range `[di .. dj)` of the decompressed file
without always having to first decompress all of `[0 .. di)`.

Conceptually, the decompressed file is partitioned into non-overlapping chunks.
Each compressed chunk can be decompressed independently (although possibly
sharing additional context, such as a LZ77 prefix dictionary). A RAC file also
contains a hierarchical index of those chunks.

RAC is a container format, and while it supports common compression codecs like
Zlib, Brotli and Zstandard, it is not tied to any particular compression codec.

Non-goals for version 1 include:

  - Filesystem metadata like file names, file sizes and modification times.
  - Multiple source files.

There is the capability (see reserved `TTag`s, below) but no promise to address
these in a future RAC version. There might not be a need to, as other designs
such as EROFS ([Extendable Read-Only File
System](https://lkml.org/lkml/2018/5/31/306)) already exist.

Non-goals in general include:

  - Encryption.
  - Streaming decompression.


### Related Work

In general, RAC differs from most other archive or compression formats in a
number of ways. This doesn't mean that RAC is necessarily better or worse than
these other designs, just that different designs make different trade-offs. For
example, supporting shared dictionaries means giving up streaming (one pass)
decoding.

  - RAC files must start with a magic sequence, but RAC still supports
    append-only modifications.
  - RAC seek points are identified by numbers (i.e. `DOffset`s), not strings
    (i.e. file names). Some other formats' seek points are positioned only at
    source file boundaries. They cannot seek to a relative offset (in what RAC
    calls `DSpace`) within a source file.
  - RAC supports [shared compression
    dictionaries](http://fastcompression.blogspot.com/2018/02/when-to-use-dictionary-compression.html)
    embedded within an archive.

These points apply to established archive formats like
[Tar](https://en.wikipedia.org/wiki/Tar_%28computing%29) and
[Zip](https://en.wikipedia.org/wiki/Zip_%28file_format%29), and newer archive
formats like [Śiva](https://blog.sourced.tech/post/siva/).

[Riegeli](https://github.com/google/riegeli) is a sequence-of-records format. A
RAC file arguably contains what Riegeli calls records (which are not files, as
they don't have names) and both formats support numerical seek points, and
multiple compression codecs, but Riegeli seeks to a point in what RAC calls
`CSpace`, whereas RAC seeks to a point in `DSpace`.

[XFLATE](https://github.com/dsnet/compress/blob/master/doc/xflate-format.pdf)
supports random access like RAC, but is tied to the DEFLATE family of
compression codecs.

[XZ](https://tukaani.org/xz/format.html) supports random access like RAC, but
as one of its explicit goals is to support streaming decoding, it does not
support shared dictionaries. Also, unlike RAC, finding the record that contains
any given `DOffset` requires a linear scan of all previous records, even if
those records don't need decompressing.

[QCOW](https://github.com/qemu/qemu/blob/master/docs/interop/qcow2.txt)
supports random access like RAC, but compression does not seem to be the
foremost concern. That specification mentions compression but does not give any
particular codec. A [separate
document](https://people.gnome.org/~markmc/qcow-image-format.html) suggests
that the codec is Zlib (with no other option), and that it does not support
shared dictionaries.

The
[`examples/zran.c`](https://github.com/madler/zlib/blob/master/examples/zran.c)
program in the zlib-the-library source code repository builds an in-memory
index, not a persistent (disk-backed) one, and building the index involves
first decompressing the whole file. It also requires storing (in memory) the
entire state of the decompressor, including 32 KiB of history, per seek point.
For coarse grained seek points (e.g. once every 1 MiB), that overhead can be
acceptable, but for fine grained seek points (e.g. once every 16 KiB), that
overhead is prohibitive.

RAC differs from the [LevelDB
Table](https://github.com/google/leveldb/blob/master/doc/table_format.md)
format, even if the LevelDB Table string keys are re-purposed to encode both
numerical `DOffset` seek points and identifiers for shared compression
dictionaries. A LevelDB Table index is flat, unlike RAC's hierarchical index.
In both cases, a maliciously written index could contain multiple entries for
the same key, and different decoders could therefore produce different output
for the same source file - a potential security vulnerability. For LevelDB
Tables, verifying an index key's uniqueness requires scanning every key in the
file, which can be relatively expensive, and is therefore not done in practice.
In comparison, RAC's "Branch Node Validation" process (see below) ensures that
exactly one `Leaf Node` contains any given byte offset `di` in the decompressed
file, and the RAC index's hierarchical nature places a scalable upper bound on
the scanning cost of verifying that, based on that portion of the index tree
visited for any given request `[di .. dj)`.


### Glossary

  - `CBias` is the delta added to a `CPointer` to produce a `COffset`.
  - `DBias` is the delta added to a `DPointer` to produce a `DOffset`.
  - `CFile` is the compressed file.
  - `DFile` is the decompressed file.
  - `CFileSize` is the size of the `CFile`.
  - `DFileSize` is the size of the `DFile`.
  - `COffset` is a byte offset in `CSpace`.
  - `DOffset` is a byte offset in `DSpace`.
  - `CPointer` is a relative `COffset`, prior to bias-correction.
  - `DPointer` is a relative `DOffset`, prior to bias-correction.
  - `CRange` is a `Range` in `CSpace`.
  - `DRange` is a `Range` in `DSpace`.
  - `CSpace` means that byte offsets refer to the `CFile`.
  - `DSpace` means that byte offsets refer to the `DFile`.

`Range` is a pair of byte offsets `[i .. j)`, in either `CSpace` or `DSpace`.
It is half-open, containing every byte offset `x` such that `(i <= x)` and `(x
< j)`. It is invalid to have `(i > j)`. The size of a `Range` equals `(j - i)`.

All bytes are 8 bits and unless explicitly specified otherwise, all fixed-size
unsigned integers (e.g. `uint32_t`, `uint64_t`) are encoded little-endian.
Within those unsigned integers, bit 0 is the least significant bit and e.g. bit
31 is the most significant bit of a `uint32_t`.

The maximum supported `CFileSize` and the maximum supported `DFileSize` are the
same number: `0x0000_FFFF_FFFF_FFFF`, which is `((1 << 48) - 1)`.


## File Structure

A RAC file (the `CFile`) must be at least 4 bytes long, and start with the 3
byte `Magic` (see below), so that no valid RAC file can also be e.g. a valid
JPEG file. The fourth byte is examined in the process described by the "Root
Node at the CFile Start" section, below.

The `CFile` contains a tree of `Node`s. Each `Node` is either a `Branch Node`
(pointing to between 1 and 255 child `Node`s) or a `Leaf Node`. There must be
at least one `Branch Node`, called the `Root Node`. Parsing a `CFile` requires
knowing the `CFileSize` in order to identify the `Root Node`, which is either
at the start or the end of the `CFile`.

Each `Node` has a `DRange`. An empty `DRange` means that the `Node` contains
metadata or other decompression context such as a shared dictionary.

Each `Leaf Node` also has 3 `CRange`s (`Primary`, `Secondary` and `Tertiary`),
any or all of which may be empty. The contents of the `CFile`, within those
`CRange`s, are decompressed according to the `Codec` (see below) to reconstruct
that part of the `DFile` within the `Leaf Node`'s `DRange`.


## Branch Nodes

A `Branch Node`'s encoding in the `CFile` has a variable byte size, between 32
and 4096 inclusive, depending on its number of children. Specifically, it
occupies `((Arity * 16) + 16)` bytes, grouped into 8 byte segments (but not
necessarily 8 byte aligned), starting at a `COffset` called its `Branch
COffset`:

    +-+-+-+-+-+-+-+-+
    |0|1|2|3|4|5|6|7|
    +-+-+-+-+-+-+-+-+
    |Magic|A|Che|0|T|  Magic, Arity, Checksum,     Reserved (0),  TTag[0]
    | DPtr[1]   |0|T|  DPtr[1],                    Reserved (0),  TTag[1]
    | DPtr[2]   |0|T|  DPtr[2],                    Reserved (0),  TTag[2]
    | ...       |0|.|  ...,                        Reserved (0),  ...
    | DPtr[A-2] |0|T|  DPtr[Arity-2],              Reserved (0),  TTag[Arity-2]
    | DPtr[A-1] |0|T|  DPtr[Arity-1],              Reserved (0),  TTag[Arity-1]
    | DPtr[A]   |0|C|  DPtr[Arity] a.k.a. DPtrMax, Reserved (0),  Codec
    | CPtr[0]   |L|S|  CPtr[0],                    CLen[0],       STag[0]
    | CPtr[1]   |L|S|  CPtr[1],                    CLen[1],       STag[1]
    | CPtr[2]   |L|S|  CPtr[2],                    CLen[2],       STag[2]
    | ...       |L|.|  ...,                        ...,           ...
    | CPtr[A-2] |L|S|  CPtr[Arity-2],              CLen[Arity-2], STag[Arity-2]
    | CPtr[A-1] |L|S|  CPtr[Arity-1],              CLen[Arity-2], STag[Arity-1]
    | CPtr[A]   |V|A|  CPtr[Arity] a.k.a. CPtrMax, Version,       Arity
    +-+-+-+-+-+-+-+-+

For the `(XPtr | Other6 | Other7)` 8 byte fields, the `XPtr` occupies the low
48 bits (as a little-endian `uint64_t`) and the `Other` fields occupy the high
16 bits.

The `CPtr` and `DPtr` values are what is explicitly written in the `CFile`'s
bytes. These are added to a `Branch Node`'s implicit `Branch CBias` and `Branch
DBias` values to give the implicit `COff` and `DOff` values: `COff[i]` and
`DOff[i]` are defined to be `(Branch_CBias + CPtr[i])` and `(Branch_DBias +
DPtr[i])`.

`CPtrMax` is another name for `CPtr[Arity]`, and `COffMax` is defined to be
`(Branch_CBias + CPtrMax)`. Likewise for `DPtrMax` and `DOffMax`.

The `DPtr[0]` value is implicit, and always equals zero, so that `DOff[0]`
always equals the `Branch DBias`.

  - For the `Root Node`, the `DPtrMax` also sets the `DFileSize`. The `Branch
    CBias` and `Branch DBias` are both zero. The `Branch COffset` is determined
    by the "Root Node" section below.
  - For a child `Branch Node`, the `Branch COffset`, `Branch CBias` and `Branch
    DBias` are given by the parent `Branch Node`. See the "Search Within a
    Branch Node" section below.


### Magic

`Magic` is the three bytes `"\x72\xC3\x63"`, which is invalid UTF-8 but is
`"rÃc"` in ISO 8859-1. The tilde isn't particularly meaningful, other than
`"rÃc"` being a nonsensical word (with nonsensical capitalization) that is
unlikely to appear in other files.

Every `Branch Node` must start with these `Magic` bytes, not just a `Branch
Node` positioned at the start of the `CFile`.


### Arity

`Arity` is the `Branch Node`'s number of children. Zero is invalid.

The `Arity` byte is given twice: the fourth byte and the final byte of the
`Branch Node`. The two values must match.

The repetition lets a RAC reader determine the size of the `Branch Node` data
(as the size depends on the `Arity`), given either its start or its end offset
in `CSpace`. For almost all `Branch Node`s, we will know its start offset (its
`Branch COffset`), but for a `Root Node` at the end of a `CFile`, we will only
know its end offset.


### Checksum

`Checksum` is a checksum of the `Branch Node`'s bytes. It is not a checksum of
the `CFile` or `DFile` contents pointed to by a `Branch Node`. Content
checksums are a `Codec`-specific consideration.

The little-endian `uint16_t` `Checksum` value is the low 16 bits XOR'ed with
the high 16 bits of the `uint32_t` CRC-32 IEEE checksum of the `((Arity * 16) +
10)` bytes immediately after the `Checksum`. The 4 bytes immediately before the
`Checksum` are not considered: the `Magic` bytes have only one valid value and
the `Arity` byte near the start is replicated by the `Arity` byte at the end.


### Reserved (0)

The `Reserved (0)` bytes must have the value `0x00`.


### COffs and DOffs, STags and TTags

For every `a` in the half-open range `[0 .. Arity)`, the `a`'th child `Node`
has two tags, `STag[a]` and `TTag[a]`, and a `DRange` of `[DOff[a] ..
DOff[a+1])`. The `DOff` values must be non-decreasing: see the "Branch Node
Validation" section below.

A `TTag[a]` of `0xFE` means that child is a `Branch Node`. A `TTag[a]` in the
half-open range `[0xC0 .. 0xFE)` is reserved. Otherwise, the child is a `Leaf
Node`.

A child `Branch Node`'s `SubBranch COffset` is defined to be `COff[a]`. Its
`SubBranch DBias` and `SubBranch DOffMax` are defined to be `DOff[a]` and
`DOff[a+1].

  - When `(STag[a] < Arity)`, it is a `CBiasing Branch Node`. The `SubBranch
    CBias` is defined to be `(Branch_CBias + CPtr[STag[a]])`. This expression
    is equivalent to `COff[STag[a]]`.
  - When `(STag[a] >= Arity)`, it is a `CNeutral Branch Node`. The `SubBranch
    CBias` is defined to be `(Branch_CBias)`.

A child `Leaf Node`'s `STag[a]` and `TTag[a]` values are also called its `Leaf
STag` and `Leaf TTag`. It also has:

  - A `Primary CRange`, equal to `MakeCRange(a)`.
  - A `Secondary CRange`, equal to `MakeCRange(STag[a])`.
  - A `Tertiary CRange`, equal to `MakeCRange(TTag[a])`.

The `MakeCRange(i)` function defines a `CRange`. If `(i >= Arity)` then that
`CRange` is the empty range `[COffMax .. COffMax)`. Otherwise, the lower bound
is `COff[i]` and the upper bound is:

  - `COffMax` when `CLen[i]` is zero.
  - The minimum of `COffMax` and `(COff[i] + (CLen[i] * 1024))` when `CLen[i]`
    is non-zero.

In other words, the `COffMax` value clamps the `CRange` upper bound. The `CLen`
value, if non-zero, combines with the `COff` value to apply another clamp. The
`CLen` is given in units of 1024 bytes, but the `(COff[i] + (CLen[i] * 1024))`
value is not necessarily quantized to 1024 byte boundaries.

Note that, since `Arity` is at most 255, an `STag[a]` of `0xFF` always results
in a `CNeutral Branch Node` or an empty `Secondary CRange`. Likewise, a
`TTag[a]` of `0xFF` always results in an empty `Tertiary CRange`.


### COffMax

`COffMax` is an inclusive upper bound on every `COff` in a `Branch Node` and in
its descendent `Branch Node`s. A child `Branch Node` must not have a larger
`COffMax` than the parent `Branch Node`'s `COffMax`, and the `Root Node`'s
`COffMax` must equal the `CFileSize`. See the "Branch Node Validation" section
below.

A RAC file can therefore be incrementally modified, if the RAC writer only
appends new `CFile` bytes and does not re-write existing `CFile` bytes, so that
the `CFileSize` increases. Even if the old (smaller) RAC file's `Root Node` was
at the `CFile` start, the new (larger) `CFileSize` means that those starting
bytes are an obsolete `Root Node` (but still a valid `Branch Node`). The new
`Root Node` is therefore located at the end of the new RAC file.

Concatenating RAC files (concatenating in `DSpace`) involves concatenating the
RAC files in `CSpace` and then appending a new `Root Node` with `CBiasing
Branch Node`s pointing to each source RAC file's `Root Node`.


### Version

`Version` must have the value `0x01`, indicating version 1 of the RAC format.


### Codec

`Codec`s define specializations of RAC, such as "RAC + Zlib" or "RAC + Brotli".
It is valid for a "RAC + Zstandard" only decoder to reject a "RAC + Brotli"
file, even if it is a valid RAC file. Recall that RAC is just a container, and
not tied to any particular compression codec. For the `Codec` byte in a `Branch
Node`:

  - `0x00` is reserved.
  - `0x01` means "RAC + Zlib".
  - `0x02` means "RAC + Brotli".
  - `0x04` means "RAC + ZStandard".
  - Any other value less than 0x08 means that all of this `Branch Node`'s
    children must be `Branch Node`s and not `Leaf Node`s and that no child's
    `Codec` byte can have a bit set that is not set in this `Codec` byte.
  - All other values, 0x08 or greater, are reserved.


### Branch Node Validation

The first time that a RAC reader visits any particular `Branch Node`, it must
check that the `Magic` matches, the two `Arity` values match and are non-zero,
the computed checksum matches the listed `Checksum` and that the RAC reader
accepts the `Version` and the `Codec`.

It must also check that all of its `DOff` values are sorted: `(DOff[a] <=
DOff[a+1])` for every `a` in the half-open range `[0 .. Arity)`. By induction,
this means that all of its `DOff` values do not exceed `DOffMax`, and again by
induction, therefore do not exceed `DFileSize`.

It must also check that all of its `COff` values do not exceed `COffMax` (and
again by induction, therefore do not exceed `CFileSize`). Other than that,
`COff` values do not have to be sorted: successive `Node`s (in `DSpace`) can be
out of order (in `CSpace`), allowing for incrementally modified RAC files.

For the `Root Node`, its `COffMax` must equal the `CFileSize`. Recall that
parsing a `CFile` requires knowing the `CFileSize`, and also that a `Root
Node`'s `Branch CBias` is zero, so its `COffMax` equals its `CPtrMax`.

For a child `Branch Node`, its `Codec` bits must be a subset of its parent's
`Codec` bits, its `COffMax` must be less than or equal to its parent's
`COffMax`, and its `DOffMax` must equal its parent's `SubBranch DOffMax`. The
`DOffMax` condition is equivalent to checking that the parent and child agree
on the child's size in `DSpace`. The parent states that it is its `(DPtr[a+1] -
DPtr[a])` and the child states that it is its `DPtrMax`.

One conservative way to check `Branch Node`s' validity on first visit is to
check them on every visit, as validating any particular `Branch Node` is
idempotent, but other ways are acceptable.


## Root Node

The `Root Node` might be at the start of the `CFile`, as this might optimize
alignment of `Branch Node`s and of `CRange`s. All `Branch Node`s' sizes are
multiples of 16 bytes, and a maximal `Branch Node` is exactly 4096 bytes.

The `Root Node` might be at the end of the `CFile`, as this allows one-pass
(streaming) encoding of a RAC file. It also allows appending to, concatenating
or incrementally modifying existing RAC files relatively cheaply.

To find the `Root Node`, first look for a valid `Root Node` at the `CFile`
start. If and only if that fails, look at the `CFile` end. If that also fails,
it is not a valid RAC file.


### Root Node at the CFile Start

The fourth byte of the `CFile` gives the `Arity`, assuming the `Root Node` is
at the `CFile` start. If it is zero, fail over to the `CFile` end. A RAC writer
that does not want to place the `Root Node` at the `CFile` start should
intentionally write a zero to the fourth byte.

Otherwise, that `Arity` defines the size in bytes of any starting `Root Node`:
`((Arity * 16) + 16)`. If the `CFileSize` is less than this size, fail over to
the `CFile` end.

If those starting bytes form a valid `Root Node` (as per the "Branch Node
Validation" section), including having a `CPtrMax` equal to the `CFileSize`,
then we have indeed found our `Root Node`: its `Branch COffset` is zero.
Otherwise, fail over to the `CFile` end.


### Root Node at the CFile End

If there is no valid `Root Node` at the `CFile` start then the last byte of the
`CFile` gives the `Root Node`'s `Arity`. This does not necessarily need to
match the fourth byte of the `CFile`.

As before, that `Arity` defines the size in bytes of any ending `Root Node`:
`((Arity * 16) + 16)`. If the `CFileSize` is less than this size, it is not a
valid RAC file.

If those ending bytes form a valid `Root Node` (as per the "Branch Node
Validation" section), including having a `CPtrMax` equal to the `CFileSize`,
then we have indeed found our `Root Node`: its `Branch COffset` is the
`CFileSize` minus the size implied by the `Arity`. Otherwise, it is not a valid
RAC file.


## DRange Reconstruction

To reconstruct the `DRange` `[di .. dj)`, if that `DRange` is empty then the
request is trivially satisfied.

Otherwise, if `(dj > DFileSize)` then reject the request.

Otherwise, start at the `Root Node` and continue to the next section to find
the `Leaf Node` containing the `DOffset` `di`.


### Search Within a Branch Node

Load (and validate) the `Branch Node` given its `Branch COffset`, `Branch
CBias` and `Branch DBias`.

Find the largest child index `a` such that `(a < Arity)` and `(DOff[a] <= di)`
and `(DOff[a+1] > di)`, then examine `TTag[a]` to see if the child is a `Leaf
Node`. If so, skip to the next section.

For a `Branch Node` child, let `CRemaining` equal this `Branch Node`'s (the
parents') `COffMax` minus the `SubBranch COffset`. It invalid for `CRemaining`
to be less than 4, or to be less than the child's size implied by the child's
`Arity` byte at a `COffset` equal to `(SubBranch_COffset + 3)`.

The `SubBranch COffset`, `SubBranch CBias` and `SubBranch DBias` from the
parent `Branch Node` become the `Branch COffset`, `Branch CBias` and `Branch
DBias` of the child `Branch Node`. In order to rule out infinite loops, at
least one of these two conditions must hold:

  - The child's `Branch COffset` is less than the parent's `Branch COffset`.
  - The child's `DPtrMax` is less than the parent's `DPtrMax`.

It is valid for one of those conditions to hold between a parent-child pair and
the other condition to hold between the next parent-child pair.

Repeat this "Search Within a Branch Node" section with the child `Branch Node`.


### Decompressing a Leaf Node

If a `Leaf Node`'s `DRange` is empty, decompression is a no-op and skip the
rest of this section.

Otherwise, decompression combines the `Primary CRange`, `Secondary CRange` and
`Tertiary CRange` slices of the `CFile`, and the `Leaf STag` and `Leaf TTag`
values, in a `Codec`-specific way to produce decompressed data.

There are two general principles, although specific `Codec`s can overrule:

  - The `Codec` may produce fewer bytes than the `DRange` size. In that case,
    the remaining bytes (in `DSpace`) are set to NUL (`memset` to zero).
  - The `Codec` may consume fewer bytes than each `CRange` size, and the
    compressed data typically contains an explicit "end of data" marker. In
    that case, the remaining bytes (in `CSpace`) are ignored. Padding allows
    `COff` values to optionally be aligned.

It is invalid to produce more bytes than the `DRange` size or to consume more
bytes than each `CRange` size.


### Continuing the Reconstruction

If decompressing that `Leaf Node` did not fully reconstruct `[di .. dj)`,
advance through the `Node` tree in depth first search order, decompressing
every `Leaf Node` along the way, until we have gone up to or beyond `dj`.

During that traversal, `Node`s with an empty `DRange` should be skipped, even
if they are `Branch Node`s.


# Specific Codecs


## RAC + Zlib

The `CFile` data in the `Leaf Node`'s `Primary CRange` is decompressed as Zlib
(RFC 1950), possibly referencing a LZ77 prefix dictionary (what the RFC calls a
"preset dictionary").

If a `Leaf Node`'s `Secondary CRange` is empty then there is no dictionary.
Otherwise, the `Secondary CRange` must be at least 6 bytes long:

  - 2 byte little-endian `uint16_t` `Dictionary Length`.
  - `Dictionary Length` bytes `Dictionary`.
  - 4 byte big-endian `uint32_t` `Dictionary Checksum`.
  - Padding (ignored).

The `Dictionary Checksum` is Zlib's Adler32 checksum over the `Dictionary`'s
bytes (excluding the initial 2 bytes for the `Dictionary Length`). The checksum
is stored big-endian, like Zlib's other checksums, and its 4 byte value must
match the `DICTID` (in RFC 1950 terminology) given in the `Primary CRange`'s
Zlib-formatted data.

The `Leaf TTag` must be `0xFF`. All other `Leaf TTag` values (below `0xC0`) are
reserved. The empty `Tertiary CRange` is ignored. The `Leaf STag` value is also
ignored, other than deriving the `Secondary CRange`.


## RAC + Brotli

TODO.


## RAC + Zstandard

TODO.


# Examples

TODO (zlib).


# Acknowledgements

I (Nigel Tao) thank Robert Obryk, Sanjay Ghemawat and Sean Klein for their
review.


---

Updated on April 2019.
