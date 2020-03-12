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

Each quirk is assigned a `uint32_t` value, packed using the [base38 namespace
convention](/doc/note/base38-and-fourcc.md). Decoders and encoders can have a
`set_quirk_enabled!(quirk base.u32, enabled base.bool)` method whose first
argument is this `uint32_t` value.

For example, the base38 encoding of `"gif "` and `"json"` is `0x0F8586` and
`0x124265` respectively, so that the GIF-specific quirks have a `uint32_t`
value of `((0x0F8586 << 10) | g)`, and the JSON-specific quirks have a
`uint32_t` value of `((0x124265 << 10) | j)`, for some small integers `g` and
`j`. The high bits are a namespace. The overall quirk values are different even
if `g` and `j` re-use the same 10-bit integer.

The generated `C` language file defines human-readable names for those constant
values, such as `WUFFS_JSON__QUIRK_ALLOW_LEADING_UNICODE_BYTE_ORDER_MARK`.


## Listing

- [GIF image decoder quirks](/std/gif/decode_quirks.wuffs)
- [JSON decoder quirks](/std/json/decode_quirks.wuffs)
