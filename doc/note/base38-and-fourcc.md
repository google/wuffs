# Base38 and FourCC Codes

Both of these encode a four-character string such as `"JPEG"` as a `uint32_t`
value. Computers can compare two integer values faster than they can compare
two arbitrary strings.

Both schemes maintain ordering: if two four-character strings `s` and `t`
satisfy `(s < t)`, and those strings have valid numerical encodings, then the
numerical values also satisfy `(encoding(s) < encoding(t))`.


## FourCC

FourCC codes are not specific to Wuffs. For example, the AVI multimedia
container format can hold various sub-formats, such as "H264" or "YV12",
distinguished in the overall file format by their FourCC code.

The FourCC encoding is the straightforward sequence of each character's ASCII
encoding. The FourCC code for `"JPEG"` is `0x4A504547`, since `'J'` is `0x4A`,
`'P'` is `0x50`, etc. This is essentially 8 bits for each character, 32 bits
overall. The big-endian representation of this number is exactly the ASCII (and
UTF-8) string `"JPEG"`.

Other FourCC documentation sometimes use a little-endian convention, so that
the `{0x4A, 0x50, 0x45, 0x47}` bytes on the wire for `"JPEG"` corresponds to
the number `0x4745504A` (little-endian) instead of `0x4A504547` (big-endian).
Wuffs uses the big-endian interpretation, as it maintains ordering.


## Base38

Base38 is a tighter encoding than FourCC, fitting four characters into 21 bits
instead of 32 bits. This is achieved by using a smaller alphabet of 38 possible
values (space, 0-9, ? or a-z), so that it cannot distinguish between e.g. an
upper case 'X' and a lower case 'x'. There's also the happy coincidence that
`38 ** 4` is slightly smaller than `2 ** 21`.

The base38 encoding of `"JPEG"` is `0x122FF6`, which is `1191926`, which is
`((21 * (38 ** 3)) + (27 * (38 ** 2)) + (16 * (38 ** 1)) + (18 * (38 ** 0)))`.

Using only 21 bits means that we can use base38 values to partition the set of
possible `uint32_t` values into file-format specific enumerations. Each package
(i.e. Wuffs implementation of a specific file format) can define up to 1024
different values in their own namespace, without conflicting with other
packages (assuming that there aren't e.g. two `"JPEG"` Wuffs packages in the
same library). The conventional `uint32_t` packing is:

- Bit        31 is reserved (zero).
- Bits 30 .. 10 are the base38 value, shifted by 10.
- Bits  9 ..  0 are the enumeration value.

For example, [quirk values](/doc/note/quirks.md) use this `((base38 << 10) |
enumeration)` scheme.
