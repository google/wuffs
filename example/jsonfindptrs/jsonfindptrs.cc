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
jsonfindptrs reads UTF-8 JSON from stdin and writes every node's JSON Pointer
(RFC 6901) to stdout.

See the "const char* g_usage" string below for details.

----

This program uses Wuffs' JSON decoder at a relatively high level, building
in-memory representations of JSON 'things' (e.g. numbers, strings, objects).
After the entire input has been converted, walking the tree prints the output
(in sorted order). The core conversion mechanism is to call JsonThing::parse,
which consumes a variable number of tokens (the output of Wuffs' JSON decoder).
JsonThing::parse can call itself recursively, as JSON values can nest.

This approach is centered around JSON things. Each JSON thing comprises one or
more JSON tokens.

An alternative, lower-level approach is in the sibling example/jsonptr program.
Neither approach is better or worse per se, but when studying this program, be
aware that there are multiple ways to use Wuffs' JSON decoder.

The two programs, jsonfindptrs and jsonptr, also demonstrate different
trade-offs with regard to JSON object duplicate keys. The JSON spec permits
different implementations to allow or reject duplicate keys. It is not always
clear which approach is safer. Rejecting them is certainly unambiguous, and
security bugs can lurk in ambiguous corners of a file format, if two different
implementations both silently accept a file but differ on how to interpret it.
On the other hand, in the worst case, detecting duplicate keys requires O(N)
memory, where N is the size of the (potentially untrusted) input.

This program (jsonfindptrs) rejects duplicate keys.

----

This example program differs from most other example Wuffs programs in that it
is written in C++, not C.

$CXX jsonfindptrs.cc && ./a.out < ../../test/data/github-tags.json; rm -f a.out

for a C++ compiler $CXX, such as clang++ or g++.
*/

#if defined(__cplusplus) && (__cplusplus < 201103L)
#error "This C++ program requires -std=c++11 or later"
#endif

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

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
    std::string z = error_msg; \
    if (!z.empty()) {          \
      return z;                \
    }                          \
  } while (false)

static const char* g_usage =
    "Usage: jsonfindptrs -flags input.json\n"
    "\n"
    "Flags:\n"
    "    -d=NUM  -max-output-depth=NUM\n"
    "    -s      -strict-json-pointer-syntax\n"
    "\n"
    "The input.json filename is optional. If absent, it reads from stdin.\n"
    "\n"
    "----\n"
    "\n"
    "jsonfindptrs reads UTF-8 JSON from stdin and writes every node's JSON\n"
    "Pointer (RFC 6901) to stdout.\n"
    "\n"
    "For example, given RFC 6901 section 5's sample input\n"
    "(https://tools.ietf.org/rfc/rfc6901.txt), this command:\n"
    "    jsonfindptrs rfc-6901-json-pointer.json\n"
    "will print:\n"
    "    \n"
    "    /\n"
    "    / \n"
    "    /a~1b\n"
    "    /c%d\n"
    "    /e^f\n"
    "    /foo\n"
    "    /foo/0\n"
    "    /foo/1\n"
    "    /g|h\n"
    "    /i\\j\n"
    "    /k\"l\n"
    "    /m~0n\n"
    "\n"
    "The first three lines are (1) a 0-byte \"\", (2) a 1-byte \"/\" and (3)\n"
    "a 2-byte \"/ \". Unlike a file system, the \"/\" JSON Pointer does not\n"
    "identify the root. Instead, \"\" is the root and \"/\" is the child (the\n"
    "value in a key-value pair) of the root whose key is the empty string.\n"
    "Similarly, \"/xyz\" and \"/xyz/\" are two different nodes.\n"
    "\n"
    "----\n"
    "\n"
    "The JSON specification (https://json.org/) permits implementations that\n"
    "allow duplicate keys, but this one does not. Conversely, it prints keys\n"
    "in sorted order, but the overall output is not necessarily sorted\n"
    "lexicographically. For example, \"/a/9\" would come before \"/a/10\",\n"
    "and \"/b/c\", a child of \"/b\", would come before \"/b+\".\n"
    "\n"
    "This JSON implementation also rejects integer values outside ±M, where\n"
    "M is ((1<<53)-1), also known as JavaScript's Number.MAX_SAFE_INTEGER.\n"
    "\n"
    "----\n"
    "\n"
    "The -s or -strict-json-pointer-syntax flag restricts the output lines\n"
    "to exactly RFC 6901, with only two escape sequences: \"~0\" and \"~1\"\n"
    "for \"~\" and \"/\". Without this flag, this program also lets \"~n\"\n"
    "and \"~r\" escape the New Line and Carriage Return ASCII control\n"
    "characters, which can work better with line oriented Unix tools that\n"
    "assume exactly one value (i.e. one JSON Pointer string) per line. With\n"
    "this flag, the program will fail if the input JSON's object keys contain\n"
    "\"\\u000A\" or \"\\u000D\".\n"
    "\n"
    "----\n"
    "\n"
    "The JSON specification permits implementations to set their own maximum\n"
    "input depth. This JSON implementation sets it to 1024.\n"
    "\n"
    "The -d=NUM or -max-output-depth=NUM flag gives the maximum (inclusive)\n"
    "output depth. JSON containers ([] arrays and {} objects) can hold other\n"
    "containers. A bare -d or -max-output-depth is equivalent to -d=1,\n"
    "analogous to the Unix ls command. The flag's absence is equivalent to an\n"
    "unlimited output depth, analogous to the Unix find command (and hence\n"
    "the name of this program: jsonfindptrs).";

