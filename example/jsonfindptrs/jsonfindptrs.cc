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
(in sorted order). The wuffs_aux::DecodeJson library function converts the
lower level token stream to higher level callbacks. This .cc file deals only
with those callbacks, not with tokens per se.

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

To run:

$CXX jsonfindptrs.cc && ./a.out < ../../test/data/github-tags.json; rm -f a.out

for a C++ compiler $CXX, such as clang++ or g++.
*/

#if defined(__cplusplus) && (__cplusplus < 201103L)
#error "This C++ program requires -std=c++11 or later"
#endif

#include <stdio.h>

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
#define WUFFS_CONFIG__MODULE__AUX__BASE
#define WUFFS_CONFIG__MODULE__AUX__JSON
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
    "    -q=STR  -query=STR\n"
    "            -input-allow-comments\n"
    "            -input-allow-extra-comma\n"
    "            -input-allow-inf-nan-numbers\n"
    "            -strict-json-pointer-syntax\n"
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
    "The -input-allow-comments flag allows \"/*slash-star*/\" and\n"
    "\"//slash-slash\" C-style comments within JSON input.\n"
    "\n"
    "The -input-allow-extra-comma flag allows input like \"[1,2,]\", with a\n"
    "comma after the final element of a JSON list or dictionary.\n"
    "\n"
    "The -input-allow-inf-nan-numbers flag allows non-finite floating point\n"
    "numbers (infinities and not-a-numbers) within JSON input.\n"
    "\n"
    "----\n"
    "\n"
    "The -strict-json-pointer-syntax flag restricts the output lines to\n"
    "exactly RFC 6901, with only two escape sequences: \"~0\" and \"~1\" for\n"
    "\"~\" and \"/\". Without this flag, this program also lets \"~n\" and\n"
    "\"~r\" escape the New Line and Carriage Return ASCII control characters,\n"
    "which can work better with line oriented Unix tools that assume exactly\n"
    "one value (i.e. one JSON Pointer string) per line. With this flag, it\n"
    "fails if the input JSON's keys contain \"\\u000A\" or \"\\u000D\".\n"
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

std::vector<uint32_t> g_quirks;

struct {
  int remaining_argc;
  char** remaining_argv;

