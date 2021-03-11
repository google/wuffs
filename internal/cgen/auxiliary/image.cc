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

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__AUX__IMAGE)

#include <utility>

namespace wuffs_aux {

DecodeImageResult::DecodeImageResult(MemOwner&& pixbuf_mem_owner0,
                                     wuffs_base__slice_u8 pixbuf_mem_slice0,
                                     wuffs_base__pixel_buffer pixbuf0,
                                     std::string&& error_message0)
    : pixbuf_mem_owner(std::move(pixbuf_mem_owner0)),
      pixbuf_mem_slice(pixbuf_mem_slice0),
      pixbuf(pixbuf0),
      error_message(std::move(error_message0)) {}

DecodeImageResult::DecodeImageResult(std::string&& error_message0)
    : pixbuf_mem_owner(nullptr, &free),
      pixbuf_mem_slice(wuffs_base__empty_slice_u8()),
      pixbuf(wuffs_base__null_pixel_buffer()),
      error_message(std::move(error_message0)) {}

DecodeImageCallbacks::AllocResult::AllocResult(MemOwner&& mem_owner0,
                                               wuffs_base__slice_u8 mem_slice0)
    : mem_owner(std::move(mem_owner0)),
      mem_slice(mem_slice0),
      error_message("") {}

DecodeImageCallbacks::AllocResult::AllocResult(std::string&& error_message0)
    : mem_owner(nullptr, &free),
      mem_slice(wuffs_base__empty_slice_u8()),
      error_message(std::move(error_message0)) {}

wuffs_base__image_decoder::unique_ptr  //
DecodeImageCallbacks::SelectDecoder(uint32_t fourcc,
                                    wuffs_base__slice_u8 prefix) {
  switch (fourcc) {
#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__BMP)
    case WUFFS_BASE__FOURCC__BMP:
      return wuffs_bmp__decoder::alloc_as__wuffs_base__image_decoder();
#endif

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__GIF)
    case WUFFS_BASE__FOURCC__GIF:
      return wuffs_gif__decoder::alloc_as__wuffs_base__image_decoder();
#endif

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__NIE)
    case WUFFS_BASE__FOURCC__NIE:
      return wuffs_nie__decoder::alloc_as__wuffs_base__image_decoder();
#endif

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__PNG)
    case WUFFS_BASE__FOURCC__PNG: {
      auto dec = wuffs_png__decoder::alloc_as__wuffs_base__image_decoder();
      // Favor faster decodes over rejecting invalid checksums.
      dec->set_quirk_enabled(WUFFS_BASE__QUIRK_IGNORE_CHECKSUM, true);
      return dec;
    }
#endif

#if !defined(WUFFS_CONFIG__MODULES) || defined(WUFFS_CONFIG__MODULE__WBMP)
    case WUFFS_BASE__FOURCC__WBMP:
      return wuffs_wbmp__decoder::alloc_as__wuffs_base__image_decoder();
#endif
  }

  return wuffs_base__image_decoder::unique_ptr(nullptr, &free);
}

wuffs_base__pixel_format  //
DecodeImageCallbacks::SelectPixfmt(
    const wuffs_base__image_config& image_config) {
  return wuffs_base__make_pixel_format(
      WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL);
}

DecodeImageCallbacks::AllocResult  //
DecodeImageCallbacks::AllocPixbuf(const wuffs_base__image_config& image_config,
                                  bool allow_uninitialized_memory) {
  uint32_t w = image_config.pixcfg.width();
  uint32_t h = image_config.pixcfg.height();
  if ((w == 0) || (h == 0)) {
    return AllocResult("");
  }
  uint64_t len = image_config.pixcfg.pixbuf_len();
  if ((len == 0) || (SIZE_MAX < len)) {
    return AllocResult(DecodeImage_UnsupportedPixelConfiguration);
  }
  void* ptr =
      allow_uninitialized_memory ? malloc((size_t)len) : calloc((size_t)len, 1);
  return ptr ? AllocResult(
                   MemOwner(ptr, &free),
                   wuffs_base__make_slice_u8((uint8_t*)ptr, (size_t)len))
             : AllocResult(DecodeImage_OutOfMemory);
}