// ----

struct {
  int remaining_argc;
  char** remaining_argv;

  uint32_t max_output_depth;
  bool strict_json_pointer_syntax;
} g_flags = {0};

std::string  //
parse_flags(int argc, char** argv) {
  g_flags.max_output_depth = 0xFFFFFFFF;

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

    if (!strcmp(arg, "d") || !strcmp(arg, "max-output-depth")) {
      g_flags.max_output_depth = 1;
      continue;
    } else if (!strncmp(arg, "d=", 2) ||
               !strncmp(arg, "max-output-depth=", 16)) {
      while (*arg++ != '=') {
      }
      wuffs_base__result_u64 u = wuffs_base__parse_number_u64(
          wuffs_base__make_slice_u8((uint8_t*)arg, strlen(arg)));
      if (wuffs_base__status__is_ok(&u.status) && (u.value <= 0xFFFFFFFF)) {
        g_flags.max_output_depth = (uint32_t)(u.value);
        continue;
      }
      return g_usage;
    }
    if (!strcmp(arg, "s") || !strcmp(arg, "strict-json-pointer-syntax")) {
      g_flags.strict_json_pointer_syntax = true;
      continue;
    }

    return g_usage;
  }

  g_flags.remaining_argc = argc - c;
  g_flags.remaining_argv = argv + c;
  return "";
}

  // ----

#define WORK_BUFFER_ARRAY_SIZE \
  WUFFS_JSON__DECODER_WORKBUF_LEN_MAX_INCL_WORST_CASE

#ifndef SRC_BUFFER_ARRAY_SIZE
#define SRC_BUFFER_ARRAY_SIZE (4 * 1024)
#endif
#ifndef TOKEN_BUFFER_ARRAY_SIZE
#define TOKEN_BUFFER_ARRAY_SIZE (1 * 1024)
#endif

class TokenStream {
 public:
  struct Result {
    std::string status_msg;
    wuffs_base__token token;
    // src_data is a sub-slice of m_src (a slice is a pointer-length pair).
    // Calling TokenStream::peek or TokenStream::next may change the backing
    // array's contents, so handling a TokenStream::Result may require copying
    // this src_data slice's contents.
    wuffs_base__slice_u8 src_data;

    Result(std::string s)
        : status_msg(s),
          token(wuffs_base__make_token(0)),
          src_data(wuffs_base__empty_slice_u8()) {}

    Result(std::string s, wuffs_base__token t, wuffs_base__slice_u8 d)
        : status_msg(s), token(t), src_data(d) {}
  };

