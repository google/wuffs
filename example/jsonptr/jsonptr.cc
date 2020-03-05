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
jsonptr is a JSON formatter (pretty-printer) that supports the JSON Pointer
(RFC 6901) query syntax. It reads UTF-8 JSON from stdin and writes
canonicalized, formatted UTF-8 JSON to stdout.

See the "const char* usage" string below for details.

----

JSON Pointer (and this program's implementation) is one of many JSON query
languages and JSON tools, such as jq, jql and JMESPath. This one is relatively
simple and fewer-featured compared to those others.

One benefit of simplicity is that this program's JSON and JSON Pointer
implementations do not dynamically allocate or free memory (yet it does not
require that the entire input fits in memory at once). They are therefore
trivially protected against certain bug classes: memory leaks, double-frees and
use-after-frees.

The core JSON implementation is also written in the Wuffs programming language
(and then transpiled to C/C++), which is memory-safe but also guards against
integer arithmetic overflows.

For defense in depth, on Linux, this program also self-imposes a
SECCOMP_MODE_STRICT sandbox before reading (or otherwise processing) its input
or writing its output. Under this sandbox, the only permitted system calls are
read, write, exit and sigreturn.

All together, this program aims to safely handle untrusted JSON files without
fear of security bugs such as remote code execution.

----

As of 2020-02-24, this program passes all 318 "test_parsing" cases from the
JSON test suite (https://github.com/nst/JSONTestSuite), an appendix to the
"Parsing JSON is a Minefield" article (http://seriot.ch/parsing_json.php) that
was first published on 2016-10-26 and updated on 2018-03-30.

After modifying this program, run "build-example.sh example/jsonptr/" and then
"script/run-json-test-suite.sh" to catch correctness regressions.

----

This example program differs from most other example Wuffs programs in that it
is written in C++, not C.

$CXX jsonptr.cc && ./a.out < ../../test/data/github-tags.json; rm -f a.out

for a C++ compiler $CXX, such as clang++ or g++.
*/

#include <errno.h>
#include <string.h>
#include <unistd.h>

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

#if defined(__linux__)
#include <linux/prctl.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#define WUFFS_EXAMPLE_USE_SECCOMP
#endif

#define TRY(error_msg)         \
  do {                         \
    const char* z = error_msg; \
    if (z) {                   \
      return z;                \
    }                          \
  } while (false)

static const char* eod = "main: end of data";

static const char* usage =
    "Usage: jsonptr -flags < input.json\n"
    "\n"
    "Note the \"<\". It only reads from stdin, not named files.\n"
    "\n"
    "jsonptr is a JSON formatter (pretty-printer) that supports the JSON\n"
    "Pointer (RFC 6901) query syntax. It reads UTF-8 JSON from stdin and\n"
    "writes canonicalized, formatted UTF-8 JSON to stdout.\n"
    "\n"
    "Canonicalized means that e.g. \"abc\\u000A\\tx\\u0177z\" is re-written\n"
    "as \"abc\\n\\txŷz\". It does not sort object keys, nor does it reject\n"
    "duplicate keys.\n"
    "\n"
    "Formatted means that arrays' and objects' elements are indented, each\n"
    "on its own line. Configure this with the -c / -compact, -i=N / -indent=N\n"
    "(for N ranging from 0 to 8) and -t / -tabs flags.\n"
    "\n"
    "The -q=etc or -query=etc flag gives an optional JSON Pointer query, to\n"
    "print a subset of the input. For example, given RFC 6901 section 5's\n"
    "[sample input](https://tools.ietf.org/rfc/rfc6901.txt), this command:\n"
    "    jsonptr -query=/foo/1 < rfc-6901-json-pointer.json\n"
    "will print:\n"
    "    \"baz\"\n"
    "\n"
    "An absent query is equivalent to the empty query, which identifies the\n"
    "entire input (the root value). The \"/\" query is not equivalent to the\n"
    "root value. Instead, it identifies the child (the key-value pair) of the\n"
    "root value whose key is the empty string.\n"
    "\n"
    "If the query found a valid JSON value, this program will return a zero\n"
    "exit code even if the rest of the input isn't valid JSON. If the query\n"
    "did not find a value, or found an invalid one, this program returns a\n"
    "non-zero exit code, but may still print partial output to stdout.\n"
    "\n"
    "The [JSON specification](https://json.org/) permits implementations that\n"
    "allow duplicate keys, as this one does. This JSON Pointer implementation\n"
    "is also greedy, following the first match for each fragment without\n"
    "back-tracking. For example, the \"/foo/bar\" query will fail if the root\n"
    "object has multiple \"foo\" children but the first one doesn't have a\n"
    "\"bar\" child, even if later ones do.\n"
    "\n"
    "The -fail-if-unsandboxed flag causes the program to exit if it does not\n"
    "self-impose a sandbox. On Linux, it self-imposes a SECCOMP_MODE_STRICT\n"
    "sandbox, regardless of this flag.";

// ----

bool sandboxed = false;

#define MAX_INDENT 8
#define INDENT_SPACES_STRING "        "
#define INDENT_TAB_STRING "\t"

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

uint32_t depth;

enum class context {
  none,
  in_list_after_bracket,
  in_list_after_value,
  in_dict_after_brace,
  in_dict_after_key,
  in_dict_after_value,
} ctx;

bool  //
in_dict_before_key() {
  return (ctx == context::in_dict_after_brace) ||
         (ctx == context::in_dict_after_value);
}

bool suppress_write_dst;
bool wrote_to_dst;

wuffs_json__decoder dec;

// ----

// Query is a JSON Pointer query. After initializing with a NUL-terminated C
// string, its multiple fragments are consumed as the program walks the JSON
// data from stdin. For example, letting "$" denote a NUL, suppose that we
// started with a query string of "/apple/banana/12/durian" and are currently
// trying to match the second fragment, "banana", so that Query::depth is 2:
//
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  / a p p l e / b a n a n a / 1 2 / d u r i a n $
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//                ^           ^
//                frag_i      frag_k
//
// The two pointers frag_i and frag_k are the start (inclusive) and end
// (exclusive) of the fragment. They satisfy (frag_i <= frag_k) and may be
// equal if the fragment empty (note that "" is a valid JSON object key).
//
// The frag_j pointer moves between these two, or is nullptr. An invariant is
// that (((frag_i <= frag_j) && (frag_j <= frag_k)) || (frag_j == nullptr)).
//
// Wuffs' JSON tokenizer can portray a single JSON string as multiple Wuffs
// tokens, as backslash-escaped values within that JSON string may each get
// their own token.
//
// At the start of each object key (a JSON string), frag_j is set to frag_i.
//
// While frag_j remains non-nullptr, each token's unescaped contents are then
// compared to that part of the fragment from frag_j to frag_k. If it is a
// prefix (including the case of an exact match), then frag_j is advanced by
// the unescaped length. Otherwise, frag_j is set to nullptr.
//
// Comparison accounts for JSON Pointer's escaping notation: "~0" and "~1" in
// the query (not the JSON value) are unescaped to "~" and "/" respectively.
//
// The frag_j pointer therefore advances from frag_i to frag_k, or drops out,
// as we incrementally match the object key with the query fragment. For
// example, if we have already matched the "ban" of "banana", then we would
// accept any of an "ana" token, an "a" token or a "\u0061" token, amongst
// others. They would advance frag_j by 3, 1 or 1 bytes respectively.
//
//                      frag_j
//                      v
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  / a p p l e / b a n a n a / 1 2 / d u r i a n $
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//                ^           ^
//                frag_i      frag_k
//
// At the end of each object key (or equivalently, at the start of each object
// value), if frag_j is non-nullptr and equal to (but not less than) frag_k
// then we have a fragment match: the query fragment equals the object key. If
// there is a next fragment (in this example, "12") we move the frag_etc
// pointers to its start and end and increment Query::depth. Otherwise, we have
// matched the complete query, and the upcoming JSON value is the result of
// that query.
//
// The discussion above centers on object keys. If the query fragment is
// numeric then it can also match as an array index: the string fragment "12"
// will match an array's 13th element (starting counting from zero). See RFC
// 6901 for its precise definition of an "array index" number.
//
// Array index fragment match is represented by the Query::array_index field,
// whose type (wuffs_base__result_u64) is a result type. An error result means
// that the fragment is not an array index. A value result holds the number of
// list elements remaining. When matching a query fragment in an array (instead
// of in an object), each element ticks this number down towards zero. At zero,
// the upcoming JSON value is the one that matches the query fragment.
class Query {
 private:
  uint8_t* frag_i;
  uint8_t* frag_j;
  uint8_t* frag_k;

  uint32_t depth;

  wuffs_base__result_u64 array_index;

 public:
  void reset(char* query_c_string) {
    this->frag_i = (uint8_t*)query_c_string;
    this->frag_j = (uint8_t*)query_c_string;
    this->frag_k = (uint8_t*)query_c_string;
    this->depth = 0;
    this->array_index.status.repr = "#main: not an array index query fragment";
    this->array_index.value = 0;
  }

  void restart_fragment(bool enable) {
    this->frag_j = enable ? this->frag_i : nullptr;
  }

  bool is_at(uint32_t depth) { return this->depth == depth; }

  // tick returns whether the fragment is a valid array index whose value is
  // zero. If valid but non-zero, it decrements it and returns false.
  bool tick() {
    if (this->array_index.status.is_ok()) {
      if (this->array_index.value == 0) {
        return true;
      }
      this->array_index.value--;
    }
    return false;
  }

  // next_fragment moves to the next fragment, returning whether it existed.
  bool next_fragment() {
    uint8_t* k = this->frag_k;
    uint32_t d = this->depth;

    this->reset(nullptr);

    if (!k || (*k != '/')) {
      return false;
    }
    k++;

    bool all_digits = true;
    uint8_t* i = k;
    while ((*k != '\x00') && (*k != '/')) {
      all_digits = all_digits && ('0' <= *k) && (*k <= '9');
      k++;
    }
    this->frag_i = i;
    this->frag_j = i;
    this->frag_k = k;
    this->depth = d + 1;
    if (all_digits) {
      // wuffs_base__parse_number_u64 rejects leading zeroes, e.g. "00", "07".
      this->array_index =
          wuffs_base__parse_number_u64(wuffs_base__make_slice_u8(i, k - i));
    }
    return true;
  }

  bool matched() { return this->frag_j && (this->frag_j == this->frag_k); }

  void incremental_match_slice(uint8_t* ptr, size_t len) {
    if (!this->frag_j) {
      return;
    }
    uint8_t* j = this->frag_j;
    while (true) {
      if (len == 0) {
        this->frag_j = j;
        return;
      }

      if (*j == '\x00') {
        break;

      } else if (*j == '~') {
        j++;
        if (*j == '0') {
          if (*ptr != '~') {
            break;
          }
        } else if (*j == '1') {
          if (*ptr != '/') {
            break;
          }
        } else {
          break;
        }

      } else if (*j != *ptr) {
        break;
      }

      j++;
      ptr++;
      len--;
    }
    this->frag_j = nullptr;
  }

  void incremental_match_code_point(uint32_t code_point) {
    if (!this->frag_j) {
      return;
    }
    uint8_t u[WUFFS_BASE__UTF_8__BYTE_LENGTH__MAX_INCL];
    size_t n = wuffs_base__utf_8__encode(
        wuffs_base__make_slice_u8(&u[0],
                                  WUFFS_BASE__UTF_8__BYTE_LENGTH__MAX_INCL),
        code_point);
    if (n > 0) {
      this->incremental_match_slice(&u[0], n);
    }
  }

  // validate returns whether the (ptr, len) arguments form a valid JSON
  // Pointer. In particular, it must be valid UTF-8, and either be empty or
  // start with a '/'. Any '~' within must immediately be followed by either
  // '0' or '1'.
  static bool validate(char* query_c_string, size_t length) {
    if (length <= 0) {
      return true;
    }
    if (query_c_string[0] != '/') {
      return false;
    }
    wuffs_base__slice_u8 s =
        wuffs_base__make_slice_u8((uint8_t*)query_c_string, length);
    bool previous_was_tilde = false;
    while (s.len > 0) {
      wuffs_base__utf_8__next__output o = wuffs_base__utf_8__next(s);
      if (!o.is_valid()) {
        return false;
      }
      if (previous_was_tilde && (o.code_point != '0') &&
          (o.code_point != '1')) {
        return false;
      }
      previous_was_tilde = o.code_point == '~';
      s.ptr += o.byte_length;
      s.len -= o.byte_length;
    }
    return !previous_was_tilde;
  }
} query;

// ----

struct {
  int remaining_argc;
  char** remaining_argv;

  bool compact;
  bool fail_if_unsandboxed;
  size_t indent;
  char* query_c_string;
  bool tabs;
} flags = {0};

const char*  //
parse_flags(int argc, char** argv) {
  flags.indent = 4;

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
    if (!strcmp(arg, "fail-if-unsandboxed")) {
      flags.fail_if_unsandboxed = true;
      continue;
    }
    if (!strncmp(arg, "i=", 2) || !strncmp(arg, "indent=", 7)) {
      while (*arg++ != '=') {
      }
      if (('0' <= arg[0]) && (arg[0] <= '8') && (arg[1] == '\x00')) {
        flags.indent = arg[0] - '0';
        continue;
      }
      return usage;
    }
    if (!strncmp(arg, "q=", 2) || !strncmp(arg, "query=", 6)) {
      while (*arg++ != '=') {
      }
      if (Query::validate(arg, strlen(arg))) {
        flags.query_c_string = arg;
        continue;
      }
      return usage;
    }
    if (!strcmp(arg, "t") || !strcmp(arg, "tabs")) {
      flags.tabs = true;
      continue;
    }

    return usage;
  }

  flags.remaining_argc = argc - c;
  flags.remaining_argv = argv + c;
  return nullptr;
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
  if (flags.fail_if_unsandboxed && !sandboxed) {
    return "main: unsandboxed";
  }
  if (flags.remaining_argc > 0) {
    return usage;
  }

  query.reset(flags.query_c_string);

  // If the query is non-empty, suprress writing to stdout until we've
  // completed the query.
  suppress_write_dst = query.next_fragment();
  wrote_to_dst = false;

  return dec.initialize(sizeof__wuffs_json__decoder(), WUFFS_VERSION, 0)
      .message();
}

// ----

// ignore_return_value suppresses errors from -Wall -Werror.
static void  //
ignore_return_value(int ignored) {}

const char*  //
read_src() {
  if (src.meta.closed) {
    return "main: internal error: read requested on a closed source";
  }
  src.compact();
  if (src.meta.wi >= src.data.len) {
    return "main: src buffer is full";
  }
  while (true) {
    const int stdin_fd = 0;
    ssize_t n =
        read(stdin_fd, src.data.ptr + src.meta.wi, src.data.len - src.meta.wi);
    if (n >= 0) {
      src.meta.wi += n;
      src.meta.closed = n == 0;
      break;
    } else if (errno != EINTR) {
      return strerror(errno);
    }
  }
  return nullptr;
}

const char*  //
flush_dst() {
  while (true) {
    size_t n = dst.meta.wi - dst.meta.ri;
    if (n == 0) {
      break;
    }
    const int stdout_fd = 1;
    ssize_t i = write(stdout_fd, dst.data.ptr + dst.meta.ri, n);
    if (i >= 0) {
      dst.meta.ri += i;
    } else if (errno != EINTR) {
      return strerror(errno);
    }
  }
  dst.compact();
  return nullptr;
}

const char*  //
write_dst(const void* s, size_t n) {
  if (suppress_write_dst) {
    return nullptr;
  }
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
    wrote_to_dst = true;
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

  } else if (ucp == '\"') {
    return write_dst("\\\"", 2);

  } else if (ucp == '\\') {
    return write_dst("\\\\", 2);

  } else {
    uint8_t u[WUFFS_BASE__UTF_8__BYTE_LENGTH__MAX_INCL];
    size_t n = wuffs_base__utf_8__encode(
        wuffs_base__make_slice_u8(&u[0],
                                  WUFFS_BASE__UTF_8__BYTE_LENGTH__MAX_INCL),
        ucp);
    if (n > 0) {
      return write_dst(&u[0], n);
    }
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
      if (query.is_at(depth)) {
        return "main: no match for query";
      }
      if (depth <= 0) {
        return "main: internal error: inconsistent depth";
      }
      depth--;

      // Write preceding whitespace.
      if ((ctx != context::in_list_after_bracket) &&
          (ctx != context::in_dict_after_brace) && !flags.compact) {
        TRY(write_dst("\n", 1));
        for (uint32_t i = 0; i < depth; i++) {
          TRY(write_dst(flags.tabs ? INDENT_TAB_STRING : INDENT_SPACES_STRING,
                        flags.tabs ? 1 : flags.indent));
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
    if (!t.link_prev()) {
      if (ctx == context::in_dict_after_key) {
        TRY(write_dst(": ", flags.compact ? 1 : 2));
      } else if (ctx != context::none) {
        if ((ctx != context::in_list_after_bracket) &&
            (ctx != context::in_dict_after_brace)) {
          TRY(write_dst(",", 1));
        }
        if (!flags.compact) {
          TRY(write_dst("\n", 1));
          for (size_t i = 0; i < depth; i++) {
            TRY(write_dst(flags.tabs ? INDENT_TAB_STRING : INDENT_SPACES_STRING,
                          flags.tabs ? 1 : flags.indent));
          }
        }
      }

      bool query_matched = false;
      if (query.is_at(depth)) {
        switch (ctx) {
          case context::in_list_after_bracket:
          case context::in_list_after_value:
            query_matched = query.tick();
            break;
          case context::in_dict_after_key:
            query_matched = query.matched();
            break;
        }
      }
      if (!query_matched) {
        // No-op.
      } else if (!query.next_fragment()) {
        // There is no next fragment. We have matched the complete query, and
        // the upcoming JSON value is the result of that query.
        //
        // Un-suppress writing to stdout and reset the ctx and depth as if we
        // were about to decode a top-level value. This makes any subsequent
        // indentation be relative to this point, and we will return eod after
        // the upcoming JSON value is complete.
        suppress_write_dst = false;
        ctx = context::none;
        depth = 0;
      } else if ((vbc != WUFFS_BASE__TOKEN__VBC__STRUCTURE) ||
                 !(vbd & WUFFS_BASE__TOKEN__VBD__STRUCTURE__PUSH)) {
        // The query has moved on to the next fragment but the upcoming JSON
        // value is not a container.
        return "main: no match for query";
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
          query.restart_fragment(in_dict_before_key() && query.is_at(depth));
        }

        if (vbd & WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_0_DST_1_SRC_DROP) {
          // No-op.
        } else if (vbd &
                   WUFFS_BASE__TOKEN__VBD__STRING__CONVERT_1_DST_1_SRC_COPY) {
          uint8_t* ptr = src.data.ptr + curr_token_end_src_index - len;
          TRY(write_dst(ptr, len));
          query.incremental_match_slice(ptr, len);
        } else {
          return "main: internal error: unexpected string-token conversion";
        }

        if (t.link_next()) {
          return nullptr;
        }
        TRY(write_dst("\"", 1));
        goto after_value;

      case WUFFS_BASE__TOKEN__VBC__UNICODE_CODE_POINT:
        if (!t.link_prev() || !t.link_next()) {
          return "main: internal error: unexpected unlinked token";
        }
        TRY(handle_unicode_code_point(vbd));
        query.incremental_match_code_point(vbd);
        return nullptr;

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
        goto end_of_data;
      }
      return z;
    }

    if (status.repr == nullptr) {
      return "main: internal error: unexpected end of token stream";
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
end_of_data:

  // With a non-empty query, don't try to consume trailing whitespace or
  // confirm that we've processed all the tokens.
  if (flags.query_c_string && *flags.query_c_string) {
    return nullptr;
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
  if ((src.meta.ri == src.meta.wi) && !src.meta.closed) {
    TRY(read_src());
  }
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
  const int stderr_fd = 2;
  ignore_return_value(write(stderr_fd, status_msg, n));
  ignore_return_value(write(stderr_fd, "\n", 1));
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
#if defined(WUFFS_EXAMPLE_USE_SECCOMP)
  prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT);
  sandboxed = true;
#endif

  const char* z = main1(argc, argv);
  if (wrote_to_dst) {
    const char* z1 = write_dst("\n", 1);
    const char* z2 = flush_dst();
    z = z ? z : (z1 ? z1 : z2);
  }
  int exit_code = compute_exit_code(z);

#if defined(WUFFS_EXAMPLE_USE_SECCOMP)
  // Call SYS_exit explicitly, instead of calling SYS_exit_group implicitly by
  // either calling _exit or returning from main. SECCOMP_MODE_STRICT allows
  // only SYS_exit.
  syscall(SYS_exit, exit_code);
#endif
  return exit_code;
}