DecodeImageCallbacks::AllocResult  //
DecodeImageCallbacks::AllocWorkbuf(wuffs_base__range_ii_u64 len_range,
                                   bool allow_uninitialized_memory) {
  uint64_t len = len_range.max_incl;
  if (len == 0) {
    return AllocResult("");
  } else if (SIZE_MAX < len) {
    return AllocResult(DecodeImage_OutOfMemory);
  }
  void* ptr =
      allow_uninitialized_memory ? malloc((size_t)len) : calloc((size_t)len, 1);
  return ptr ? AllocResult(
                   MemOwner(ptr, &free),
                   wuffs_base__make_slice_u8((uint8_t*)ptr, (size_t)len))
             : AllocResult(DecodeImage_OutOfMemory);
}

void  //
DecodeImageCallbacks::Done(
    DecodeImageResult& result,
    sync_io::Input& input,
    IOBuffer& buffer,
    wuffs_base__image_decoder::unique_ptr image_decoder) {}

const char DecodeImage_BufferIsTooShort[] =  //
    "wuffs_aux::DecodeImage: buffer is too short";
const char DecodeImage_MaxInclDimensionExceeded[] =  //
    "wuffs_aux::DecodeImage: max_incl_dimension exceeded";
const char DecodeImage_OutOfMemory[] =  //
    "wuffs_aux::DecodeImage: out of memory";
const char DecodeImage_UnexpectedEndOfFile[] =  //
    "wuffs_aux::DecodeImage: unexpected end of file";
const char DecodeImage_UnsupportedImageFormat[] =  //
    "wuffs_aux::DecodeImage: unsupported image format";
const char DecodeImage_UnsupportedPixelBlend[] =  //
    "wuffs_aux::DecodeImage: unsupported pixel blend";
const char DecodeImage_UnsupportedPixelConfiguration[] =  //
    "wuffs_aux::DecodeImage: unsupported pixel configuration";
const char DecodeImage_UnsupportedPixelFormat[] =  //
    "wuffs_aux::DecodeImage: unsupported pixel format";

// --------

