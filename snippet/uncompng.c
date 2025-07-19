// Copyright 2024 The Wuffs Authors.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// https://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

// ----------------

// This is a small, self-contained, single-file C library to convert pixel data
// to the PNG file format, without using any compression.
//
// To use this file as a "foo.c"-like implementation, instead of a "foo.h"-like
// header, #define UNCOMPNG_IMPLEMENTATION before #include'ing or compiling it.
//
// As an option, you may also #define UNCOMPNG_CONFIG__STATIC_FUNCTIONS to make
// these functions have static storage. This can help the compiler ignore or
// discard unused code, which can produce faster compiles and smaller binaries.
//
// It is a C port of the github.com/google/wuffs/lib/uncompng library. The
// original Go code has much more comments about the algorithmic details.
//
// There's an example program (using this library) at
// https://nigeltao.github.io/blog/2025/uncompressed-png.html

#ifndef UNCOMPNG_INCLUDE_GUARD
#define UNCOMPNG_INCLUDE_GUARD

#if defined(UNCOMPNG_CONFIG__STATIC_FUNCTIONS)
#define UNCOMPNG__MAYBE_STATIC static
#else
#define UNCOMPNG__MAYBE_STATIC
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// clang-format off

// UNCOMPNG__PIXEL_FORMAT__ETC are the valid pixel_format values to pass to
// uncompng__encode.
//
// These constants' values are the same as the corresponding Wuffs definitions,
// after replacing the name's "WUFFS_BASE" prefix with "UNCOMPNG". This file is
// stand-alone. It does not #include any Wuffs code.
#define UNCOMPNG__PIXEL_FORMAT__Y                        0x20000008
#define UNCOMPNG__PIXEL_FORMAT__Y_16LE                   0x2000000B
#define UNCOMPNG__PIXEL_FORMAT__YXXX                     0x30008888
#define UNCOMPNG__PIXEL_FORMAT__YXXX_4X16LE              0x3000BBBB
#define UNCOMPNG__PIXEL_FORMAT__BGRA_NONPREMUL           0x81008888
#define UNCOMPNG__PIXEL_FORMAT__BGRA_NONPREMUL_4X16LE    0x8100BBBB
#define UNCOMPNG__PIXEL_FORMAT__BGRX                     0x90008888
#define UNCOMPNG__PIXEL_FORMAT__BGRX_4X16LE              0x9000BBBB

// clang-format on

// UNCOMPNG__RESULT__ETC can be returned by uncompng__encode. write_func can
// also return its own negative error codes, which are passed on.
#define UNCOMPNG__RESULT__OK 0
#define UNCOMPNG__RESULT__INVALID_ARGUMENT 1
#define UNCOMPNG__RESULT__UNSUPPORTED_IMAGE_SIZE 2
#define UNCOMPNG__RESULT__CONCURRENT_CALL 3

// UNCOMPNG__DATA_LEN__INCL_MAX is the inclusive maximum value of write_func's
// data_len argument. In hexadecimal, it equals 0x10000u.
#define UNCOMPNG__DATA_LEN__INCL_MAX 65536u

// uncompng__encode writes pixel data in PNG format to write_func. The callback
// may be run multiple times. Each time, write_func is expected to handle the
// entirety of the (data_ptr, data_len) slice. data_len will never exceed
// UNCOMPNG__DATA_LEN__INCL_MAX.
//
// It returns zero on success, a positive number (an UNCOMPNG__RESULT__ETC
// constant) on library failures or a negative number on write_func failures.
//
// write_func should return zero for success or negative for failure (which is
// passed back to the uncompng__encode caller). Returning a positive number is
// not recommended, as that may clash with UNCOMPNG__RESULT__ETC values.
//
// The context argument is unused other than being an opaque value that is
// forwarded on to write_func.
//
// Pixel data is in the (pixel_ptr, pixel_len) slice, either 1, 2, 4 or 8 bytes
// per pixel depending on the pixel_format. width and height are measured in
// pixels. stride, the pointer difference between rows, is measured in bytes.
//
// This function is not thread-safe.
UNCOMPNG__MAYBE_STATIC int  //
uncompng__encode(int (*write_func)(void* context,
                                   const uint8_t* data_ptr,
                                   size_t data_len),
                 void* context,
                 const uint8_t* pixel_ptr,
                 size_t pixel_len,
                 uint32_t width,
                 uint32_t height,
                 size_t stride,
                 uint32_t pixel_format);