  TokenStream(int input_file_descriptor)
      : m_status(wuffs_base__make_status(nullptr)),
        m_src(wuffs_base__make_io_buffer(
            wuffs_base__make_slice_u8(m_src_array, SRC_BUFFER_ARRAY_SIZE),
            wuffs_base__empty_io_buffer_meta())),
        m_tok(wuffs_base__make_token_buffer(
            wuffs_base__make_slice_token(m_tok_array, TOKEN_BUFFER_ARRAY_SIZE),
            wuffs_base__empty_token_buffer_meta())),
        m_input_file_descriptor(input_file_descriptor),
        m_curr_token_end_src_index(0) {
    m_status =
        m_dec.initialize(sizeof__wuffs_json__decoder(), WUFFS_VERSION, 0);

    // Uncomment these lines to enable the WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_X
    // option, discussed in a separate comment.
    //
    // if (m_status.is_ok()) {
    //   m_dec.set_quirk_enabled(WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_X, true);
    // }
  }

  Result peek() { return peek_or_next(false); }
  Result next() { return peek_or_next(true); }

 private:
  Result peek_or_next(bool next) {
    while (m_tok.meta.ri >= m_tok.meta.wi) {
      if (m_status.repr == nullptr) {
        // No-op.
      } else if (m_status.repr == wuffs_base__suspension__short_read) {
        if (m_curr_token_end_src_index != m_src.meta.ri) {
          return Result(
              "TokenStream: internal error: inconsistent src indexes");
        }
        const char* z = read_src();
        m_curr_token_end_src_index = m_src.meta.ri;
        if (z) {
          return Result(z);
        }
      } else if (m_status.repr == wuffs_base__suspension__short_write) {
        m_tok.compact();
      } else {
        return Result(m_status.message());
      }

      m_status =
          m_dec.decode_tokens(&m_tok, &m_src,
                              wuffs_base__make_slice_u8(
                                  m_work_buffer_array, WORK_BUFFER_ARRAY_SIZE));
    }

    wuffs_base__token t = m_tok.data.ptr[m_tok.meta.ri];
    size_t i = m_curr_token_end_src_index;
    uint64_t n = t.length();
    if ((m_src.meta.ri < i) || ((m_src.meta.ri - i) < n)) {
      return Result("TokenStream: internal error: inconsistent src indexes");
    }
    if (next) {
      m_tok.meta.ri++;
      m_curr_token_end_src_index += n;
    }
    return Result("", t, wuffs_base__make_slice_u8(m_src.data.ptr + i, n));
  }

  const char*  //
  read_src() {
    if (m_src.meta.closed) {
      return "main: internal error: read requested on a closed source";
    }
    m_src.compact();
    if (m_src.meta.wi >= m_src.data.len) {
      return "main: src buffer is full";
    }
    while (true) {
      ssize_t n = read(m_input_file_descriptor, m_src.data.ptr + m_src.meta.wi,
                       m_src.data.len - m_src.meta.wi);
      if (n >= 0) {
        m_src.meta.wi += n;
        m_src.meta.closed = n == 0;
        break;
      } else if (errno != EINTR) {
        return strerror(errno);
      }
    }
    return nullptr;
  }

  wuffs_base__status m_status;
  wuffs_base__io_buffer m_src;
  wuffs_base__token_buffer m_tok;
  int m_input_file_descriptor;
  // m_curr_token_end_src_index is the m_src.data.ptr index of the end of the
  // current token. An invariant is that (m_curr_token_end_src_index <=
  // m_src.meta.ri).
  size_t m_curr_token_end_src_index;

  wuffs_base__token m_tok_array[TOKEN_BUFFER_ARRAY_SIZE];
  uint8_t m_src_array[SRC_BUFFER_ARRAY_SIZE];
#if WORK_BUFFER_ARRAY_SIZE > 0
  uint8_t m_work_buffer_array[WORK_BUFFER_ARRAY_SIZE];
#else
  // Not all C/C++ compilers support 0-length arrays.
  uint8_t m_work_buffer_array[1];
#endif
  wuffs_json__decoder m_dec;
};

// ----

class JsonThing {
 public:
  struct Result;

  using Vector = std::vector<JsonThing>;

  // We use a std::map in this example program to avoid dependencies outside of
  // the C++ standard library. If you're copy/pasting this JsonThing code,
  // consider a more efficient data structure such as an absl::btree_map.
  //
  // See CppCon 2014: Chandler Carruth "Efficiency with Algorithms, Performance
  // with Data Structures" at https://www.youtube.com/watch?v=fHNmRkzxHWs
  using Map = std::map<std::string, JsonThing>;

  enum class Kind {
    Null,
    Bool,
    Int64,
    Float64,
    String,
    Array,
    Object,
  } kind = Kind::Null;