namespace {

std::string  //
DecodeImageAdvanceIOBuf(sync_io::Input& input,
                        wuffs_base__io_buffer& io_buf,
                        bool compactable,
                        uint64_t min_excl_pos,
                        uint64_t pos) {
  if ((pos <= min_excl_pos) || (pos < io_buf.reader_position())) {
    // Redirects must go forward.
    return DecodeImage_UnsupportedImageFormat;
  }
  while (true) {
    uint64_t relative_pos = pos - io_buf.reader_position();
    if (relative_pos <= io_buf.reader_length()) {
      io_buf.meta.ri += (size_t)relative_pos;
      break;
    } else if (io_buf.meta.closed) {
      return DecodeImage_UnexpectedEndOfFile;
    }
    io_buf.meta.ri = io_buf.meta.wi;
    if (compactable) {
      io_buf.compact();
    }
    std::string error_message = input.CopyIn(&io_buf);
    if (!error_message.empty()) {
      return error_message;
    }
  }
  return "";
}

DecodeImageResult  //
DecodeImage0(wuffs_base__image_decoder::unique_ptr& image_decoder,
             DecodeImageCallbacks& callbacks,
             sync_io::Input& input,
             wuffs_base__io_buffer& io_buf,
             wuffs_base__pixel_blend pixel_blend,
             wuffs_base__color_u32_argb_premul background_color,
             uint32_t max_incl_dimension) {
  // Check args.
  switch (pixel_blend) {
    case WUFFS_BASE__PIXEL_BLEND__SRC:
    case WUFFS_BASE__PIXEL_BLEND__SRC_OVER:
      break;
    default:
      return DecodeImageResult(DecodeImage_UnsupportedPixelBlend);
  }

  wuffs_base__image_config image_config = wuffs_base__null_image_config();
  uint64_t start_pos = io_buf.reader_position();
  bool redirected = false;
  int32_t fourcc = 0;
redirect:
  do {
    // Determine the image format.
    if (!redirected) {
      while (true) {
        fourcc = wuffs_base__magic_number_guess_fourcc(io_buf.reader_slice());
        if (fourcc > 0) {
          break;
        } else if ((fourcc == 0) && (io_buf.reader_length() >= 64)) {
          break;
        } else if (io_buf.meta.closed || (io_buf.writer_length() == 0)) {
          fourcc = 0;
          break;
        }
        std::string error_message = input.CopyIn(&io_buf);
        if (!error_message.empty()) {
          return DecodeImageResult(std::move(error_message));
        }
      }
    } else {
      wuffs_base__io_buffer empty = wuffs_base__empty_io_buffer();
      wuffs_base__more_information minfo = wuffs_base__empty_more_information();
      wuffs_base__status tmm_status =
          image_decoder->tell_me_more(&empty, &minfo, &io_buf);
      if (tmm_status.repr != nullptr) {
        return DecodeImageResult(tmm_status.message());
      }
      if (minfo.flavor != WUFFS_BASE__MORE_INFORMATION__FLAVOR__IO_REDIRECT) {
        return DecodeImageResult(DecodeImage_UnsupportedImageFormat);
      }
      uint64_t pos = minfo.io_redirect__range().min_incl;
      std::string error_message = DecodeImageAdvanceIOBuf(
          input, io_buf, !input.BringsItsOwnIOBuffer(), start_pos, pos);
      if (!error_message.empty()) {
        return DecodeImageResult(std::move(error_message));
      }
      fourcc = (int32_t)(minfo.io_redirect__fourcc());
      if (fourcc == 0) {
        return DecodeImageResult(DecodeImage_UnsupportedImageFormat);
      }
      image_decoder.reset();
    }

    // Select the image decoder.
    image_decoder = callbacks.SelectDecoder(
        (uint32_t)fourcc,
        fourcc ? wuffs_base__empty_slice_u8() : io_buf.reader_slice());
    if (!image_decoder) {
      return DecodeImageResult(DecodeImage_UnsupportedImageFormat);
    }

    // Decode the image config.
    while (true) {
      wuffs_base__status id_dic_status =
          image_decoder->decode_image_config(&image_config, &io_buf);
      if (id_dic_status.repr == nullptr) {
        break;
      } else if (id_dic_status.repr == wuffs_base__note__i_o_redirect) {
        if (redirected) {
          return DecodeImageResult(DecodeImage_UnsupportedImageFormat);
        }
        redirected = true;
        goto redirect;
      } else if (id_dic_status.repr != wuffs_base__suspension__short_read) {
        return DecodeImageResult(id_dic_status.message());
      } else if (io_buf.meta.closed) {
        return DecodeImageResult(DecodeImage_UnexpectedEndOfFile);
      } else {
        std::string error_message = input.CopyIn(&io_buf);
        if (!error_message.empty()) {
          return DecodeImageResult(std::move(error_message));
        }
      }
    }
  } while (false);

  // Select the pixel format.
  uint32_t w = image_config.pixcfg.width();
  uint32_t h = image_config.pixcfg.height();
  if ((w > max_incl_dimension) || (h > max_incl_dimension)) {
    return DecodeImageResult(DecodeImage_MaxInclDimensionExceeded);
  }
  wuffs_base__pixel_format pixel_format = callbacks.SelectPixfmt(image_config);
  if (pixel_format.repr != image_config.pixcfg.pixel_format().repr) {
    switch (pixel_format.repr) {
      case WUFFS_BASE__PIXEL_FORMAT__BGR_565:
      case WUFFS_BASE__PIXEL_FORMAT__BGR:
      case WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL:
      case WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL:
        break;
      default:
        return DecodeImageResult(DecodeImage_UnsupportedPixelFormat);
    }
    image_config.pixcfg.set(pixel_format.repr,
                            WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, w, h);
  }

  // Allocate the pixel buffer.
  uint64_t pixbuf_len_min_incl = 0;
  if ((w > 0) && (h > 0)) {
    pixbuf_len_min_incl = image_config.pixcfg.pixbuf_len();
    if (pixbuf_len_min_incl == 0) {
      return DecodeImageResult(DecodeImage_UnsupportedPixelFormat);
    }
  }
  bool valid_background_color =
      wuffs_base__color_u32_argb_premul__is_valid(background_color);
  DecodeImageCallbacks::AllocResult alloc_pixbuf_result =
      callbacks.AllocPixbuf(image_config, valid_background_color);
  if (!alloc_pixbuf_result.error_message.empty()) {
    return DecodeImageResult(std::move(alloc_pixbuf_result.error_message));
  } else if (alloc_pixbuf_result.mem_slice.len < pixbuf_len_min_incl) {
    return DecodeImageResult(DecodeImage_BufferIsTooShort);
  }
  wuffs_base__pixel_buffer pixel_buffer;
  wuffs_base__status pb_sfs_status = pixel_buffer.set_from_slice(
      &image_config.pixcfg, alloc_pixbuf_result.mem_slice);
  if (!pb_sfs_status.is_ok()) {
    return DecodeImageResult(pb_sfs_status.message());
  }
  if (valid_background_color) {
    wuffs_base__status pb_scufr_status = pixel_buffer.set_color_u32_fill_rect(
        pixel_buffer.pixcfg.bounds(), background_color);
    if (pb_scufr_status.repr != nullptr) {
      return DecodeImageResult(pb_scufr_status.message());
    }
  }

  // Allocate the work buffer. Wuffs' decoders conventionally assume that this
  // can be uninitialized memory.
  wuffs_base__range_ii_u64 workbuf_len = image_decoder->workbuf_len();
  DecodeImageCallbacks::AllocResult alloc_workbuf_result =
      callbacks.AllocWorkbuf(workbuf_len, true);
  if (!alloc_workbuf_result.error_message.empty()) {
    return DecodeImageResult(std::move(alloc_workbuf_result.error_message));
  } else if (alloc_workbuf_result.mem_slice.len < workbuf_len.min_incl) {
    return DecodeImageResult(DecodeImage_BufferIsTooShort);
  }

  // Decode the frame config.
  wuffs_base__frame_config frame_config = wuffs_base__null_frame_config();
  while (true) {
    wuffs_base__status id_dfc_status =
        image_decoder->decode_frame_config(&frame_config, &io_buf);
    if (id_dfc_status.repr == nullptr) {
      break;
    } else if (id_dfc_status.repr != wuffs_base__suspension__short_read) {
      return DecodeImageResult(id_dfc_status.message());
    } else if (io_buf.meta.closed) {
      return DecodeImageResult(DecodeImage_UnexpectedEndOfFile);
    } else {
      std::string error_message = input.CopyIn(&io_buf);
      if (!error_message.empty()) {
        return DecodeImageResult(std::move(error_message));
      }
    }
  }

  // Decode the frame (the pixels).
  //
  // From here on, always returns the pixel_buffer. If we get this far, we can
  // still display a partial image, even if we encounter an error.
  std::string message("");
  if ((pixel_blend == WUFFS_BASE__PIXEL_BLEND__SRC_OVER) &&
      frame_config.overwrite_instead_of_blend()) {
    pixel_blend = WUFFS_BASE__PIXEL_BLEND__SRC;
  }
  while (true) {
    wuffs_base__status id_df_status =
        image_decoder->decode_frame(&pixel_buffer, &io_buf, pixel_blend,
                                    alloc_workbuf_result.mem_slice, nullptr);
    if (id_df_status.repr == nullptr) {
      break;
    } else if (id_df_status.repr != wuffs_base__suspension__short_read) {
      message = id_df_status.message();
      break;
    } else if (io_buf.meta.closed) {
      message = DecodeImage_UnexpectedEndOfFile;
      break;
    } else {
      std::string error_message = input.CopyIn(&io_buf);
      if (!error_message.empty()) {
        message = std::move(error_message);
        break;
      }
    }
  }
  return DecodeImageResult(std::move(alloc_pixbuf_result.mem_owner),
                           alloc_pixbuf_result.mem_slice, pixel_buffer,
                           std::move(message));
}

}  // namespace