// --------

#ifdef UNCOMPNG_IMPLEMENTATION

static uint32_t  //
uncompng__private_impl_crc32_ieee(const uint8_t* ptr, size_t len) {
  static const uint32_t table[256] = {
      0x00000000u, 0x77073096u, 0xEE0E612Cu, 0x990951BAu, 0x076DC419u,
      0x706AF48Fu, 0xE963A535u, 0x9E6495A3u, 0x0EDB8832u, 0x79DCB8A4u,
      0xE0D5E91Eu, 0x97D2D988u, 0x09B64C2Bu, 0x7EB17CBDu, 0xE7B82D07u,
      0x90BF1D91u, 0x1DB71064u, 0x6AB020F2u, 0xF3B97148u, 0x84BE41DEu,
      0x1ADAD47Du, 0x6DDDE4EBu, 0xF4D4B551u, 0x83D385C7u, 0x136C9856u,
      0x646BA8C0u, 0xFD62F97Au, 0x8A65C9ECu, 0x14015C4Fu, 0x63066CD9u,
      0xFA0F3D63u, 0x8D080DF5u, 0x3B6E20C8u, 0x4C69105Eu, 0xD56041E4u,
      0xA2677172u, 0x3C03E4D1u, 0x4B04D447u, 0xD20D85FDu, 0xA50AB56Bu,
      0x35B5A8FAu, 0x42B2986Cu, 0xDBBBC9D6u, 0xACBCF940u, 0x32D86CE3u,
      0x45DF5C75u, 0xDCD60DCFu, 0xABD13D59u, 0x26D930ACu, 0x51DE003Au,
      0xC8D75180u, 0xBFD06116u, 0x21B4F4B5u, 0x56B3C423u, 0xCFBA9599u,
      0xB8BDA50Fu, 0x2802B89Eu, 0x5F058808u, 0xC60CD9B2u, 0xB10BE924u,
      0x2F6F7C87u, 0x58684C11u, 0xC1611DABu, 0xB6662D3Du, 0x76DC4190u,
      0x01DB7106u, 0x98D220BCu, 0xEFD5102Au, 0x71B18589u, 0x06B6B51Fu,
      0x9FBFE4A5u, 0xE8B8D433u, 0x7807C9A2u, 0x0F00F934u, 0x9609A88Eu,
      0xE10E9818u, 0x7F6A0DBBu, 0x086D3D2Du, 0x91646C97u, 0xE6635C01u,
      0x6B6B51F4u, 0x1C6C6162u, 0x856530D8u, 0xF262004Eu, 0x6C0695EDu,
      0x1B01A57Bu, 0x8208F4C1u, 0xF50FC457u, 0x65B0D9C6u, 0x12B7E950u,
      0x8BBEB8EAu, 0xFCB9887Cu, 0x62DD1DDFu, 0x15DA2D49u, 0x8CD37CF3u,
      0xFBD44C65u, 0x4DB26158u, 0x3AB551CEu, 0xA3BC0074u, 0xD4BB30E2u,
      0x4ADFA541u, 0x3DD895D7u, 0xA4D1C46Du, 0xD3D6F4FBu, 0x4369E96Au,
      0x346ED9FCu, 0xAD678846u, 0xDA60B8D0u, 0x44042D73u, 0x33031DE5u,
      0xAA0A4C5Fu, 0xDD0D7CC9u, 0x5005713Cu, 0x270241AAu, 0xBE0B1010u,
      0xC90C2086u, 0x5768B525u, 0x206F85B3u, 0xB966D409u, 0xCE61E49Fu,
      0x5EDEF90Eu, 0x29D9C998u, 0xB0D09822u, 0xC7D7A8B4u, 0x59B33D17u,
      0x2EB40D81u, 0xB7BD5C3Bu, 0xC0BA6CADu, 0xEDB88320u, 0x9ABFB3B6u,
      0x03B6E20Cu, 0x74B1D29Au, 0xEAD54739u, 0x9DD277AFu, 0x04DB2615u,
      0x73DC1683u, 0xE3630B12u, 0x94643B84u, 0x0D6D6A3Eu, 0x7A6A5AA8u,
      0xE40ECF0Bu, 0x9309FF9Du, 0x0A00AE27u, 0x7D079EB1u, 0xF00F9344u,
      0x8708A3D2u, 0x1E01F268u, 0x6906C2FEu, 0xF762575Du, 0x806567CBu,
      0x196C3671u, 0x6E6B06E7u, 0xFED41B76u, 0x89D32BE0u, 0x10DA7A5Au,
      0x67DD4ACCu, 0xF9B9DF6Fu, 0x8EBEEFF9u, 0x17B7BE43u, 0x60B08ED5u,
      0xD6D6A3E8u, 0xA1D1937Eu, 0x38D8C2C4u, 0x4FDFF252u, 0xD1BB67F1u,
      0xA6BC5767u, 0x3FB506DDu, 0x48B2364Bu, 0xD80D2BDAu, 0xAF0A1B4Cu,
      0x36034AF6u, 0x41047A60u, 0xDF60EFC3u, 0xA867DF55u, 0x316E8EEFu,
      0x4669BE79u, 0xCB61B38Cu, 0xBC66831Au, 0x256FD2A0u, 0x5268E236u,
      0xCC0C7795u, 0xBB0B4703u, 0x220216B9u, 0x5505262Fu, 0xC5BA3BBEu,
      0xB2BD0B28u, 0x2BB45A92u, 0x5CB36A04u, 0xC2D7FFA7u, 0xB5D0CF31u,
      0x2CD99E8Bu, 0x5BDEAE1Du, 0x9B64C2B0u, 0xEC63F226u, 0x756AA39Cu,
      0x026D930Au, 0x9C0906A9u, 0xEB0E363Fu, 0x72076785u, 0x05005713u,
      0x95BF4A82u, 0xE2B87A14u, 0x7BB12BAEu, 0x0CB61B38u, 0x92D28E9Bu,
      0xE5D5BE0Du, 0x7CDCEFB7u, 0x0BDBDF21u, 0x86D3D2D4u, 0xF1D4E242u,
      0x68DDB3F8u, 0x1FDA836Eu, 0x81BE16CDu, 0xF6B9265Bu, 0x6FB077E1u,
      0x18B74777u, 0x88085AE6u, 0xFF0F6A70u, 0x66063BCAu, 0x11010B5Cu,
      0x8F659EFFu, 0xF862AE69u, 0x616BFFD3u, 0x166CCF45u, 0xA00AE278u,
      0xD70DD2EEu, 0x4E048354u, 0x3903B3C2u, 0xA7672661u, 0xD06016F7u,
      0x4969474Du, 0x3E6E77DBu, 0xAED16A4Au, 0xD9D65ADCu, 0x40DF0B66u,
      0x37D83BF0u, 0xA9BCAE53u, 0xDEBB9EC5u, 0x47B2CF7Fu, 0x30B5FFE9u,
      0xBDBDF21Cu, 0xCABAC28Au, 0x53B39330u, 0x24B4A3A6u, 0xBAD03605u,
      0xCDD70693u, 0x54DE5729u, 0x23D967BFu, 0xB3667A2Eu, 0xC4614AB8u,
      0x5D681B02u, 0x2A6F2B94u, 0xB40BBE37u, 0xC30C8EA1u, 0x5A05DF1Bu,
      0x2D02EF8Du,
  };

  uint32_t hash = 0xFFFFFFFFu;
  while (len--) {
    hash = table[(uint8_t)hash ^ (*ptr++)] ^ (hash >> 8);
  }
  return hash ^ 0xFFFFFFFFu;
}