  struct Value {
    bool b = false;
    int64_t i = 0;
    double f = 0;
    std::string s;
    Vector a;
    Map o;
  } value;

  static JsonThing::Result parse(TokenStream& ts);

 private:
  static JsonThing::Result parse_array(TokenStream& ts);
  static JsonThing::Result parse_literal(TokenStream::Result tsr);
  static JsonThing::Result parse_number(TokenStream::Result tsr);
  static JsonThing::Result parse_object(TokenStream& ts);
  static JsonThing::Result parse_string(TokenStream& ts,
                                        TokenStream::Result tsr);
};

struct JsonThing::Result {
  std::string status_msg;
  JsonThing thing;

  Result(std::string s) : status_msg(s), thing(JsonThing()) {}

  Result(std::string s, JsonThing t) : status_msg(s), thing(t) {}
};

JsonThing::Result  //
JsonThing::parse(TokenStream& ts) {
  while (true) {
    TokenStream::Result tsr = ts.next();
    if (!tsr.status_msg.empty()) {
      return Result(std::move(tsr.status_msg));
    }

    int64_t vbc = tsr.token.value_base_category();
    uint64_t vbd = tsr.token.value_base_detail();
    switch (vbc) {
      case WUFFS_BASE__TOKEN__VBC__FILLER:
        continue;
      case WUFFS_BASE__TOKEN__VBC__STRUCTURE:
        if (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__PUSH) {
          if (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST) {
            return parse_array(ts);
          } else if (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_DICT) {
            return parse_object(ts);
          }
        }
        break;
      case WUFFS_BASE__TOKEN__VBC__STRING:
        return parse_string(ts, tsr);
      case WUFFS_BASE__TOKEN__VBC__LITERAL:
        return parse_literal(tsr);
      case WUFFS_BASE__TOKEN__VBC__NUMBER:
        return parse_number(tsr);
    }

    return Result("main: internal error: unexpected token");
  }
}

JsonThing::Result  //
JsonThing::parse_array(TokenStream& ts) {
  JsonThing jt;
  jt.kind = Kind::Array;
  while (true) {
    TokenStream::Result tsr = ts.peek();
    if (!tsr.status_msg.empty()) {
      return Result(std::move(tsr.status_msg));
    }
    int64_t vbc = tsr.token.value_base_category();
    uint64_t vbd = tsr.token.value_base_detail();
    if (vbc == WUFFS_BASE__TOKEN__VBC__FILLER) {
      ts.next();
      continue;
    } else if ((vbc == WUFFS_BASE__TOKEN__VBC__STRUCTURE) &&
               (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__POP)) {
      ts.next();
      break;
    }

    JsonThing::Result jtr = JsonThing::parse(ts);
    if (!jtr.status_msg.empty()) {
      return Result(std::move(jtr.status_msg));
    }
    jt.value.a.push_back(std::move(jtr.thing));
  }
  return Result("", jt);
}

JsonThing::Result  //
JsonThing::parse_literal(TokenStream::Result tsr) {
  uint64_t vbd = tsr.token.value_base_detail();
  if (vbd & WUFFS_BASE__TOKEN__VBD__LITERAL__NULL) {
    JsonThing jt;
    jt.kind = Kind::Null;
    return Result("", jt);
  } else if (vbd & WUFFS_BASE__TOKEN__VBD__LITERAL__FALSE) {
    JsonThing jt;
    jt.kind = Kind::Bool;
    jt.value.b = false;
    return Result("", jt);
  } else if (vbd & WUFFS_BASE__TOKEN__VBD__LITERAL__TRUE) {
    JsonThing jt;
    jt.kind = Kind::Bool;
    jt.value.b = true;
    return Result("", jt);
  }
  return Result("main: internal error: unexpected token");
}

