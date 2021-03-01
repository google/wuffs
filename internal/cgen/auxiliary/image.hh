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
//  1. OnImageFormat
//  2. OnImageConfig
//  3. AllocWorkbuf
//  4. Done
//
// It may return early - the third callback might not be invoked if the second
// one fails (returns a non-empty error message) - but the final callback
// (Done) is always invoked.
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

  // OnImageFormat returns the image decoder for the input data's file format.
  // Returning a nullptr means failure (DecodeImage_UnsupportedImageFormat).
  //
  // Common formats will have a non-zero FourCC value, such as
  // WUFFS_BASE__FOURCC__JPEG. A zero FourCC code means that the caller is
  // responsible for examining the opening bytes (a prefix) of the input data.
  // OnImageFormat implementations should not modify those bytes.
  //
  // OnImageFormat might be called more than once, since some image file
  // formats can wrap others. For example, a nominal BMP file can actually
  // contain a JPEG or a PNG.
  //
  // There is no default implementation, as modular Wuffs builds (those that
  // define WUFFS_CONFIG__MODULES) may enable or disable particular codecs.
  virtual wuffs_base__image_decoder::unique_ptr  //
  OnImageFormat(uint32_t fourcc, wuffs_base__slice_u8 prefix) = 0;

  // OnImageConfig allocates the pixel buffer.
  //
  // The default OnImageConfig implementation allocates zeroed memory.
  virtual AllocResult  //
  OnImageConfig(const wuffs_base__image_config& image_config);

  // AllocWorkbuf allocates the work buffer. The allocated buffer's length
  // should be at least len.min_incl, but larger allocations (up to
  // len.max_incl) may have better performance (by using more memory).
  //
  // The default AllocWorkbuf implementation allocates len.max_incl bytes of
  // zeroed memory.
  virtual AllocResult  //
  AllocWorkbuf(wuffs_base__range_ii_u64 len);

  // Done is always the last Callback method called by DecodeImage, whether or
  // not parsing the input encountered an error. Even when successful, trailing
  // data may remain in input and buffer.
  //
  // Do not keep a reference to buffer or buffer.data.ptr after Done returns,
  // as DecodeImage may then de-allocate the backing array.
  //
  // The default Done implementation is a no-op.
  virtual void  //
  Done(DecodeImageResult& result, sync_io::Input& input, IOBuffer& buffer);
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
// can be decoded, depending on what callbacks.OnImageFormat returns. For
// animated formats, only the first frame is returned.
//
//  - On total success, the returned error_message is empty.
//  - On partial success (e.g. the input file was truncated but we are still
//    able to decode some pixels), error_message is non-empty but the returned
//    pixbuf.pixcfg.is_valid() is true.
//  - On failure, the error_message is non_empty and pixbuf.pixcfg.is_valid()
//    is false.
//
// The callbacks allocate the pixel buffer memory and work buffer memory. On
// success, pixel buffer memory ownership is passed to the DecodeImage caller
// as the returned pixbuf_mem_owner. Regardless of success or failure, the work
// buffer memory is deleted.
//
// If override_pixel_format_repr is zero then the pixel buffer will have the
// image file's natural pixel format. For example, GIF images' natural pixel
// format is an indexed one.
//
// If override_pixel_format_repr is non-zero (and one of the constants listed
// below) then the image will be decoded to that particular pixel format:
//  - WUFFS_BASE__PIXEL_FORMAT__BGR_565
//  - WUFFS_BASE__PIXEL_FORMAT__BGR
//  - WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL
//  - WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL
//
// The pixel_blend (one of the constants listed below) determines how to
// composite the decoded image over the pixel buffer's original pixels (as
// returned by callbacks.OnImageConfig):
//  - WUFFS_BASE__PIXEL_BLEND__SRC
//  - WUFFS_BASE__PIXEL_BLEND__SRC_OVER
//
// Decoding fails (with DecodeImage_MaxInclDimensionExceeded) if the image's
// width or height is greater than max_incl_dimension.
DecodeImageResult  //
DecodeImage(DecodeImageCallbacks& callbacks,
            sync_io::Input& input,
            uint32_t override_pixel_format_repr,
            wuffs_base__pixel_blend pixel_blend = WUFFS_BASE__PIXEL_BLEND__SRC,
            uint32_t max_incl_dimension = 1048575);  // 0x000F_FFFF

}  // namespace wuffs_aux
