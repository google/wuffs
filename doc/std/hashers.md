# Hashers

Hashers take a stream of bytes (of arbitrary length) and produce a fixed length
value - the hash. For example, the CRC-32/IEEE hashing algorithm produces a 32
bit value (a `base.u32`). The MD5 hashing algorithm produces a 128 bit value.

Wuffs' hasher implementations have only one method. For 32 bit hashes, the
method signature is `update_u32!(x: slice base.u8) base.u32`. It incrementally
updates the hasher object's state with the addition data `x`, and returns the
hash value so far, for all of the data up to and including `x`.

This method is stateful. Calling `update_u32` twice with the same slice of
bytes can produce two different hash values. Conversely, calling `update_u32`
twice with two different slices should be equivalent to calling it once on
their concatenation. [Re-initialize](/doc/note/initialization.md) the object to
reset the state.

Wuffs' hasher implementations are not cryptographic. They make no attempt to
resist timing attacks.


## Implementations

- [std/adler32](/std/adler32)
- [std/crc32](/std/crc32)


## Examples

- [example/crc32](/example/crc32)


## Related Documentation

- [I/O (Input / Output)](/doc/note/io-input-output.md)

See also the general remarks on [Wuffs' standard library](/doc/std/README.md).