JsonThing::Result  //
JsonThing::parse_number(TokenStream::Result tsr) {
  // Parsing the number from its string representation (converting from "123"
  // to 123) isn't necessary for the jsonfindptrs program, but if you're
  // copy/pasting this JsonThing code, here's how to do it.
  uint64_t vbd = tsr.token.value_base_detail();
  if (vbd & WUFFS_BASE__TOKEN__VBD__NUMBER__FORMAT_TEXT) {
    if (vbd & WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_INTEGER_SIGNED) {
      static constexpr int64_t m = 0x001FFFFFFFFFFFFF;  // ((1<<53) - 1).
      wuffs_base__result_i64 r = wuffs_base__parse_number_i64(tsr.src_data);
      if (!r.status.is_ok()) {
        return Result(r.status.message());
      } else if ((r.value < -m) || (+m < r.value)) {
        return Result(wuffs_base__error__out_of_bounds);
      }
      JsonThing jt;
      jt.kind = Kind::Int64;
      jt.value.i = r.value;
      return Result("", jt);
    } else if (vbd & WUFFS_BASE__TOKEN__VBD__NUMBER__CONTENT_FLOATING_POINT) {
      wuffs_base__result_f64 r = wuffs_base__parse_number_f64(tsr.src_data);
      if (!r.status.is_ok()) {
        return Result(r.status.message());
      }
      JsonThing jt;
      jt.kind = Kind::Float64;
      jt.value.f = r.value;
      return Result("", jt);
    }
  }
  return Result("main: internal error: unexpected number");
}

JsonThing::Result  //
JsonThing::parse_object(TokenStream& ts) {
  JsonThing jt;
  jt.kind = Kind::Object;

  std::string key;
  bool have_key = false;

  while (true) {
    TokenStream::Result tsr = ts.peek();
    if (!tsr.status_msg.empty()) {
      return Result(std::move(tsr.status_msg));
    }
    int64_t vbc = tsr.token.value_base_category();
    uint64_t vbd = tsr.token.value_base_detail();
    if (vbc == WUFFS_BASE__TOKEN__VBC__FILLER) {
      ts.next();
      continue;
    } else if ((vbc == WUFFS_BASE__TOKEN__VBC__STRUCTURE) &&
               (vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__POP)) {
      ts.next();
      break;
    }

    JsonThing::Result jtr = JsonThing::parse(ts);
    if (!jtr.status_msg.empty()) {
      return Result(std::move(jtr.status_msg));
    }

    if (have_key) {
      have_key = false;
      auto iter = jt.value.o.find(key);
      if (iter == jt.value.o.end()) {
        jt.value.o.insert(
            iter, Map::value_type(std::move(key), std::move(jtr.thing)));
      } else {
        return Result("main: duplicate key: " + key);
      }
    } else if (jtr.thing.kind == Kind::String) {
      have_key = true;
      key = std::move(jtr.thing.value.s);
    } else {
      return Result("main: internal error: unexpected non-string key");
    }
  }
  if (have_key) {
    return Result("main: internal error: unpaired key");
  }
  return Result("", jt);
}

JsonThing::Result  //
JsonThing::parse_string(TokenStream& ts, TokenStream::Result tsr) {
  JsonThing jt;
  jt.kind = Kind::String;
  while (true) {
    int64_t vbc = tsr.token.value_base_category();
    uint64_t vbd = tsr.token.value_base_detail();

    switch (vbc) {
      case WUFFS_BASE__TOKEN__VBC__STRING: {
        if (vbd & WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_0_DST_1_SRC_DROP) {
          // No-op.

        } else if (vbd &
                   WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_1_DST_1_SRC_COPY) {
          const char* ptr =  // Convert from (uint8_t*).
              static_cast<const char*>(static_cast<void*>(tsr.src_data.ptr));
          jt.value.s.append(ptr, tsr.src_data.len);

        } else if (
            vbd &
            WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_1_DST_4_SRC_BACKSLASH_X) {
          // We shouldn't get here unless we enable the
          // WUFFS_JSON__QUIRK_ALLOW_BACKSLASH_X option. The jsonfindptrs
          // program doesn't enable that by default, but if you're copy/pasting
          // this JsonThing code and your program does enable that option,
          // here's how to handle it.
          //
          // As per the quirk documentation, there are two options for how to
          // interpret a backslash-x: as a byte or as a Unicode code point.
          // This implementation chooses as a byte.
          wuffs_base__slice_u8 encoded = tsr.src_data;
          if (encoded.len & 3) {
            return Result(
                "main: internal error: \\x token length not a multiple of 4",
                JsonThing());
          }
          while (encoded.len) {
            uint8_t decoded[64];
            size_t len = wuffs_base__hexadecimal__decode4(
                wuffs_base__make_slice_u8(&decoded[0], 64), encoded);
            if ((len > 64) || ((len * 4) > encoded.len)) {
              return Result(
                  "main: internal error: inconsistent hexadecimal decoding",
                  JsonThing());
            }
            const char* ptr =  // Convert from (uint8_t*).
                static_cast<const char*>(static_cast<void*>(&decoded[0]));
            jt.value.s.append(ptr, len);
            encoded.ptr += len * 4;
            encoded.len -= len * 4;
          }

        } else {
          return Result(
              "main: internal error: unexpected string-token conversion",
              JsonThing());
        }
        break;
      }

      case WUFFS_BASE__TOKEN__VBC__UNICODE_CODE_POINT: {
        uint8_t u[WUFFS_BASE__UTF_8__BYTE_LENGTH__MAX_INCL];
        size_t n = wuffs_base__utf_8__encode(
            wuffs_base__make_slice_u8(&u[0],
                                      WUFFS_BASE__UTF_8__BYTE_LENGTH__MAX_INCL),
            vbd);
        const char* ptr =  // Convert from (uint8_t*).
            static_cast<const char*>(static_cast<void*>(&u[0]));
        jt.value.s.append(ptr, n);
        break;
      }

      default:
        return Result("main: internal error: unexpected token");
    }

    if (!tsr.token.continued()) {
      break;
    }
    tsr = ts.next();
    if (!tsr.status_msg.empty()) {
      return Result(std::move(tsr.status_msg));
    }
  }
  return Result("", jt);
}

