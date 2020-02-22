// Copyright 2020 The Wuffs Authors.
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

// ----------------

/*
jsonptr is a JSON formatter (pretty-printer).

This example program differs from most other example Wuffs programs in that it
is written in C++, not C.

$CXX jsonptr.cc && ./a.out < ../../test/data/github-tags.json; rm -f a.out

for a C++ compiler $CXX, such as clang++ or g++.
*/

#include <inttypes.h>
#include <stdio.h>

// Wuffs ships as a "single file C library" or "header file library" as per
// https://github.com/nothings/stb/blob/master/docs/stb_howto.txt
//
// To use that single file as a "foo.c"-like implementation, instead of a
// "foo.h"-like header, #define WUFFS_IMPLEMENTATION before #include'ing or
// compiling it.
#define WUFFS_IMPLEMENTATION

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c whitelist which parts of Wuffs to build. That file contains
// the entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__JSON

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C++ file.
#include "../../release/c/wuffs-unsupported-snapshot.c"

#ifndef DST_BUFFER_SIZE
#define DST_BUFFER_SIZE (32 * 1024)
#endif
#ifndef SRC_BUFFER_SIZE
#define SRC_BUFFER_SIZE (32 * 1024)
#endif
#ifndef TOKEN_BUFFER_SIZE
#define TOKEN_BUFFER_SIZE (4 * 1024)
#endif

uint8_t dst_buffer[DST_BUFFER_SIZE];
uint8_t src_buffer[SRC_BUFFER_SIZE];
wuffs_base__token tok_buffer[TOKEN_BUFFER_SIZE];

wuffs_base__io_buffer dst;
wuffs_base__io_buffer src;
wuffs_base__token_buffer tok;

wuffs_json__decoder dec;
wuffs_base__status dec_status;

// dec_current_token_end_src_index is the src.data.ptr index of the end of the
// current token. An invariant is that (dec_current_token_end_src_index <=
// src.meta.ri).
size_t dec_current_token_end_src_index;

#define MAX_INDENT 8
#define INDENT_STRING "        "
size_t indent;

#define TRY(error_msg)         \
  do {                         \
    const char* z = error_msg; \
    if (z) {                   \
      return z;                \
    }                          \
  } while (false)

// ----

const char* read_src() {
  if (src.meta.closed) {
    return "main: read error: unexpected EOF";
  }
  src.compact();
  if (src.meta.wi >= src.data.len) {
    return "main: src buffer is full";
  }
  size_t n = fread(src.data.ptr + src.meta.wi, sizeof(uint8_t),
                   src.data.len - src.meta.wi, stdin);
  src.meta.wi += n;
  src.meta.closed = feof(stdin);
  if ((n == 0) && !src.meta.closed) {
    return "main: read error";
  }
  return nullptr;
}

const char* flush_dst() {
  size_t n = dst.meta.wi - dst.meta.ri;
  if (n > 0) {
    size_t i = fwrite(dst.data.ptr + dst.meta.ri, sizeof(uint8_t), n, stdout);
    dst.meta.ri += i;
    if (i != n) {
      return "main: write error";
    }
    dst.compact();
  }
  return nullptr;
}

const char* write_dst(const void* s, size_t n) {
  const uint8_t* p = static_cast<const uint8_t*>(s);
  while (n > 0) {
    size_t i = dst.writer_available();
    if (i == 0) {
      const char* z = flush_dst();
      if (z) {
        return z;
      }
      i = dst.writer_available();
      if (i == 0) {
        return "main: dst buffer is full";
      }
    }

    if (i > n) {
      i = n;
    }
    memcpy(dst.data.ptr + dst.meta.wi, p, i);
    dst.meta.wi += i;
    p += i;
    n -= i;
  }
  return nullptr;
}

// ----

enum class context {
  none,
  in_list_after_bracket,
  in_list_after_value,
  in_dict_after_brace,
  in_dict_after_key,
  in_dict_after_value,
};

// parsed_token is a result type, combining a wuffs_base_token and an error.
// For the parsed_token returned by make_parsed_token, it also contains the src
// data bytes for the token. This slice is just a view into the src_buffer
// array, and its contents may change on the next call to parse_next_token.
//
// An invariant is that (token.length() == data.len).
typedef struct {
  const char* error_msg;
  wuffs_base__token token;
  wuffs_base__slice_u8 data;
} parsed_token;

parsed_token make_pt_error(const char* error_msg) {
  parsed_token p;
  p.error_msg = error_msg;
  p.token = wuffs_base__make_token(0);
  p.data = wuffs_base__make_slice_u8(nullptr, 0);
  return p;
}

parsed_token make_pt_token(uint64_t token_repr,
                           uint8_t* data_ptr,
                           size_t data_len) {
  parsed_token p;
  p.error_msg = nullptr;
  p.token = wuffs_base__make_token(token_repr);
  p.data = wuffs_base__make_slice_u8(data_ptr, data_len);
  return p;
}

