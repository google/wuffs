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

As of 2020-02-24, this program passes all 318 "test_parsing" cases from the
JSON test suite (https://github.com/nst/JSONTestSuite), an appendix to the
"Parsing JSON is a Minefield" article (http://seriot.ch/parsing_json.php) that
was first published on 2016-10-26 and updated on 2018-03-30.

This example program differs from most other example Wuffs programs in that it
is written in C++, not C.

$CXX jsonptr.cc && ./a.out < ../../test/data/github-tags.json; rm -f a.out

for a C++ compiler $CXX, such as clang++ or g++.

After modifying this program, run "build-example.sh example/jsonptr/" and then
"script/run-json-test-suite.sh" to catch correctness regressions.
*/

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

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

#define TRY(error_msg)         \
  do {                         \
    const char* z = error_msg; \
    if (z) {                   \
      return z;                \
    }                          \
  } while (false)

static const char* eod = "main: end of data";

// ----

#define MAX_INDENT 8
#define INDENT_SPACES_STRING "        "
#define INDENT_TABS_STRING "\t\t\t\t\t\t\t\t"

#ifndef DST_BUFFER_SIZE
#define DST_BUFFER_SIZE (32 * 1024)
#endif
#ifndef SRC_BUFFER_SIZE
#define SRC_BUFFER_SIZE (32 * 1024)
#endif
#ifndef TOKEN_BUFFER_SIZE
#define TOKEN_BUFFER_SIZE (4 * 1024)
#endif

uint8_t dst_array[DST_BUFFER_SIZE];
uint8_t src_array[SRC_BUFFER_SIZE];
wuffs_base__token tok_array[TOKEN_BUFFER_SIZE];

wuffs_base__io_buffer dst;
wuffs_base__io_buffer src;
wuffs_base__token_buffer tok;

// curr_token_end_src_index is the src.data.ptr index of the end of the current
// token. An invariant is that (curr_token_end_src_index <= src.meta.ri).
size_t curr_token_end_src_index;

uint64_t depth;

enum class context {
  none,
  in_list_after_bracket,
  in_list_after_value,
  in_dict_after_brace,
  in_dict_after_key,
  in_dict_after_value,
} ctx;

wuffs_json__decoder dec;

struct {
  int remaining_argc;
  char** remaining_argv;

  bool compact;
  size_t indent;
  bool tabs;
} flags = {0};

const char*  //
parse_flags(int argc, char** argv) {
  bool explicit_indent = false;

  int c = (argc > 0) ? 1 : 0;  // Skip argv[0], the program name.
  for (; c < argc; c++) {
    char* arg = argv[c];
    if (*arg++ != '-') {
      break;
    }

    // A double-dash "--foo" is equivalent to a single-dash "-foo". As special
    // cases, a bare "-" is not a flag (some programs may interpret it as
    // stdin) and a bare "--" means to stop parsing flags.
    if (*arg == '\x00') {
      break;
    } else if (*arg == '-') {
      arg++;
      if (*arg == '\x00') {
        c++;
        break;
      }
    }

    if (!strcmp(arg, "c") || !strcmp(arg, "compact")) {
      flags.compact = true;
      continue;
    }
    if (!strncmp(arg, "i=", 2) || !strncmp(arg, "indent=", 7)) {
      while (*arg++ != '=') {
      }
      if (('0' <= arg[0]) && (arg[0] <= '8') && (arg[1] == '\x00')) {
        flags.indent = arg[0] - '0';
        explicit_indent = true;
        continue;
      }
    }
    if (!strcmp(arg, "t") || !strcmp(arg, "tabs")) {
      flags.tabs = true;
      continue;
    }

    return "main: unrecognized flag argument";
  }

  flags.remaining_argc = argc - c;
  flags.remaining_argv = argv + c;
  if (!explicit_indent) {
    flags.indent = flags.tabs ? 1 : 4;
  }
  return NULL;
}

const char*  //
initialize_globals(int argc, char** argv) {
  dst = wuffs_base__make_io_buffer(
      wuffs_base__make_slice_u8(dst_array, DST_BUFFER_SIZE),
      wuffs_base__empty_io_buffer_meta());

  src = wuffs_base__make_io_buffer(
      wuffs_base__make_slice_u8(src_array, SRC_BUFFER_SIZE),
      wuffs_base__empty_io_buffer_meta());

  tok = wuffs_base__make_token_buffer(
      wuffs_base__make_slice_token(tok_array, TOKEN_BUFFER_SIZE),
      wuffs_base__empty_token_buffer_meta());

  curr_token_end_src_index = 0;

  depth = 0;

  ctx = context::none;

  TRY(parse_flags(argc, argv));
  if (flags.remaining_argc > 0) {
    return "main: bad argument: use \"program < input\", not \"program input\"";
  }

  return dec.initialize(sizeof__wuffs_json__decoder(), WUFFS_VERSION, 0)
      .message();
}

// ----

const char*  //
read_src() {
  if (src.meta.closed) {
    return "main: internal error: read requested on a closed source";
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

const char*  //
flush_dst() {
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

const char*  //
write_dst(const void* s, size_t n) {
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

uint8_t  //
hex_digit(uint8_t nibble) {
  nibble &= 0x0F;
  if (nibble <= 9) {
    return '0' + nibble;
  }
  return ('A' - 10) + nibble;
}

const char*  //
handle_unicode_code_point(uint32_t ucp) {
  if (ucp < 0x0020) {
    switch (ucp) {
      case '\b':
        return write_dst("\\b", 2);
      case '\f':
        return write_dst("\\f", 2);
      case '\n':
        return write_dst("\\n", 2);
      case '\r':
        return write_dst("\\r", 2);
      case '\t':
        return write_dst("\\t", 2);
      default: {
        // Other bytes less than 0x0020 are valid UTF-8 but not valid in a
        // JSON string. They need to remain escaped.
        uint8_t esc6[6];
        esc6[0] = '\\';
        esc6[1] = 'u';
        esc6[2] = '0';
        esc6[3] = '0';
        esc6[4] = hex_digit(ucp >> 4);
        esc6[5] = hex_digit(ucp >> 0);
        return write_dst(&esc6[0], 6);
      }
    }

  } else if (ucp <= 0x007F) {
    switch (ucp) {
      case '\"':
        return write_dst("\\\"", 2);
      case '\\':
        return write_dst("\\\\", 2);
      default: {
        // The UTF-8 encoding takes 1 byte.
        uint8_t esc0 = (uint8_t)(ucp);
        return write_dst(&esc0, 1);
      }
    }

  } else if (ucp <= 0x07FF) {
    // The UTF-8 encoding takes 2 bytes.
    uint8_t esc2[2];
    esc2[0] = 0xC0 | (uint8_t)((ucp >> 6));
    esc2[1] = 0x80 | (uint8_t)((ucp >> 0) & 0x3F);
    return write_dst(&esc2[0], 2);

  } else if (ucp <= 0xFFFF) {
    if ((0xD800 <= ucp) && (ucp <= 0xDFFF)) {
      return "main: internal error: unexpected Unicode surrogate";
    }
    // The UTF-8 encoding takes 3 bytes.
    uint8_t esc3[3];
    esc3[0] = 0xE0 | (uint8_t)((ucp >> 12));
    esc3[1] = 0x80 | (uint8_t)((ucp >> 6) & 0x3F);
    esc3[2] = 0x80 | (uint8_t)((ucp >> 0) & 0x3F);
    return write_dst(&esc3[0], 3);

  } else if (ucp <= 0x10FFFF) {
    // The UTF-8 encoding takes 4 bytes.
    uint8_t esc4[4];
    esc4[0] = 0xF0 | (uint8_t)((ucp >> 18));
    esc4[1] = 0x80 | (uint8_t)((ucp >> 12) & 0x3F);
    esc4[2] = 0x80 | (uint8_t)((ucp >> 6) & 0x3F);
    esc4[3] = 0x80 | (uint8_t)((ucp >> 0) & 0x3F);
    return write_dst(&esc4[0], 4);
  }

  return "main: internal error: unexpected Unicode code point";
}

const char*  //
handle_token(wuffs_base__token t) {
  do {
    uint64_t vbc = t.value_base_category();
    uint64_t vbd = t.value_base_detail();
    uint64_t len = t.length();

    // Handle ']' or '}'.
    if ((vbc == WUFFS_BASE__TOKEN__VBC__STRUCTURE) &&
        (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__POP)) {
      if (depth <= 0) {
        return "main: internal error: inconsistent depth";
      }
      depth--;

      // Write preceding whitespace.
      if ((ctx != context::in_list_after_bracket) &&
          (ctx != context::in_dict_after_brace) && !flags.compact) {
        TRY(write_dst("\n", 1));
        for (size_t i = 0; i < depth; i++) {
          TRY(write_dst(flags.tabs ? INDENT_TABS_STRING : INDENT_SPACES_STRING,
                        flags.indent));
        }
      }

      TRY(write_dst(
          (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_LIST) ? "]" : "}", 1));
      ctx = (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST)
                ? context::in_list_after_value
                : context::in_dict_after_key;
      goto after_value;
    }

    // Write preceding whitespace and punctuation, if it wasn't ']', '}' or a
    // continuation of a multi-token chain.
    if (t.link_prev()) {
      // No-op.
    } else if (ctx == context::in_dict_after_key) {
      TRY(write_dst(": ", flags.compact ? 1 : 2));
    } else if (ctx != context::none) {
      if ((ctx != context::in_list_after_bracket) &&
          (ctx != context::in_dict_after_brace)) {
        TRY(write_dst(",", 1));
      }
      if (!flags.compact) {
        TRY(write_dst("\n", 1));
        for (size_t i = 0; i < depth; i++) {
          TRY(write_dst(flags.tabs ? INDENT_TABS_STRING : INDENT_SPACES_STRING,
                        flags.indent));
        }
      }
    }

    // Handle the token itself: either a container ('[' or '{') or a simple
    // value: string (a chain of raw or escaped parts), literal or number.
    switch (vbc) {
      case WUFFS_BASE__TOKEN__VBC__STRUCTURE:
        TRY(write_dst(
            (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST) ? "[" : "{", 1));
        depth++;
        ctx = (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST)
                  ? context::in_list_after_bracket
                  : context::in_dict_after_brace;
        return nullptr;

      case WUFFS_BASE__TOKEN__VBC__STRING:
        if (!t.link_prev()) {
          TRY(write_dst("\"", 1));
        }

        if (vbd & WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_0_DST_1_SRC_DROP) {
          // No-op.
        } else if (vbd &
                   WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_1_DST_1_SRC_COPY) {
          TRY(write_dst(src.data.ptr + curr_token_end_src_index - len, len));
        } else {
          return "main: internal error: unexpected string-token conversion";
        }

        if (t.link_next()) {
          return nullptr;
        }
        TRY(write_dst("\"", 1));
        goto after_value;

      case WUFFS_BASE__TOKEN__VBC__UNICODE_CODE_POINT:
        return handle_unicode_code_point(vbd);

      case WUFFS_BASE__TOKEN__VBC__LITERAL:
      case WUFFS_BASE__TOKEN__VBC__NUMBER:
        TRY(write_dst(src.data.ptr + curr_token_end_src_index - len, len));
        goto after_value;
    }

    // Return an error if we didn't match the (vbc, vbd) pair.
    return "main: internal error: unexpected token";
  } while (0);

  // Book-keeping after completing a value (whether a container value or a
  // simple value). Empty parent containers are no longer empty. If the parent
  // container is a "{...}" object, toggle between keys and values.
after_value:
  if (depth == 0) {
    return eod;
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
  return nullptr;
}

const char*  //
main1(int argc, char** argv) {
  TRY(initialize_globals(argc, argv));

  while (true) {
    wuffs_base__status status = dec.decode_tokens(&tok, &src);

    while (tok.meta.ri < tok.meta.wi) {
      wuffs_base__token t = tok.data.ptr[tok.meta.ri++];
      uint64_t n = t.length();
      if ((src.meta.ri - curr_token_end_src_index) < n) {
        return "main: internal error: inconsistent src indexes";
      }
      curr_token_end_src_index += n;

      if (t.value() == 0) {
        continue;
      }

      const char* z = handle_token(t);
      if (z == nullptr) {
        continue;
      } else if (z == eod) {
        break;
      }
      return z;
    }

    if (status.repr == nullptr) {
      break;
    } else if (status.repr == wuffs_base__suspension__short_read) {
      if (curr_token_end_src_index != src.meta.ri) {
        return "main: internal error: inconsistent src indexes";
      }
      TRY(read_src());
      curr_token_end_src_index = src.meta.ri;
    } else if (status.repr == wuffs_base__suspension__short_write) {
      tok.compact();
    } else {
      return status.message();
    }
  }

  // Consume an optional whitespace trailer. This isn't part of the JSON spec,
  // but it works better with line oriented Unix tools (such as "echo 123 |
  // jsonptr" where it's "echo", not "echo -n") or hand-edited JSON files which
  // can accidentally contain trailing whitespace.
  //
  // A whitespace trailer is zero or more ' ' and then zero or one '\n'.
  while (true) {
    if (src.meta.ri < src.meta.wi) {
      uint8_t c = src.data.ptr[src.meta.ri];
      if (c == ' ') {
        src.meta.ri++;
        continue;
      } else if (c == '\n') {
        src.meta.ri++;
        break;
      }
      // The "exhausted the input" check below will fail.
      break;
    } else if (src.meta.closed) {
      break;
    }
    TRY(read_src());
  }

  // Check that we've exhausted the input.
  if ((src.meta.ri < src.meta.wi) || !src.meta.closed) {
    return "main: valid JSON followed by further (unexpected) data";
  }

  // Check that we've used all of the decoded tokens, other than trailing
  // filler tokens. For example, a bare `"foo"` string is valid JSON, but even
  // without a trailing '\n', the Wuffs JSON parser emits a filler token for
  // the final '\"'.
  for (; tok.meta.ri < tok.meta.wi; tok.meta.ri++) {
    if (tok.data.ptr[tok.meta.ri].value_base_category() !=
        WUFFS_BASE__TOKEN__VBC__FILLER) {
      return "main: internal error: decoded OK but unprocessed tokens remain";
    }
  }

  return nullptr;
}

int  //
compute_exit_code(const char* status_msg) {
  if (!status_msg) {
    return 0;
  }
  size_t n = strnlen(status_msg, 2047);
  if (n >= 2047) {
    status_msg = "main: internal error: error message is too long";
    n = strnlen(status_msg, 2047);
  }
  fprintf(stderr, "%s\n", status_msg);
  // Return an exit code of 1 for regular (forseen) errors, e.g. badly
  // formatted or unsupported input.
  //
  // Return an exit code of 2 for internal (exceptional) errors, e.g. defensive
  // run-time checks found that an internal invariant did not hold.
  //
  // Automated testing, including badly formatted inputs, can therefore
  // discriminate between expected failure (exit code 1) and unexpected failure
  // (other non-zero exit codes). Specifically, exit code 2 for internal
  // invariant violation, exit code 139 (which is 128 + SIGSEGV on x86_64
  // linux) for a segmentation fault (e.g. null pointer dereference).
  return strstr(status_msg, "internal error:") ? 2 : 1;
}

int  //
main(int argc, char** argv) {
  const char* z0 = main1(argc, argv);
  const char* z1 = write_dst("\n", 1);
  const char* z2 = flush_dst();
  int exit_code = compute_exit_code(z0 ? z0 : (z1 ? z1 : z2));
  return exit_code;
}