// ----

std::string  //
escape(std::string s) {
  for (char& c : s) {
    if ((c == '~') || (c == '/') || (c == '\n') || (c == '\r')) {
      goto escape_needed;
    }
  }
  return s;

escape_needed:
  std::string e;
  e.reserve(8 + s.length());
  for (char& c : s) {
    switch (c) {
      case '~':
        e += "~0";
        break;
      case '/':
        e += "~1";
        break;
      case '\n':
        if (g_flags.strict_json_pointer_syntax) {
          return "";
        }
        e += "~n";
        break;
      case '\r':
        if (g_flags.strict_json_pointer_syntax) {
          return "";
        }
        e += "~r";
        break;
      default:
        e += c;
        break;
    }
  }
  return e;
}

std::string  //
print_json_pointers(JsonThing& jt, std::string s, uint32_t depth) {
  std::cout << s << std::endl;
  if (depth++ >= g_flags.max_output_depth) {
    return "";
  }

  switch (jt.kind) {
    case JsonThing::Kind::Array:
      s += "/";
      for (size_t i = 0; i < jt.value.a.size(); i++) {
        TRY(print_json_pointers(jt.value.a[i], s + std::to_string(i), depth));
      }
      break;
    case JsonThing::Kind::Object:
      s += "/";
      for (auto& kv : jt.value.o) {
        std::string e = escape(kv.first);
        if (e.empty() && !kv.first.empty()) {
          return "main: unsupported \"\\u000A\" or \"\\u000D\" in object key";
        }
        TRY(print_json_pointers(kv.second, s + e, depth));
      }
      break;
    default:
      break;
  }
  return "";
}

std::string  //
main1(int argc, char** argv) {
  TRY(parse_flags(argc, argv));

  int input_file_descriptor = 0;  // A 0 default means stdin.
  if (g_flags.remaining_argc > 1) {
    return g_usage;
  } else if (g_flags.remaining_argc == 1) {
    const char* arg = g_flags.remaining_argv[0];
    input_file_descriptor = open(arg, O_RDONLY);
    if (input_file_descriptor < 0) {
      return std::string("main: cannot read ") + arg + ": " + strerror(errno);
    }
  }

  TokenStream ts(input_file_descriptor);
  JsonThing::Result jtr = JsonThing::parse(ts);
  if (!jtr.status_msg.empty()) {
    return jtr.status_msg;
  }
  return print_json_pointers(jtr.thing, "", 0);
}

// ----

int  //
compute_exit_code(std::string status_msg) {
  if (status_msg.empty()) {
    return 0;
  }
  std::cerr << status_msg << std::endl;
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
  return (status_msg.find("internal error:") != std::string::npos) ? 2 : 1;
}

int  //
main(int argc, char** argv) {
  std::string z = main1(argc, argv);
  int exit_code = compute_exit_code(z);
  return exit_code;
}