static uint8_t uncompng__private_impl_buffer[65536];

static void  //
uncompng__private_impl_initialize_buffer(uint32_t width,
                                         uint32_t height,
                                         uint32_t pixel_format) {
  uncompng__private_impl_buffer[0x0000] = 0x89;
  uncompng__private_impl_buffer[0x0001] = 'P';
  uncompng__private_impl_buffer[0x0002] = 'N';
  uncompng__private_impl_buffer[0x0003] = 'G';
  uncompng__private_impl_buffer[0x0004] = 0x0D;
  uncompng__private_impl_buffer[0x0005] = 0x0A;
  uncompng__private_impl_buffer[0x0006] = 0x1A;
  uncompng__private_impl_buffer[0x0007] = 0x0A;
  uncompng__private_impl_buffer[0x0008] = 0;
  uncompng__private_impl_buffer[0x0009] = 0;
  uncompng__private_impl_buffer[0x000A] = 0;
  uncompng__private_impl_buffer[0x000B] = 0x0D;
  uncompng__private_impl_buffer[0x000C] = 'I';
  uncompng__private_impl_buffer[0x000D] = 'H';
  uncompng__private_impl_buffer[0x000E] = 'D';
  uncompng__private_impl_buffer[0x000F] = 'R';
  uncompng__private_impl_buffer[0x0010] = (uint8_t)(width >> 24);
  uncompng__private_impl_buffer[0x0011] = (uint8_t)(width >> 16);
  uncompng__private_impl_buffer[0x0012] = (uint8_t)(width >> 8);
  uncompng__private_impl_buffer[0x0013] = (uint8_t)(width >> 0);
  uncompng__private_impl_buffer[0x0014] = (uint8_t)(height >> 24);
  uncompng__private_impl_buffer[0x0015] = (uint8_t)(height >> 16);
  uncompng__private_impl_buffer[0x0016] = (uint8_t)(height >> 8);
  uncompng__private_impl_buffer[0x0017] = (uint8_t)(height >> 0);

  uint8_t depth;
  uint8_t color_type;
  switch (pixel_format) {
    case UNCOMPNG__PIXEL_FORMAT__Y:
    case UNCOMPNG__PIXEL_FORMAT__YXXX:
      depth = 8;
      color_type = 0;
      break;
    case UNCOMPNG__PIXEL_FORMAT__BGRA_NONPREMUL:
      depth = 8;
      color_type = 6;
      break;
    case UNCOMPNG__PIXEL_FORMAT__BGRX:
      depth = 8;
      color_type = 2;
      break;
    case UNCOMPNG__PIXEL_FORMAT__Y_16LE:
    case UNCOMPNG__PIXEL_FORMAT__YXXX_4X16LE:
      depth = 16;
      color_type = 0;
      break;
    case UNCOMPNG__PIXEL_FORMAT__BGRA_NONPREMUL_4X16LE:
      depth = 16;
      color_type = 6;
      break;
    case UNCOMPNG__PIXEL_FORMAT__BGRX_4X16LE:
      depth = 16;
      color_type = 2;
      break;
    default:
      return;
  }
  uncompng__private_impl_buffer[0x0018] = depth;
  uncompng__private_impl_buffer[0x0019] = color_type;
  uncompng__private_impl_buffer[0x001A] = 0;
  uncompng__private_impl_buffer[0x001B] = 0;
  uncompng__private_impl_buffer[0x001C] = 0;

  uint32_t ihdr_crc32 = uncompng__private_impl_crc32_ieee(
      uncompng__private_impl_buffer + 0x000C, 0x001D - 0x000C);
  uncompng__private_impl_buffer[0x001D] = (uint8_t)(ihdr_crc32 >> 24);
  uncompng__private_impl_buffer[0x001E] = (uint8_t)(ihdr_crc32 >> 16);
  uncompng__private_impl_buffer[0x001F] = (uint8_t)(ihdr_crc32 >> 8);
  uncompng__private_impl_buffer[0x0020] = (uint8_t)(ihdr_crc32 >> 0);

  uncompng__private_impl_buffer[0x0021] = 0;
  uncompng__private_impl_buffer[0x0022] = 0;
  uncompng__private_impl_buffer[0x0023] = 0;
  uncompng__private_impl_buffer[0x0024] = 0;
  uncompng__private_impl_buffer[0x0025] = 'I';
  uncompng__private_impl_buffer[0x0026] = 'D';
  uncompng__private_impl_buffer[0x0027] = 'A';
  uncompng__private_impl_buffer[0x0028] = 'T';
  uncompng__private_impl_buffer[0x0029] = 0x78;
  uncompng__private_impl_buffer[0x002A] = 0x01;
  uncompng__private_impl_buffer[0x002B] = 0;
  uncompng__private_impl_buffer[0x002C] = 0;
  uncompng__private_impl_buffer[0x002D] = 0;
  uncompng__private_impl_buffer[0x002E] = 0;
  uncompng__private_impl_buffer[0x002F] = 0;
  uncompng__private_impl_buffer[0xFFFC] = 0;
  uncompng__private_impl_buffer[0xFFFD] = 0;
  uncompng__private_impl_buffer[0xFFFE] = 0;
  uncompng__private_impl_buffer[0xFFFF] = 1;
}