parsed_token parse_next_token() {
  while (true) {
    // Return a previously produced token, if one exists.
    //
    // We do this before checking dec_status. This is analogous to Go's
    // io.Reader's documented idiom, when processing io.Reader.Read's returned
    // (n int, err error), to "process the n > 0 bytes returned before
    // considering the error err. Doing so correctly handles I/O errors that
    // happen after reading some bytes".
    if (tok.meta.ri < tok.meta.wi) {
      wuffs_base__token t = tok.data.ptr[tok.meta.ri++];

      uint64_t n = t.length();
      if ((src.meta.ri - dec_current_token_end_src_index) < n) {
        return make_pt_error("main: internal error: inconsistent src indexes");
      }
      dec_current_token_end_src_index += n;

      // Filter out any filler tokens (e.g. whitespace).
      if (t.value_base_category() == 0) {
        continue;
      }

      return make_pt_token(
          t.repr, src.data.ptr + dec_current_token_end_src_index - n, n);
    }

    // Now consider dec_status.
    if (dec_status.repr == nullptr) {
      return make_pt_error("main: internal error: parser stopped");

    } else if (dec_status.repr == wuffs_base__suspension__short_read) {
      if (dec_current_token_end_src_index != src.meta.ri) {
        return make_pt_error("main: internal error: inconsistent src indexes");
      }
      const char* z = read_src();
      if (z) {
        return make_pt_error(z);
      }
      dec_current_token_end_src_index = src.meta.ri;

    } else if (dec_status.repr == wuffs_base__suspension__short_write) {
      tok.compact();

    } else {
      return make_pt_error(dec_status.message());
    }

    // Retry a "short read" or "short write" suspension.
    dec_status = dec.decode_tokens(&tok, &src);
  }
}

// ----

uint8_t hex_digit(uint8_t nibble) {
  nibble &= 0x0F;
  if (nibble <= 9) {
    return '0' + nibble;
  }
  return ('A' - 10) + nibble;
}

const char* handle_string(parsed_token pt) {
  TRY(write_dst("\"", 1));
  while (true) {
    uint64_t vbc = pt.token.value_base_category();
    uint64_t vbd = pt.token.value_base_detail();

    if (vbc == WUFFS_BASE__TOKEN__VBC__STRING) {
      TRY(write_dst(pt.data.ptr, pt.data.len));
      if ((vbd & WUFFS_BASE__TOKEN__VBD__STRING__INCOMPLETE) == 0) {
        break;
      }

    } else if (vbc != WUFFS_BASE__TOKEN__VBC__UNICODE_CODE_POINT) {
      return "main: unexpected token";

    } else if (vbd < 0x0020) {
      switch (vbd) {
        case '\b':
          TRY(write_dst("\\b", 2));
          break;
        case '\f':
          TRY(write_dst("\\f", 2));
          break;
        case '\n':
          TRY(write_dst("\\n", 2));
          break;
        case '\r':
          TRY(write_dst("\\r", 2));
          break;
        case '\t':
          TRY(write_dst("\\t", 2));
          break;
        default: {
          // Other bytes less than 0x0020 are valid UTF-8 but not valid in a
          // JSON string. They need to remain escaped.
          uint8_t esc6[6];
          esc6[0] = '\\';
          esc6[1] = 'u';
          esc6[2] = '0';
          esc6[3] = '0';
          esc6[4] = hex_digit(vbd >> 4);
          esc6[5] = hex_digit(vbd >> 0);
          TRY(write_dst(&esc6[0], 6));
          break;
        }
      }

    } else if (vbd <= 0x007F) {
      switch (vbd) {
        case '\"':
          TRY(write_dst("\\\"", 2));
          break;
        case '\\':
          TRY(write_dst("\\\\", 2));
          break;
        default: {
          // The UTF-8 encoding takes 1 byte.
          uint8_t esc0 = (uint8_t)(vbd);
          TRY(write_dst(&esc0, 1));
          break;
        }
      }

    } else if (vbd <= 0x07FF) {
      // The UTF-8 encoding takes 2 bytes.
      uint8_t esc2[6];
      esc2[0] = 0xC0 | (uint8_t)((vbd >> 6));
      esc2[1] = 0x80 | (uint8_t)((vbd >> 0) & 0x3F);
      TRY(write_dst(&esc2[0], 2));

    } else if (vbd <= 0xFFFF) {
      // The UTF-8 encoding takes 3 bytes.
      uint8_t esc3[6];
      esc3[0] = 0xE0 | (uint8_t)((vbd >> 12));
      esc3[1] = 0x80 | (uint8_t)((vbd >> 6) & 0x3F);
      esc3[2] = 0x80 | (uint8_t)((vbd >> 0) & 0x3F);
      TRY(write_dst(&esc3[0], 3));

    } else {
      return "main: unexpected Unicode code point";
    }

    pt = parse_next_token();
    if (pt.error_msg) {
      return pt.error_msg;
    }
  }
  TRY(write_dst("\"", 1));
  return nullptr;
}

