# Image Decoders

Regardless of the file format (GIF, JPEG, PNG, etc), every Wuffs image decoder
has a similar API. This document gives `wuffs_gif__decoder__foo_bar` examples,
but the `gif` could be replaced by `jpeg`, `png`, etc.

Some image formats (GIF) are animated, consisting of an image header and then N
frames. The image header gives e.g. the overall image's width and height. Each
frame consists of a frame header (e.g. frame rectangle bounds, display
duration) and a payload (the pixels).

In general, there is one `wuffs_base__image_config` and then N pairs of
(`wuffs_base__frame_config`, frame). Non-animated (still) image formats are
treated the same way: that their N is 1 and their single frame's bounds equals
the overall image bounds.

To decode everything (without knowing N in advance) sequentially:

```
wuffs_base__io_buffer src = etc;
wuffs_gif__decoder* dec = etc;

wuffs_base__image_config ic;
wuffs_base__status status = wuffs_gif__decoder__decode_image_config(dec, &ic, &src);
// Error checking (inspecting the status variable) is not shown, for brevity.
// See the example programs, listed below, for how to handle errors.

// Allocate the pixel buffer, based on ic's width and height, etc.
wuffs_base__pixel_buffer pb = etc;

while (true) {
  wuffs_base__frame_config fc;
  status = wuffs_gif__decoder__decode_frame_config(dec, &fc, &src);
  if (status.repr == wuffs_base__note__end_of_data) {
    break;
  }
  // Ditto re error checking.

  status = wuffs_gif__decoder__decode_frame(dec, &pb, &etc);
  // Ditto re error checking.
}
```

The second argument to each `decode_xxx` method (`&ic`, `&fc` or `&pb`), is the
destination data structure to store the decoded information.

For random (instead of sequential) access to an image's frames, call
`wuffs_gif__decoder__restart_frame(dec, i, io_pos)` to prepare to decode the
i'th frame. Essentially, it restores the state to be at the top of the while
loop above. The `wuffs_base__io_buffer`'s reader position will also need to be
set to the `io_pos` position in the source data stream. The position for the
i'th frame is calculated by the i'th `decode_frame_config` call and saved in
the `frame_config`. You can only call `restart_frame` after
`decode_image_config` is called, explicitly or implicitly (see below), as
decoding a single frame might require for-all-frames information like the
overall image dimensions and the global palette.

All of those `decode_xxx` calls are optional. For example, if
`decode_image_config` is not called, then the first `decode_frame_config` call
will implicitly parse and verify the image header, before parsing the first
frame's header. Similarly, you can call only `decode_frame` N times, without
calling `decode_image_config` or `decode_frame_config`, if you already know
metadata like N and each frame's rectangle bounds by some other means (e.g.
this is a first party, statically known image).

Specifically, starting with an unknown (but re-windable) animated image, if you
want to just find N (i.e. count the number of frames), you can loop calling
only the `decode_frame_config` method and avoid calling the more expensive
`decode_frame` method. For GIF, in terms of the underlying wire format, this
will skip over (instead of decompress) the LZW-encoded pixel data, which is
faster.