static void  //
uncompng__private_impl_update_adler32(int ei, int ej) {
  uint32_t b = ((uint32_t)uncompng__private_impl_buffer[0xFFFC] << 8) |
               ((uint32_t)uncompng__private_impl_buffer[0xFFFD]);
  uint32_t a = ((uint32_t)uncompng__private_impl_buffer[0xFFFE] << 8) |
               ((uint32_t)uncompng__private_impl_buffer[0xFFFF]);
  while (ei < ej) {
    int end = ei + 5552;
    if (end > ej) {
      end = ej;
    }
    for (; ei < end; ei++) {
      a += (uint32_t)uncompng__private_impl_buffer[ei];
      b += a;
    }
    a %= 65521;
    b %= 65521;
  }
  uncompng__private_impl_buffer[0xFFFC] = (uint8_t)(b >> 8);
  uncompng__private_impl_buffer[0xFFFD] = (uint8_t)(b >> 0);
  uncompng__private_impl_buffer[0xFFFE] = (uint8_t)(a >> 8);
  uncompng__private_impl_buffer[0xFFFF] = (uint8_t)(a >> 0);
}

static int  //
uncompng__private_impl_flush(int (*write_func)(void* context,
                                               const uint8_t* data_ptr,
                                               size_t data_len),
                             void* context,
                             int ej,
                             bool final) {
  static const int ei_first = 0x0030;
  static const int ei_later = 0x000D;

  int crc32_start = 0;
  int ei = 0;
  if (uncompng__private_impl_buffer[0x0004] == 0x0D) {
    uint32_t idat_chunk_len = ej - 0x0029;
    if (final) {
      idat_chunk_len += 4;
    }
    uncompng__private_impl_buffer[0x0021] = (uint8_t)(idat_chunk_len >> 24);
    uncompng__private_impl_buffer[0x0022] = (uint8_t)(idat_chunk_len >> 16);
    uncompng__private_impl_buffer[0x0023] = (uint8_t)(idat_chunk_len >> 8);
    uncompng__private_impl_buffer[0x0024] = (uint8_t)(idat_chunk_len >> 0);
    crc32_start = 0x0025;
    ei = ei_first;
  } else {
    uint32_t idat_chunk_len = ej - 0x0008;
    if (final) {
      idat_chunk_len += 4;
    }
    uncompng__private_impl_buffer[0x0000] = (uint8_t)(idat_chunk_len >> 24);
    uncompng__private_impl_buffer[0x0001] = (uint8_t)(idat_chunk_len >> 16);
    uncompng__private_impl_buffer[0x0002] = (uint8_t)(idat_chunk_len >> 8);
    uncompng__private_impl_buffer[0x0003] = (uint8_t)(idat_chunk_len >> 0);
    crc32_start = 0x0004;
    ei = ei_later;
  }

  uint32_t deflate_block_len = (uint32_t)(ej - ei);
  uncompng__private_impl_buffer[ei - 5] = final ? 1 : 0;
  uncompng__private_impl_buffer[ei - 4] =
      0x00u ^ (uint8_t)(deflate_block_len >> 0);
  uncompng__private_impl_buffer[ei - 3] =
      0x00u ^ (uint8_t)(deflate_block_len >> 8);
  uncompng__private_impl_buffer[ei - 2] =
      0xFFu ^ (uint8_t)(deflate_block_len >> 0);
  uncompng__private_impl_buffer[ei - 1] =
      0xFFu ^ (uint8_t)(deflate_block_len >> 8);

  uncompng__private_impl_update_adler32(ei, ej);
  if (final) {
    uncompng__private_impl_buffer[ej + 0] =
        uncompng__private_impl_buffer[0xFFFC];
    uncompng__private_impl_buffer[ej + 1] =
        uncompng__private_impl_buffer[0xFFFD];
    uncompng__private_impl_buffer[ej + 2] =
        uncompng__private_impl_buffer[0xFFFE];
    uncompng__private_impl_buffer[ej + 3] =
        uncompng__private_impl_buffer[0xFFFF];
    ej += 4;
  }

  uint32_t idat_crc32 = uncompng__private_impl_crc32_ieee(
      uncompng__private_impl_buffer + crc32_start, ej - crc32_start);
  uncompng__private_impl_buffer[ej + 0] = (uint8_t)(idat_crc32 >> 24);
  uncompng__private_impl_buffer[ej + 1] = (uint8_t)(idat_crc32 >> 16);
  uncompng__private_impl_buffer[ej + 2] = (uint8_t)(idat_crc32 >> 8);
  uncompng__private_impl_buffer[ej + 3] = (uint8_t)(idat_crc32 >> 0);
  ej += 4;

  if (!final) {
    int err0 =
        (*write_func)(context, uncompng__private_impl_buffer, (size_t)ej);
    if (err0 != 0) {
      return err0;
    }
    uncompng__private_impl_buffer[0x0004] = 'I';
    uncompng__private_impl_buffer[0x0005] = 'D';
    uncompng__private_impl_buffer[0x0006] = 'A';
    uncompng__private_impl_buffer[0x0007] = 'T';
    return 0;
  }

  static const uint8_t iend_chunk[] = {
      0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
  };

  bool write_separate_iend_chunk = (ej + 12) > 65536;
  if (!write_separate_iend_chunk) {
    for (int i = 0; i < 12; i++) {
      uncompng__private_impl_buffer[ej++] = iend_chunk[i];
    }
  }

  int err1 = (*write_func)(context, uncompng__private_impl_buffer, (size_t)ej);
  if (err1 != 0) {
    return err1;
  }

  if (write_separate_iend_chunk) {
    int err2 = (*write_func)(context, iend_chunk, 12u);
    if (err2 != 0) {
      return err2;
    }
  }

  return 0;
}

