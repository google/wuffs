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

Wuffs, out of the box, makes particular choices (typically mimicking the de
facto canonical implementation), but enabling various quirks result in
different choices. In particular, quirks are useful in regression testing that
Wuffs and another implementation produce the same output for the same input,
even for malformed input.


## Wuffs API

Each quirk is assigned a `uint32_t` value, packed using the [base38 namespace
convention](/doc/note/base38-and-fourcc.md). Decoders and encoders can have a
`set_quirk_enabled!(quirk base.u32, enabled base.bool)` method whose first
argument is this `uint32_t` value.

For example, the base38 encoding of `"gif "` is `0xF8586`, so that the
GIF-specific quirks have a `uint32_t` value of `((0xF8586 << 10) | n)`, for
some small integer `n`. The generated `C` language file defines human-readable
names for those constant values.
