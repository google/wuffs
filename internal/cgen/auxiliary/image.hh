// After editing this file, run "go generate" in the ../data directory.

// Copyright 2021 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// ---------------- Auxiliary - Image

namespace wuffs_aux {

struct DecodeImageResult {
  DecodeImageResult(MemOwner&& pixbuf_mem_owner0,
                    wuffs_base__slice_u8 pixbuf_mem_slice0,
                    wuffs_base__pixel_buffer pixbuf0,
                    std::string&& error_message0);
  DecodeImageResult(std::string&& error_message0);

  MemOwner pixbuf_mem_owner;
  wuffs_base__slice_u8 pixbuf_mem_slice;
  wuffs_base__pixel_buffer pixbuf;
  std::string error_message;
};

// DecodeImageCallbacks are the callbacks given to DecodeImage. They are always
// called in this order:
//  1. SelectDecoder
//  2. SelectPixfmt
//  3. AllocPixbuf
//  4. AllocWorkbuf
//  5. Done
//
// It may return early - the third callback might not be invoked if the second
// one fails - but the final callback (Done) is always invoked.
class DecodeImageCallbacks {
 public:
  // AllocResult holds a memory allocation (the result of malloc or new, a
  // statically allocated pointer, etc), or an error message. The memory is
  // de-allocated when mem_owner goes out of scope and is destroyed.
  struct AllocResult {
    AllocResult(MemOwner&& mem_owner0, wuffs_base__slice_u8 mem_slice0);
    AllocResult(std::string&& error_message0);

    MemOwner mem_owner;
    wuffs_base__slice_u8 mem_slice;
    std::string error_message;
  };

  // SelectDecoder returns the image decoder for the input data's file format.
  // Returning a nullptr means failure (DecodeImage_UnsupportedImageFormat).
  //
  // Common formats will have a FourCC value in the range [1 ..= 0x7FFF_FFFF],
  // such as WUFFS_BASE__FOURCC__JPEG. A zero FourCC value means that the
  // caller is responsible for examining the opening bytes (a prefix) of the
  // input data. SelectDecoder implementations should not modify those bytes.
  //
  // SelectDecoder might be called more than once, since some image file
  // formats can wrap others. For example, a nominal BMP file can actually
  // contain a JPEG or a PNG.
  //
  // The default SelectDecoder accepts the FOURCC codes listed below. For
  // modular builds (i.e. when #define'ing WUFFS_CONFIG__MODULES), acceptance
  // of the ETC file format is optional (for each value of ETC) and depends on
  // the corresponding module to be enabled at compile time (i.e. #define'ing
  // WUFFS_CONFIG__MODULE__ETC).
  //  - WUFFS_BASE__FOURCC__BMP
  //  - WUFFS_BASE__FOURCC__GIF
  //  - WUFFS_BASE__FOURCC__NIE
  //  - WUFFS_BASE__FOURCC__PNG
  //  - WUFFS_BASE__FOURCC__WBMP
  virtual wuffs_base__image_decoder::unique_ptr  //
  SelectDecoder(uint32_t fourcc, wuffs_base__slice_u8 prefix);

  // SelectPixfmt returns the destination pixel format for AllocPixbuf. It
  // should return wuffs_base__make_pixel_format(etc) called with one of:
  //  - WUFFS_BASE__PIXEL_FORMAT__BGR_565
  //  - WUFFS_BASE__PIXEL_FORMAT__BGR
  //  - WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL
  //  - WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL
  // or return image_config.pixcfg.pixel_format(). The latter means to use the
  // image file's natural pixel format. For example, GIF images' natural pixel
  // format is an indexed one.
  //
  // Returning otherwise means failure (DecodeImage_UnsupportedPixelFormat).
  //
  // The default SelectPixfmt implementation returns
  // wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL)
  // which is 4 bytes per pixel (8 bits per channel × 4 channels).
  virtual wuffs_base__pixel_format  //
  SelectPixfmt(const wuffs_base__image_config& image_config);

  // AllocPixbuf allocates the pixel buffer.
  //
  // allow_uninitialized_memory will be true if a valid background_color was
  // passed to DecodeImage, since the pixel buffer's contents will be
  // overwritten with that color after AllocPixbuf returns.
  //
  // The default AllocPixbuf implementation allocates either uninitialized or
  // zeroed memory. Zeroed memory typically corresponds to filling with opaque
  // black or transparent black, depending on the pixel format.
  virtual AllocResult  //
  AllocPixbuf(const wuffs_base__image_config& image_config,
              bool allow_uninitialized_memory);

