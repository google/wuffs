# Quirks

Quirks are configuration options that let Wuffs more closely match other codec
implementations.

Some codecs and file formats are more tightly specified than others. When a
spec doesn't fully dictate how to treat malformed files, different codec
implementations have disagreed on whether to accept it (e.g. whether they
follow Postel's Principle of "be liberal in what you accept") and if so, how to
interpret it. Some implementations also raise those decisions to the
application level, not the library level: "provide mechanism, not policy".

For example, in an animated GIF image, the first frame might not fill out the
entire image, and different implementations choose a different default color
for the outside pixels: opaque black, transparent black, or something else.

Other file formats have unofficial extensions. For example, a strict reading of
the [JSON specification](https://json.org/) rejects an initial `"\x1E"` ASCII
Record Separator, `"// comments"` or commas after the final array element or
object key-value pair (e.g. `"[1,2,3,]"`), but
[some](https://www.ietf.org/rfc/rfc7464.txt) [extensions](https://json5.org) to
the format allow these and other changes.

Wuffs, out of the box, makes particular choices (typically mimicking the de
facto canonical implementation), but enabling various quirks results in
different choices. In particular, quirks are useful in regression testing that
Wuffs and another implementation produce the same output for the same input,
even for malformed input.


## Wuffs API

Each quirk is assigned a `uint32_t` number, packed using the [base38 namespace
convention](/doc/note/base38-and-fourcc.md). Decoders and encoders can have a
`set_quirk!(key: base.u32, value: base.u64) base.status` method whose key
argument is this `uint32_t` number.

For example, the base38 encoding of `"gif "` and `"json"` is `0x0F8586` and
`0x124265` respectively, so that the GIF-specific quirks have a `uint32_t` key
of `((0x0F8586 << 10) | g)` and the JSON-specific quirks have a `uint32_t` key
of `((0x124265 << 10) | j)`, for some small integers `g` and `j`. The high bits
are a namespace. The overall quirk keys are different even if `g` and `j`
re-use the same 10-bit integer.

The generated `C` language file defines human-readable names for those constant
numbers, such as `WUFFS_JSON__QUIRK_ALLOW_LEADING_UNICODE_BYTE_ORDER_MARK`.

The value argument (a `uint64_t` number) is often effectively a boolean: zero
means to disable and non-zero to enable the quirk. But `set_quirk` is sometimes
also used to pass out-of-band configuration, and not necessarily due to
underspecified file formats, such as `WUFFS_LZW__QUIRK_LITERAL_WIDTH_PLUS_ONE`.
For those cases, zero typically means to use the default configuration.


## Listing

Common quirks:

- `WUFFS_BASE__QUIRK_IGNORE_CHECKSUM` configures decoders (but not encoders) to
  skip checksum verification. This can result in [noticably faster
  decodes](https://github.com/google/wuffs/commit/170a8104867fa818d329d85921012c922577c955),
  at a cost of being less able to detect data corruption and to deviate from a
  strict reading of the relevant file format specifications, accepting some
  inputs that are technically invalid (but otherwise decode fine).

Package-specific quirks:

- [GIF image decoder quirks](/std/gif/decode_quirks.wuffs)
- [JSON decoder quirks](/std/json/decode_quirks.wuffs)
- [LZW decoder quirks](/std/lzw/decode_quirks.wuffs)
- [ZLIB decoder quirks](/std/zlib/decode_quirks.wuffs)
