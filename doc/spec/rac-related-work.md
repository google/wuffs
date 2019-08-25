# Random Access Compression (RAC) Related Work

This is a companion document to the [Random Access Compression (RAC)
Specification](./rac-spec.md). Its contents aren't necessary for implementing
the RAC file format, but gives further context on what RAC is and is not.

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
