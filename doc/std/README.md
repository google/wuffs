# Standard Library

Wuffs' standard library consists of multiple packages, each implementing a
particular file format or algorithm. Those packages can be grouped into several
categories:

- [Compression Decoders](/doc/std/compression-decoders.md).
- [Hashers](/doc/std/hashers.md).
- [Image Decoders](/doc/std/image-decoders.md).

The general pattern is that a package `foo` (e.g. `jpeg`, `png`) contains a
struct `bar` (e.g. `hasher`, `decoder`, etc) that implements a
package-independent interface. For example, every compression decoder struct
would have a `decode_io_writer` method. In C, this would be invoked as

```
// Allocate and initialize the struct.
wuffs_foo__decoder* dec = etc;

// Do the work. Error checking is not shown, for brevity.
const char* status = wuffs_foo__decoder__decode_io_writer(dec, etc);
```

When that C library is used as C++, that last line can be shortened:

```
const char* status = dec->decode_io_writer(etc);
```

See also the [glossary](/doc/glossary.md), as well as the notes on:

- [Coroutines](/doc/note/coroutines.md).
- [Initialization](/doc/note/initialization.md).
- [Statuses](/doc/note/statuses.md).