Those `decode_xxx` methods are also suspendible
[coroutines](/doc/note/coroutines.md). They will return early (with a status
code that `is_suspendible` and therefore isn't `is_complete`) if there isn't
enough source data to complete the operation: an incremental decode. Calling
`decode_xxx` again with additional source data will resume the previous
operation instead of starting a new one. Calling `decode_yyy` whilst
`decode_xxx` is suspended will result in an error.

Once an error is encountered, whether from invalid source data or from a
programming error, such as calling `decode_yyy` while suspended in
`decode_xxx`, all subsequent calls will be no-ops that return an error. To
reset the decoder into something that does productive work,
[re-initialize](/doc/note/initialization.md) and then, in order to be able to
call `restart_frame`, call `decode_image_config`. The `io_buffer` and its
associated stream will also need to be rewound.


## Metadata

Images can also contain metadata (e.g. color profiles, time stamps). By
default, Wuffs' image decoders skip over metadata, but calling
`set_report_metadata` will opt in to having `decode_image_config` return early
when encountering metadata in the file. Calling `set_report_metadata` can be
done multiple times, each with a different
[FourCC](/doc/note/base38-and-fourcc.md) code such as `0x49434350` "ICCP" or
`0x584D5020` "XMP ", to indicate what sorts of metadata the caller is
interested in. Conversely, when the parser encounters metadata (and returns a
"@metadata reported" [status](/doc/note/statuses.md)), call `tell_me_more` to
see what sort of metadata it is.

Some metadata is short and `tell_me_more` will also fill in its `minfo: nptr
more_information` out parameter with "parsed metadata". For example, Gamma
Correction metadata is a single numerical value and Primary Chromaticities
metadata is only eight numbers. From C++ code, call the `metadata_parsed__gama`
or `metadata_parsed__chrm` methods to retrieve these value.

Some metadata is long (or at least variable length) and needs processing by a
separate parser. For example, processing XMP metadata usually involves some
sort of XML parser, regardless of what particular image format that XMP
metadata was embedded in. Wuffs decoders will return "raw metadata" instead of
parsing it themselves. Raw metadata is further sub-divided into two categories:
"raw passthrough" and "raw transform". Either way, note that raw metadata might
also be arranged in multiple (non-contiguous) chunks of the source data.

"Raw passthrough" means that the bytes in the wire format can be sent "as is"
to the separate parser. Call `tell_me_more` in a loop (the `dst: io_writer`
argument will be ignored) and `metadata_raw_passthrough__range` to identify a
range (a chunk) of the input stream. Divert the bytes in that range to the
separate parser (which should advance the `io_buffer` to the end of that range)
and repeat the loop as long as `tell_me_more` returned a "$even more
information" status.

"Raw transform" means that the bytes in the wire format have to be further
processed (e.g. decompressed) before sending them on to the separate parser.
That processing is done by the `image_decoder` callee, not the caller. Pass a
writable `dst io_buffer` to `tell_me_more` (where writable means that it has a
positive `writer_length`) and implementations should fill in that buffer with
post-transformed (e.g. decompressed) data to send onwards. Like any
`io_transformer`-like coroutine, this should be in a loop, as it may suspend
with "$short read" and "$short write" statuses.

Either way, break the loop (after handling the `dst` and `minfo` out
parameters) when `tell_me_more` returns a NULL status, meaning ok, when the
metadata is complete. Afterwards, call the original action (e.g.
`decode_image_config`) again to resume after the "@metadata reported" detour.


## API Listing

In Wuffs syntax, the `base.image_decoder` methods are:

- `decode_frame?(dst: ptr pixel_buffer, src: io_reader, blend: pixel_blend, workbuf: slice u8, opts: nptr decode_frame_options)`
- `decode_frame_config?(dst: nptr frame_config, src: io_reader)`
- `decode_image_config?(dst: nptr image_config, src: io_reader)`
- `frame_dirty_rect() rect_ie_u32`
- `num_animation_loops() u32`
- `num_decoded_frame_configs() u64`
- `num_decoded_frames() u64`
- `restart_frame!(index: u64, io_position: u64) status`
- `set_quirk_enabled!(quirk: u32, enabled: bool)`
- `set_report_metadata!(fourcc: u32, report: bool)`
- `tell_me_more?(dst: io_writer, minfo: nptr more_information, src: io_reader)`
- `workbuf_len() range_ii_u64`


## Implementations

- [std/bmp](/std/bmp)
- [std/gif](/std/gif)
- [std/nie](/std/nie)
- [std/png](/std/png)
- [std/tga](/std/tga)
- [std/wbmp](/std/wbmp)


## Examples

- [example/convert-to-nia](/example/convert-to-nia)
- [example/gifplayer](/example/gifplayer)
- [example/imageviewer](/example/imageviewer)
- [example/sdl-imageviewer](/example/sdl-imageviewer)

Examples in other repositories:

- [Skia](https://skia.googlesource.com/skia/+/refs/heads/master/src/codec/SkWuffsCodec.cpp),
  a 2-D graphics library used by the Android operating system and the Chromium
  web browser.


## [Quirks](/doc/note/quirks.md)

- [GIF decoder quirks](/std/gif/decode_quirks.wuffs)


## Related Documentation

- [I/O (Input / Output)](/doc/note/io-input-output.md)
- [Pixel Formats](/doc/note/pixel-formats.md)
- [Pixel Subsampling](/doc/note/pixel-subsampling.md)
- [Ranges and Rects](/doc/note/ranges-and-rects.md)

See also the general remarks on [Wuffs' standard library](/doc/std/README.md).
