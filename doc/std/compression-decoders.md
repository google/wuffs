# Compression Decoders

Compression decoders read from one input stream (an `io_reader` called `src`)
and write to an output stream (an `io_writer` called `dst`). Wuffs'
implementations have one key method: `transform_io`. It incrementally
decompresses the source data.

This method is a [coroutine](/doc/note/coroutines.md), and does not require
either all of the input or all of the output to fit in a single contiguous
buffer. It can suspend with the `$short_read` or `$short_write`
[statuses](/doc/note/statuses.md) when the `src` buffer needs re-filling or the
`dst` buffer needs flushing. For an example, look at the
[example/zcat](/example/zcat/zcat.c) program, which uses fixed size buffers,
but reads arbitrarily long compressed input from stdin and writes arbitrarily
long decompressed output to stdout.


## Dictionaries

TODO: standardize the various dictionary APIs, after Wuffs v0.2 is released.


## Implementations

- [std/deflate](/std/deflate)
- [std/gzip](/std/gzip)
- [std/lzw](/std/lzw)
- [std/zlib](/std/zlib)


## Examples

- [example/library](/example/library)
- [example/zcat](/example/zcat)


## Related Documentation

- [I/O (Input / Output)](/doc/note/io-input-output.md)

See also the general remarks on [Wuffs' standard library](/doc/std/README.md).