const char* main2() {
  dec_status = dec.initialize(sizeof__wuffs_json__decoder(), WUFFS_VERSION, 0);
  if (!dec_status.is_ok()) {
    return dec_status.message();
  }
  dec_status = dec.decode_tokens(&tok, &src);
  dec_current_token_end_src_index = 0;

  uint64_t depth = 0;
  context ctx = context::none;

continue_loop:
  while (true) {
    parsed_token pt = parse_next_token();
    if (pt.error_msg) {
      return pt.error_msg;
    }
    uint64_t vbc = pt.token.value_base_category();
    uint64_t vbd = pt.token.value_base_detail();

    // Handle ']' or '}'.
    if ((vbc == WUFFS_BASE__TOKEN__VBC__STRUCTURE) &&
        ((vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__POP) != 0)) {
      if (depth <= 0) {
        return "main: internal error: inconsistent depth";
      }
      depth--;

      // Write preceding whitespace.
      if ((ctx != context::in_list_after_bracket) &&
          (ctx != context::in_dict_after_brace)) {
        TRY(write_dst("\n", 1));
        for (size_t i = 0; i < depth; i++) {
          TRY(write_dst(INDENT_STRING, indent));
        }
      }

      TRY(write_dst(
          (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_LIST) ? "]" : "}", 1));
      ctx = (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST)
                ? context::in_list_after_value
                : context::in_dict_after_key;
      goto after_value;
    }

    // Write preceding whitespace and punctuation, if it wasn't ']' or '}'.
    if (ctx == context::in_dict_after_key) {
      TRY(write_dst(": ", 2));
    } else if (ctx != context::none) {
      if ((ctx != context::in_list_after_bracket) &&
          (ctx != context::in_dict_after_brace)) {
        TRY(write_dst(",", 1));
      }
      TRY(write_dst("\n", 1));
      for (size_t i = 0; i < depth; i++) {
        TRY(write_dst(INDENT_STRING, indent));
      }
    }

    // Handle the token itself: either a container ('[' or '{') or a simple
    // value (number, string or literal).
    switch (vbc) {
      case WUFFS_BASE__TOKEN__VBC__STRUCTURE:
        TRY(write_dst(
            (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST) ? "[" : "{", 1));
        depth++;
        ctx = (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST)
                  ? context::in_list_after_bracket
                  : context::in_dict_after_brace;
        goto continue_loop;

      case WUFFS_BASE__TOKEN__VBC__NUMBER:
        TRY(write_dst(pt.data.ptr, pt.data.len));
        goto after_value;

      case WUFFS_BASE__TOKEN__VBC__STRING:
        TRY(handle_string(pt));
        goto after_value;
    }

    // Return an error if we didn't match the (vbc, vbd) pair.
    return "main: unexpected token";

    // Book-keeping after completing a value (whether a container value or a
    // simple value). Empty parent containers are no longer empty. If the
    // parent container is a "{...}" object, toggle between keys and values.
  after_value:
    if (depth <= 0) {
      return nullptr;
    }
    switch (ctx) {
      case context::in_list_after_bracket:
        ctx = context::in_list_after_value;
        break;
      case context::in_dict_after_brace:
        ctx = context::in_dict_after_key;
        break;
      case context::in_dict_after_key:
        ctx = context::in_dict_after_value;
        break;
      case context::in_dict_after_value:
        ctx = context::in_dict_after_key;
        break;
    }
  }
}

const char* main1(int argc, char** argv) {
  dst = wuffs_base__make_io_buffer(
      wuffs_base__make_slice_u8(dst_buffer, DST_BUFFER_SIZE),
      wuffs_base__empty_io_buffer_meta());

  src = wuffs_base__make_io_buffer(
      wuffs_base__make_slice_u8(src_buffer, SRC_BUFFER_SIZE),
      wuffs_base__empty_io_buffer_meta());

  tok = wuffs_base__make_token_buffer(
      wuffs_base__make_slice_token(tok_buffer, TOKEN_BUFFER_SIZE),
      wuffs_base__empty_token_buffer_meta());

  indent = 4;

  TRY(main2());
  TRY(write_dst("\n", 1));
  return nullptr;
}

int main(int argc, char** argv) {
  const char* z0 = main1(argc, argv);
  const char* z1 = flush_dst();
  const char* z = z0 ? z0 : z1;
  if (z) {
    fprintf(stderr, "%s\n", z);
    return 1;
  }
  return 0;
}