static int  //
uncompng__private_impl_do_encode(int (*write_func)(void* context,
                                                   const uint8_t* data_ptr,
                                                   size_t data_len),
                                 void* context,
                                 const uint8_t* pixel_ptr,
                                 size_t pixel_len,
                                 uint32_t width,
                                 uint32_t height,
                                 size_t stride,
                                 uint32_t pixel_format) {
  static const int ei_first = 0x0030;
  static const int ei_later = 0x000D;
  static const int ej_max = 0xFFF8;

  uncompng__private_impl_initialize_buffer(width, height, pixel_format);

  int ej = ei_first;

  for (uint32_t y = 0; y < height; y++) {
    if ((ej + 1) > ej_max) {
      int err = uncompng__private_impl_flush(write_func, context, ej, false);
      if (err != 0) {
        return err;
      }
      ej = ei_later;
    }
    uncompng__private_impl_buffer[ej++] = 0;

    const uint8_t* row = pixel_ptr + (stride * y);

    switch (pixel_format) {
      case UNCOMPNG__PIXEL_FORMAT__Y:
        for (uint32_t x = 0; x < width; x++) {
          if ((ej + 1) > ej_max) {
            int err =
                uncompng__private_impl_flush(write_func, context, ej, false);
            if (err != 0) {
              return err;
            }
            ej = ei_later;
          }
          uncompng__private_impl_buffer[ej++] = row[0];
          row += 1;
        }
        break;

      case UNCOMPNG__PIXEL_FORMAT__Y_16LE:
        for (uint32_t x = 0; x < width; x++) {
          if ((ej + 2) > ej_max) {
            int err =
                uncompng__private_impl_flush(write_func, context, ej, false);
            if (err != 0) {
              return err;
            }
            ej = ei_later;
          }
          uncompng__private_impl_buffer[ej++] = row[1];
          uncompng__private_impl_buffer[ej++] = row[0];
          row += 2;
        }
        break;

      case UNCOMPNG__PIXEL_FORMAT__YXXX:
        for (uint32_t x = 0; x < width; x++) {
          if ((ej + 1) > ej_max) {
            int err =
                uncompng__private_impl_flush(write_func, context, ej, false);
            if (err != 0) {
              return err;
            }
            ej = ei_later;
          }
          uncompng__private_impl_buffer[ej++] = row[0];
          row += 4;
        }
        break;

      case UNCOMPNG__PIXEL_FORMAT__YXXX_4X16LE:
        for (uint32_t x = 0; x < width; x++) {
          if ((ej + 2) > ej_max) {
            int err =
                uncompng__private_impl_flush(write_func, context, ej, false);
            if (err != 0) {
              return err;
            }
            ej = ei_later;
          }
          uncompng__private_impl_buffer[ej++] = row[1];
          uncompng__private_impl_buffer[ej++] = row[0];
          row += 8;
        }
        break;

      case UNCOMPNG__PIXEL_FORMAT__BGRA_NONPREMUL:
        for (uint32_t x = 0; x < width; x++) {
          if ((ej + 4) > ej_max) {
            int err =
                uncompng__private_impl_flush(write_func, context, ej, false);
            if (err != 0) {
              return err;
            }
            ej = ei_later;
          }
          uncompng__private_impl_buffer[ej++] = row[2];
          uncompng__private_impl_buffer[ej++] = row[1];
          uncompng__private_impl_buffer[ej++] = row[0];
          uncompng__private_impl_buffer[ej++] = row[3];
          row += 4;
        }
        break;

      case UNCOMPNG__PIXEL_FORMAT__BGRA_NONPREMUL_4X16LE:
        for (uint32_t x = 0; x < width; x++) {
          if ((ej + 8) > ej_max) {
            int err =
                uncompng__private_impl_flush(write_func, context, ej, false);
            if (err != 0) {
              return err;
            }
            ej = ei_later;
          }
          uncompng__private_impl_buffer[ej++] = row[5];
          uncompng__private_impl_buffer[ej++] = row[4];
          uncompng__private_impl_buffer[ej++] = row[3];
          uncompng__private_impl_buffer[ej++] = row[2];
          uncompng__private_impl_buffer[ej++] = row[1];
          uncompng__private_impl_buffer[ej++] = row[0];
          uncompng__private_impl_buffer[ej++] = row[7];
          uncompng__private_impl_buffer[ej++] = row[6];
          row += 8;
        }
        break;

      case UNCOMPNG__PIXEL_FORMAT__BGRX:
        for (uint32_t x = 0; x < width; x++) {
          if ((ej + 3) > ej_max) {
            int err =
                uncompng__private_impl_flush(write_func, context, ej, false);
            if (err != 0) {
              return err;
            }
            ej = ei_later;
          }
          uncompng__private_impl_buffer[ej++] = row[2];
          uncompng__private_impl_buffer[ej++] = row[1];
          uncompng__private_impl_buffer[ej++] = row[0];
          row += 4;
        }
        break;

      case UNCOMPNG__PIXEL_FORMAT__BGRX_4X16LE:
        for (uint32_t x = 0; x < width; x++) {
          if ((ej + 6) > ej_max) {
            int err =
                uncompng__private_impl_flush(write_func, context, ej, false);
            if (err != 0) {
              return err;
            }
            ej = ei_later;
          }
          uncompng__private_impl_buffer[ej++] = row[5];
          uncompng__private_impl_buffer[ej++] = row[4];
          uncompng__private_impl_buffer[ej++] = row[3];
          uncompng__private_impl_buffer[ej++] = row[2];
          uncompng__private_impl_buffer[ej++] = row[1];
          uncompng__private_impl_buffer[ej++] = row[0];
          row += 8;
        }
        break;

      default:
        return UNCOMPNG__RESULT__INVALID_ARGUMENT;
    }
  }

  return uncompng__private_impl_flush(write_func, context, ej, true);
}