DecodeImageResult  //
DecodeImage(DecodeImageCallbacks& callbacks,
            sync_io::Input& input,
            wuffs_base__pixel_blend pixel_blend,
            wuffs_base__color_u32_argb_premul background_color,
            uint32_t max_incl_dimension) {
  wuffs_base__io_buffer* io_buf = input.BringsItsOwnIOBuffer();
  wuffs_base__io_buffer fallback_io_buf = wuffs_base__empty_io_buffer();
  std::unique_ptr<uint8_t[]> fallback_io_array(nullptr);
  if (!io_buf) {
    fallback_io_array = std::unique_ptr<uint8_t[]>(new uint8_t[32768]);
    fallback_io_buf =
        wuffs_base__ptr_u8__writer(fallback_io_array.get(), 32768);
    io_buf = &fallback_io_buf;
  }

  wuffs_base__image_decoder::unique_ptr image_decoder(nullptr, &free);
  DecodeImageResult result =
      DecodeImage0(image_decoder, callbacks, input, *io_buf, pixel_blend,
                   background_color, max_incl_dimension);
  callbacks.Done(result, input, *io_buf, std::move(image_decoder));
  return result;
}

}  // namespace wuffs_aux

#endif  // !defined(WUFFS_CONFIG__MODULES) ||
        // defined(WUFFS_CONFIG__MODULE__AUX__IMAGE)