  uint32_t max_output_depth;
  char* query_c_string;
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
          wuffs_base__make_slice_u8((uint8_t*)arg, strlen(arg)),
          WUFFS_BASE__PARSE_NUMBER_XXX__DEFAULT_OPTIONS);
      if (wuffs_base__status__is_ok(&u.status) && (u.value <= 0xFFFFFFFF)) {
        g_flags.max_output_depth = (uint32_t)(u.value);
        continue;
      }
      return g_usage;
    }
    if (!strcmp(arg, "input-allow-comments")) {
      g_quirks.push_back(WUFFS_JSON__QUIRK_ALLOW_COMMENT_BLOCK);
      g_quirks.push_back(WUFFS_JSON__QUIRK_ALLOW_COMMENT_LINE);
      continue;
    }
    if (!strcmp(arg, "input-allow-extra-comma")) {
      g_quirks.push_back(WUFFS_JSON__QUIRK_ALLOW_EXTRA_COMMA);
      continue;
    }
    if (!strcmp(arg, "input-allow-inf-nan-numbers")) {
      g_quirks.push_back(WUFFS_JSON__QUIRK_ALLOW_INF_NAN_NUMBERS);
      continue;
    }
    if (!strncmp(arg, "q=", 2) || !strncmp(arg, "query=", 6)) {
      while (*arg++ != '=') {
      }
      g_flags.query_c_string = arg;
      continue;
    }
    if (!strcmp(arg, "strict-json-pointer-syntax")) {
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

class JsonThing {
 public:
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
};

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

// ----

class Callbacks : public wuffs_aux::DecodeJsonCallbacks {
 public:
  struct Entry {
    Entry(JsonThing&& jt)
        : thing(std::move(jt)), has_map_key(false), map_key() {}

    JsonThing thing;
    bool has_map_key;
    std::string map_key;
  };

  Callbacks() = default;

  std::string Append(JsonThing&& jt) {
    if (m_stack.empty()) {
      m_stack.push_back(Entry(std::move(jt)));
      return "";
    }
    Entry& top = m_stack.back();
    switch (top.thing.kind) {
      case JsonThing::Kind::Array:
        top.thing.value.a.push_back(std::move(jt));
        return "";
      case JsonThing::Kind::Object:
        if (top.has_map_key) {
          top.has_map_key = false;
          auto iter = top.thing.value.o.find(top.map_key);
          if (iter != top.thing.value.o.end()) {
            return "main: duplicate key: " + top.map_key;
          }
          top.thing.value.o.insert(
              iter, JsonThing::Map::value_type(std::move(top.map_key),
                                               std::move(jt)));
          return "";
        } else if (jt.kind == JsonThing::Kind::String) {
          top.has_map_key = true;
          top.map_key = std::move(jt.value.s);
          return "";
        }
        return "main: internal error: non-string map key";
    }
    return "main: internal error: non-container stack entry";
  }

  std::string AppendNull() override {
    JsonThing jt;
    jt.kind = JsonThing::Kind::Null;
    return Append(std::move(jt));
  }

  std::string AppendBool(bool val) override {
    JsonThing jt;
    jt.kind = JsonThing::Kind::Bool;
    jt.value.b = val;
    return Append(std::move(jt));
  }

  std::string AppendI64(int64_t val) override {
    JsonThing jt;
    jt.kind = JsonThing::Kind::Int64;
    jt.value.i = val;
    return Append(std::move(jt));
  }

  std::string AppendF64(double val) override {
    JsonThing jt;
    jt.kind = JsonThing::Kind::Float64;
    jt.value.f = val;
    return Append(std::move(jt));
  }

  std::string AppendTextString(std::string&& val) override {
    JsonThing jt;
    jt.kind = JsonThing::Kind::String;
    jt.value.s = std::move(val);
    return Append(std::move(jt));
  }

  std::string Push(uint32_t flags) override {
    if (flags & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST) {
      JsonThing jt;
      jt.kind = JsonThing::Kind::Array;
      m_stack.push_back(std::move(jt));
      return "";
    } else if (flags & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_DICT) {
      JsonThing jt;
      jt.kind = JsonThing::Kind::Object;
      m_stack.push_back(std::move(jt));
      return "";
    }
    return "main: internal error: bad push";
  }

  std::string Pop(uint32_t flags) override {
    if (m_stack.empty()) {
      return "main: internal error: bad pop";
    }
    JsonThing jt = std::move(m_stack.back().thing);
    m_stack.pop_back();
    return Append(std::move(jt));
  }

  void Done(wuffs_aux::DecodeJsonResult& result,
            wuffs_aux::sync_io::Input& input,
            wuffs_aux::IOBuffer& buffer) override {
    if (!result.error_message.empty()) {
      return;
    } else if (m_stack.size() != 1) {
      result.error_message = "main: internal error: bad depth";
      return;
    }
    result.error_message = print_json_pointers(m_stack.back().thing, "", 0);
  }

 private:
  std::vector<Entry> m_stack;
};

// ----

std::string  //
main1(int argc, char** argv) {
  TRY(parse_flags(argc, argv));
  if (!g_flags.strict_json_pointer_syntax) {
    g_quirks.push_back(WUFFS_JSON__QUIRK_JSON_POINTER_ALLOW_TILDE_R_TILDE_N);
  }

  FILE* in = stdin;
  if (g_flags.remaining_argc > 1) {
    return g_usage;
  } else if (g_flags.remaining_argc == 1) {
    in = fopen(g_flags.remaining_argv[0], "r");
    if (!in) {
      return std::string("main: cannot read input file");
    }
  }

  Callbacks callbacks;
  wuffs_aux::sync_io::FileInput input(in);
  return wuffs_aux::DecodeJson(
             callbacks, input,
             wuffs_base__make_slice_u32(g_quirks.data(), g_quirks.size()),
             (g_flags.query_c_string ? g_flags.query_c_string : ""))
      .error_message;
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