UNCOMPNG__MAYBE_STATIC int  //
uncompng__encode(int (*write_func)(void* context,
                                   const uint8_t* data_ptr,
                                   size_t data_len),
                 void* context,
                 const uint8_t* pixel_ptr,
                 size_t pixel_len,
                 uint32_t width,
                 uint32_t height,
                 size_t stride,
                 uint32_t pixel_format) {
  if (!write_func) {
    return UNCOMPNG__RESULT__INVALID_ARGUMENT;
  }
  uint64_t bytes_per_pixel;
  switch (pixel_format) {
    case UNCOMPNG__PIXEL_FORMAT__Y:
      bytes_per_pixel = 1u;
      break;
    case UNCOMPNG__PIXEL_FORMAT__Y_16LE:
      bytes_per_pixel = 2u;
      break;
    case UNCOMPNG__PIXEL_FORMAT__YXXX:
    case UNCOMPNG__PIXEL_FORMAT__BGRA_NONPREMUL:
    case UNCOMPNG__PIXEL_FORMAT__BGRX:
      bytes_per_pixel = 4u;
      break;
    case UNCOMPNG__PIXEL_FORMAT__YXXX_4X16LE:
    case UNCOMPNG__PIXEL_FORMAT__BGRA_NONPREMUL_4X16LE:
    case UNCOMPNG__PIXEL_FORMAT__BGRX_4X16LE:
      bytes_per_pixel = 8u;
      break;
    default:
      return UNCOMPNG__RESULT__INVALID_ARGUMENT;
  }
  if ((width > 0xFFFFFFu) || (height > 0xFFFFFFu) || (stride >= 0xFFFFFFFFu)) {
    return UNCOMPNG__RESULT__UNSUPPORTED_IMAGE_SIZE;
  }

  if (height > 0u) {
    // This calculation is similar to the one used in
    // wuffs_base__table__flattened_length.
    uint64_t n = ((uint64_t)stride * (uint64_t)(height - 1u)) +
                 (bytes_per_pixel * (uint64_t)width);
    if (pixel_len < n) {
      return UNCOMPNG__RESULT__INVALID_ARGUMENT;
    }
  }

  // uncompng__private_impl_buffer is a global variable in this C code, unlike
  // the original Go code, so try to reject concurrent use. This isn't perfect,
  // as it doesn't use atomics, but it's better than nothing.
  static volatile bool concurrent = false;
  if (concurrent) {
    return UNCOMPNG__RESULT__CONCURRENT_CALL;
  }
  concurrent = true;
  int ret = uncompng__private_impl_do_encode(write_func, context, pixel_ptr,
                                             pixel_len, width, height, stride,
                                             pixel_format);
  concurrent = false;
  return ret;
}

#endif  // UNCOMPNG_IMPLEMENTATION
#endif  // UNCOMPNG_INCLUDE_GUARD