  // AllocWorkbuf allocates the work buffer. The allocated buffer's length
  // should be at least len_range.min_incl, but larger allocations (up to
  // len_range.max_incl) may have better performance (by using more memory).
  //
  // The default AllocWorkbuf implementation allocates len_range.max_incl bytes
  // of either uninitialized or zeroed memory.
  virtual AllocResult  //
  AllocWorkbuf(wuffs_base__range_ii_u64 len_range,
               bool allow_uninitialized_memory);

  // Done is always the last Callback method called by DecodeImage, whether or
  // not parsing the input encountered an error. Even when successful, trailing
  // data may remain in input and buffer.
  //
  // The image_decoder is the one returned by SelectDecoder (if SelectDecoder
  // was successful), or a no-op unique_ptr otherwise. Like any unique_ptr,
  // ownership moves to the Done implementation.
  //
  // Do not keep a reference to buffer or buffer.data.ptr after Done returns,
  // as DecodeImage may then de-allocate the backing array.
  //
  // The default Done implementation is a no-op, other than running the
  // image_decoder unique_ptr destructor.
  virtual void  //
  Done(DecodeImageResult& result,
       sync_io::Input& input,
       IOBuffer& buffer,
       wuffs_base__image_decoder::unique_ptr image_decoder);
};

extern const char DecodeImage_BufferIsTooShort[];
extern const char DecodeImage_MaxInclDimensionExceeded[];
extern const char DecodeImage_OutOfMemory[];
extern const char DecodeImage_UnexpectedEndOfFile[];
extern const char DecodeImage_UnsupportedImageFormat[];
extern const char DecodeImage_UnsupportedPixelBlend[];
extern const char DecodeImage_UnsupportedPixelConfiguration[];
extern const char DecodeImage_UnsupportedPixelFormat[];

// DecodeImage decodes the image data in input. A variety of image file formats
// can be decoded, depending on what callbacks.SelectDecoder returns.
//
// For animated formats, only the first frame is returned, since the API is
// simpler for synchronous I/O and having DecodeImage only return when
// completely done, but rendering animation often involves handling other
// events in between animation frames. To decode multiple frames of animated
// images, or for asynchronous I/O (e.g. when decoding an image streamed over
// the network), use Wuffs' lower level C API instead of its higher level,
// simplified C++ API (the wuffs_aux API).
//
// The DecodeImageResult's fields depend on whether decoding succeeded:
//  - On total success, the error_message is empty and pixbuf.pixcfg.is_valid()
//    is true.
//  - On partial success (e.g. the input file was truncated but we are still
//    able to decode some of the pixels), error_message is non-empty but
//    pixbuf.pixcfg.is_valid() is still true. It is up to the caller whether to
//    accept or reject partial success.
//  - On failure, the error_message is non_empty and pixbuf.pixcfg.is_valid()
//    is false.
//
// The callbacks allocate the pixel buffer memory and work buffer memory. On
// success, pixel buffer memory ownership is passed to the DecodeImage caller
// as the returned pixbuf_mem_owner. Regardless of success or failure, the work
// buffer memory is deleted.
//
// The pixel_blend (one of the constants listed below) determines how to
// composite the decoded image over the pixel buffer's original pixels (as
// returned by callbacks.AllocPixbuf):
//  - WUFFS_BASE__PIXEL_BLEND__SRC
//  - WUFFS_BASE__PIXEL_BLEND__SRC_OVER
//
// The background_color is used to fill the pixel buffer after
// callbacks.AllocPixbuf returns, if it is valid in the
// wuffs_base__color_u32_argb_premul__is_valid sense. The default value,
// 0x0000_0001, is not valid since its Blue channel value (0x01) is greater
// than its Alpha channel value (0x00). A valid background_color will typically
// be overwritten when pixel_blend is WUFFS_BASE__PIXEL_BLEND__SRC, but might
// still be visible on partial (not total) success or when pixel_blend is
// WUFFS_BASE__PIXEL_BLEND__SRC_OVER and the decoded image is not fully opaque.
//
// Decoding fails (with DecodeImage_MaxInclDimensionExceeded) if the image's
// width or height is greater than max_incl_dimension.
DecodeImageResult  //
DecodeImage(DecodeImageCallbacks& callbacks,
            sync_io::Input& input,
            wuffs_base__pixel_blend pixel_blend = WUFFS_BASE__PIXEL_BLEND__SRC,
            wuffs_base__color_u32_argb_premul background_color = 1,  // Invalid.
            uint32_t max_incl_dimension = 1048575);  // 0x000F_FFFF

}  // namespace wuffs_aux
