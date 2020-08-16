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
cbor-to-json reads CBOR (a binary format) from stdin and writes the equivalent
formatted JSON (a text format) to stdout.

See the "const char* g_usage" string below for details.

----

To run:

$CXX cbor-to-json.cc && ./a.out < ../../test/data/json-things.cbor; rm -f a.out

for a C++ compiler $CXX, such as clang++ or g++.
*/

#if defined(__cplusplus) && (__cplusplus < 201103L)
#error "This C++ program requires -std=c++11 or later"
#endif

#include <stdio.h>

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
#define WUFFS_CONFIG__MODULE__AUX__CBOR
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__CBOR

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
    "Usage: cbor-to-json -flags input.cbor\n"
    "\n"
    "Flags:\n"
    "    -c      -compact-output\n"
    "    -s=NUM  -spaces=NUM\n"
    "    -t      -tabs\n"
    "            -output-cbor-metadata-as-comments\n"
    "            -output-extra-comma\n"
    "            -output-inf-nan-numbers\n"
    "\n"
    "The input.cbor filename is optional. If absent, it reads from stdin.\n"
    "\n"
    "----\n"
    "\n"
    "cbor-to-json reads CBOR (a binary format) from stdin and writes the\n"
    "equivalent formatted JSON (a text format) to stdout.\n"
    "\n"
    "The output JSON's arrays' and objects' elements are indented, each on\n"
    "its own line. Configure this with the -c / -compact-output, -s=NUM /\n"
    "-spaces=NUM (for NUM ranging from 0 to 8) and -t / -tabs flags.\n"
    "\n"
    "The conversion may be lossy. For example, CBOR metadata such as tags or\n"
    "distinguishing undefined from null are either dropped or, with\n"
    "-output-cbor-metadata-as-comments, converted to \"/*comments*/\". Such\n"
    "comments are non-compliant with the JSON specification but many parsers\n"
    "accept them.\n"
    "\n"
    "The -output-extra-comma flag writes output like \"[1,2,]\", with a comma\n"
    "after the final element of a JSON list or dictionary. Such commas are\n"
    "non-compliant with the JSON specification but many parsers accept them\n"
    "and they can produce simpler line-based diffs. This flag is ignored when\n"
    "-compact-output is set.\n"
    "\n"
    "The -output-inf-nan-numbers flag writes Inf and NaN instead of a\n"
    "substitute null value. Such values are non-compliant with the JSON\n"
    "specification but many parsers accept them.\n"
    "\n"
    "CBOR is more permissive about map keys but JSON only allows strings.\n"
    "When converting from -i=cbor to -o=json, this program rejects keys other\n"
    "than integers and strings (CBOR major types 0, 1, 2 and 3). Integer\n"
    "keys like 123 quoted to be string keys like \"123\".\n"
    "\n"
    "The CBOR specification permits implementations to set their own maximum\n"
    "input depth. This CBOR implementation sets it to 1024.";

// ----

// parse_flags enforces that g_flags.spaces <= 8 (the length of
// INDENT_SPACES_STRING).
#define INDENT_SPACES_STRING "        "
#define INDENT_TAB_STRING "\t"

uint8_t g_dst_array[32768];
wuffs_base__io_buffer g_dst;

uint32_t g_depth;

enum class context {
  none,
  in_list_after_bracket,
  in_list_after_value,
  in_dict_after_brace,
  in_dict_after_key,
  in_dict_after_value,
} g_ctx;

bool g_wrote_to_dst;

std::vector<uint64_t> g_cbor_tags;

struct {
  int remaining_argc;
  char** remaining_argv;

  bool compact_output;
  bool output_cbor_metadata_as_comments;
  bool output_extra_comma;
  bool output_inf_nan_numbers;
  size_t spaces;
  bool tabs;
} g_flags = {0};

std::string  //
parse_flags(int argc, char** argv) {
  g_flags.spaces = 4;

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

    if (!strcmp(arg, "c") || !strcmp(arg, "compact-output")) {
      g_flags.compact_output = true;
      continue;
    }
    if (!strcmp(arg, "output-cbor-metadata-as-comments")) {
      g_flags.output_cbor_metadata_as_comments = true;
      continue;
    }
    if (!strcmp(arg, "output-extra-comma")) {
      g_flags.output_extra_comma = true;
      continue;
    }
    if (!strcmp(arg, "output-inf-nan-numbers")) {
      g_flags.output_inf_nan_numbers = true;
      continue;
    }
    if (!strncmp(arg, "s=", 2) || !strncmp(arg, "spaces=", 7)) {
      while (*arg++ != '=') {
      }
      if (('0' <= arg[0]) && (arg[0] <= '8') && (arg[1] == '\x00')) {
        g_flags.spaces = arg[0] - '0';
        continue;
      }
      return g_usage;
    }
    if (!strcmp(arg, "t") || !strcmp(arg, "tabs")) {
      g_flags.tabs = true;
      continue;
    }

    return g_usage;
  }

  g_flags.remaining_argc = argc - c;
  g_flags.remaining_argv = argv + c;
  return "";
}

// ----

std::string  //
flush_dst() {
  while (true) {
    size_t n = g_dst.reader_length();
    if (n == 0) {
      break;
    }
    ssize_t i = fwrite(g_dst.reader_pointer(), 1, n, stdout);
    if (i >= 0) {
      g_dst.meta.ri += i;
    }
    if (i < n) {
      return "main: error writing to stdout";
    }
  }
  g_dst.compact();
  return "";
}

std::string  //
write_dst(const void* s, size_t n) {
  const uint8_t* p = static_cast<const uint8_t*>(s);
  while (n > 0) {
    size_t i = g_dst.writer_length();
    if (i == 0) {
      TRY(flush_dst());
      i = g_dst.writer_length();
      if (i == 0) {
        return "main: g_dst buffer is full";
      }
    }

    if (i > n) {
      i = n;
    }
    memcpy(g_dst.data.ptr + g_dst.meta.wi, p, i);
    g_dst.meta.wi += i;
    p += i;
    n -= i;
    g_wrote_to_dst = true;
  }
  return "";
}

// ----

class Callbacks : public wuffs_aux::DecodeCborCallbacks {
 public:
  Callbacks() = default;

  std::string WritePreambleAndUpdateContext() {
    // Write preceding punctuation, whitespace and indentation. Update g_ctx.
    do {
      switch (g_ctx) {
        case context::none:
          // No-op.
          break;
        case context::in_list_after_bracket:
          TRY(write_dst("\n", g_flags.compact_output ? 0 : 1));
          g_ctx = context::in_list_after_value;
          break;
        case context::in_list_after_value:
          TRY(write_dst(",\n", g_flags.compact_output ? 1 : 2));
          break;
        case context::in_dict_after_brace:
          TRY(write_dst("\n", g_flags.compact_output ? 0 : 1));
          g_ctx = context::in_dict_after_key;
          break;
        case context::in_dict_after_key:
          TRY(write_dst(": ", g_flags.compact_output ? 1 : 2));
          g_ctx = context::in_dict_after_value;
          goto skip_indentation;
        case context::in_dict_after_value:
          TRY(write_dst(",\n", g_flags.compact_output ? 1 : 2));
          g_ctx = context::in_dict_after_key;
          break;
      }

      if (!g_flags.compact_output) {
        for (size_t i = 0; i < g_depth; i++) {
          TRY(write_dst(g_flags.tabs ? INDENT_TAB_STRING : INDENT_SPACES_STRING,
                        g_flags.tabs ? 1 : g_flags.spaces));
        }
      }
    } while (false);
  skip_indentation:

    // Write any CBOR tags.
    if (g_flags.output_cbor_metadata_as_comments) {
      for (const auto& cbor_tag : g_cbor_tags) {
        uint8_t buf[WUFFS_BASE__U64__BYTE_LENGTH__MAX_INCL];
        size_t n = wuffs_base__render_number_u64(
            wuffs_base__make_slice_u8(&buf[0],
                                      WUFFS_BASE__U64__BYTE_LENGTH__MAX_INCL),
            cbor_tag, WUFFS_BASE__RENDER_NUMBER_XXX__DEFAULT_OPTIONS);
        TRY(write_dst("/*cbor:tag", 10));
        TRY(write_dst(&buf[0], n));
        TRY(write_dst("*/", 2));
      }
      g_cbor_tags.clear();
    }

    return "";
  }

  std::string AppendNull() override {
    TRY(WritePreambleAndUpdateContext());
    if (g_ctx == context::in_dict_after_key) {
      return "main: invalid JSON map key";
    }
    return write_dst("null", 4);
  }

  std::string AppendUndefined() override {
    TRY(WritePreambleAndUpdateContext());
    if (g_ctx == context::in_dict_after_key) {
      return "main: invalid JSON map key";
    }
    // JSON's closest approximation to "undefined" is "null".
    if (g_flags.output_cbor_metadata_as_comments) {
      return write_dst("/*cbor:undefined*/null", 22);
    }
    return write_dst("null", 4);
  }

  std::string AppendBool(bool val) override {
    TRY(WritePreambleAndUpdateContext());
    if (g_ctx == context::in_dict_after_key) {
      return "main: invalid JSON map key";
    }
    if (val) {
      return write_dst("true", 4);
    }
    return write_dst("false", 5);
  }

  std::string AppendF64(double val) override {
    TRY(WritePreambleAndUpdateContext());
    if (g_ctx == context::in_dict_after_key) {
      return "main: invalid JSON map key";
    }

    uint8_t buf[64];
    constexpr uint32_t precision = 0;
    size_t n = wuffs_base__render_number_f64(
        wuffs_base__make_slice_u8(&buf[0], sizeof buf), val, precision,
        WUFFS_BASE__RENDER_NUMBER_FXX__JUST_ENOUGH_PRECISION);
    if (!g_flags.output_inf_nan_numbers) {
      // JSON numbers don't include Infinities or NaNs. For such numbers, their
      // IEEE 754 bit representation's 11 exponent bits are all on.
      uint64_t u =
          wuffs_base__ieee_754_bit_representation__from_f64_to_u64(val);
      if (((u >> 52) & 0x7FF) == 0x7FF) {
        if (g_flags.output_cbor_metadata_as_comments) {
          TRY(write_dst("/*cbor:", 7));
          TRY(write_dst(&buf[0], n));
          TRY(write_dst("*/", 2));
        }
        return write_dst("null", 4);
      }
    }
    return write_dst(&buf[0], n);
  }

  std::string AppendI64(int64_t val) override {
    TRY(WritePreambleAndUpdateContext());
    if (g_ctx == context::in_dict_after_key) {
      TRY(write_dst("\"", 1));
    }

    uint8_t buf[WUFFS_BASE__I64__BYTE_LENGTH__MAX_INCL];
    size_t n = wuffs_base__render_number_i64(
        wuffs_base__make_slice_u8(&buf[0], sizeof buf), val,
        WUFFS_BASE__RENDER_NUMBER_XXX__DEFAULT_OPTIONS);
    TRY(write_dst(&buf[0], n));

    if (g_ctx == context::in_dict_after_key) {
      TRY(write_dst("\"", 1));
    }
    return "";
  }

  std::string AppendU64(uint64_t val) override {
    TRY(WritePreambleAndUpdateContext());
    if (g_ctx == context::in_dict_after_key) {
      TRY(write_dst("\"", 1));
    }

    uint8_t buf[WUFFS_BASE__U64__BYTE_LENGTH__MAX_INCL];
    size_t n = wuffs_base__render_number_u64(
        wuffs_base__make_slice_u8(&buf[0], sizeof buf), val,
        WUFFS_BASE__RENDER_NUMBER_XXX__DEFAULT_OPTIONS);
    TRY(write_dst(&buf[0], n));

    if (g_ctx == context::in_dict_after_key) {
      TRY(write_dst("\"", 1));
    }
    return "";
  }

  std::string AppendByteString(std::string&& val) override {
    TRY(WritePreambleAndUpdateContext());
    if (g_flags.output_cbor_metadata_as_comments) {
      TRY(write_dst("/*cbor:base64url*/\"", 19));
    } else {
      TRY(write_dst("\"", 1));
    }

    const uint8_t* ptr =
        static_cast<const uint8_t*>(static_cast<const void*>(val.data()));
    size_t len = val.length();
    while (len > 0) {
      constexpr bool closed = true;
      wuffs_base__transform__output o = wuffs_base__base_64__encode(
          g_dst.writer_slice(),
          wuffs_base__make_slice_u8(const_cast<uint8_t*>(ptr), len), closed,
          WUFFS_BASE__BASE_64__URL_ALPHABET);
      g_dst.meta.wi += o.num_dst;
      ptr += o.num_src;
      len -= o.num_src;
      if (o.status.repr == nullptr) {
        if (len != 0) {
          return "main: internal error: inconsistent base-64 length";
        }
        break;
      } else if (o.status.repr != wuffs_base__suspension__short_write) {
        return o.status.message();
      }
      TRY(flush_dst());
    }

    return write_dst("\"", 1);
  }

  std::string AppendTextString(std::string&& val) override {
    TRY(WritePreambleAndUpdateContext());
    TRY(write_dst("\"", 1));
    const uint8_t* ptr =
        static_cast<const uint8_t*>(static_cast<const void*>(val.data()));
    size_t len = val.length();
  loop:
    if (len > 0) {
      for (size_t i = 0; i < len; i++) {
        uint8_t c = ptr[i];
        if ((c == '"') || (c == '\\') || (c < 0x20)) {
          TRY(write_dst(ptr, i));
          TRY(AppendAsciiByte(c));
          ptr += i + 1;
          len -= i + 1;
          goto loop;
        }
      }
      TRY(write_dst(ptr, len));
    }
    return write_dst("\"", 1);
  }

  std::string AppendAsciiByte(uint8_t c) {
    switch (c) {
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
      case '\"':
        return write_dst("\\\"", 2);
      case '\\':
        return write_dst("\\\\", 2);
    }
    static const char* hex_digits = "0123456789ABCDEF";
    uint8_t esc6[6];
    esc6[0] = '\\';
    esc6[1] = 'u';
    esc6[2] = '0';
    esc6[3] = '0';
    esc6[4] = hex_digits[c >> 4];
    esc6[5] = hex_digits[c & 0x1F];
    return write_dst(&esc6[0], 6);
  }

  std::string AppendMinus1MinusX(uint64_t val) override {
    TRY(WritePreambleAndUpdateContext());
    if (g_ctx == context::in_dict_after_key) {
      TRY(write_dst("\"", 1));
    }

    val++;
    if (val == 0) {
      // See the cbor.TOKEN_VALUE_MINOR__MINUS_1_MINUS_X comment re overflow.
      TRY(write_dst("-18446744073709551616", 21));
    } else {
      uint8_t buf[1 + WUFFS_BASE__U64__BYTE_LENGTH__MAX_INCL];
      uint8_t* b = &buf[0];
      *b++ = '-';
      size_t n = wuffs_base__render_number_u64(
          wuffs_base__make_slice_u8(b, WUFFS_BASE__U64__BYTE_LENGTH__MAX_INCL),
          val, WUFFS_BASE__RENDER_NUMBER_XXX__DEFAULT_OPTIONS);
      TRY(write_dst(&buf[0], 1 + n));
    }

    if (g_ctx == context::in_dict_after_key) {
      TRY(write_dst("\"", 1));
    }
    return "";
  }

  std::string AppendCborSimpleValue(uint8_t val) override {
    TRY(WritePreambleAndUpdateContext());
    if (g_ctx == context::in_dict_after_key) {
      return "main: invalid JSON map key";
    }

    if (!g_flags.output_cbor_metadata_as_comments) {
      return write_dst("null", 4);
    }
    uint8_t buf[WUFFS_BASE__U64__BYTE_LENGTH__MAX_INCL];
    size_t n = wuffs_base__render_number_u64(
        wuffs_base__make_slice_u8(&buf[0],
                                  WUFFS_BASE__U64__BYTE_LENGTH__MAX_INCL),
        val, WUFFS_BASE__RENDER_NUMBER_XXX__DEFAULT_OPTIONS);
    TRY(write_dst("/*cbor:simple", 13));
    TRY(write_dst(&buf[0], n));
    return write_dst("*/null", 6);
  }

  std::string AppendCborTag(uint64_t val) override {
    // No call to WritePreambleAndUpdateContext. A CBOR tag isn't a value. It
    // decorates the upcoming value.
    if (g_flags.output_cbor_metadata_as_comments) {
      g_cbor_tags.push_back(val);
    }
    return "";
  }

  std::string Push(uint32_t flags) override {
    TRY(WritePreambleAndUpdateContext());
    if (g_ctx == context::in_dict_after_key) {
      return "main: invalid JSON map key";
    }

    g_depth++;
    g_ctx = (flags & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST)
                ? context::in_list_after_bracket
                : context::in_dict_after_brace;
    return write_dst(
        (flags & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST) ? "[" : "{", 1);
  }

  std::string Pop(uint32_t flags) override {
    // No call to WritePreambleAndUpdateContext. We write the extra comma,
    // outdent, etc. ourselves.
    g_depth--;
    if (g_flags.compact_output) {
      // No-op.
    } else if ((g_ctx != context::in_list_after_bracket) &&
               (g_ctx != context::in_dict_after_brace)) {
      if (g_flags.output_extra_comma) {
        TRY(write_dst(",\n", 2));
      } else {
        TRY(write_dst("\n", 1));
      }
      for (size_t i = 0; i < g_depth; i++) {
        TRY(write_dst(g_flags.tabs ? INDENT_TAB_STRING : INDENT_SPACES_STRING,
                      g_flags.tabs ? 1 : g_flags.spaces));
      }
    }
    g_ctx = (flags & WUFFS_BASE__TOKEN__VBD__STRUCTURE__TO_LIST)
                ? context::in_list_after_value
                : context::in_dict_after_value;
    return write_dst(
        (flags & WUFFS_BASE__TOKEN__VBD__STRUCTURE__FROM_LIST) ? "]" : "}", 1);
  }
};

// ----

std::string  //
main1(int argc, char** argv) {
  g_dst = wuffs_base__ptr_u8__writer(
      &g_dst_array[0], (sizeof(g_dst_array) / sizeof(g_dst_array[0])));
  g_depth = 0;
  g_ctx = context::none;
  g_wrote_to_dst = false;

  TRY(parse_flags(argc, argv));

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
  return wuffs_aux::DecodeCbor(callbacks, input).error_message;
}

// ----

int  //
compute_exit_code(std::string status_msg) {
  if (status_msg.empty()) {
    return 0;
  }
  fputs(status_msg.c_str(), stderr);
  fputc('\n', stderr);
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
  if (g_wrote_to_dst) {
    std::string z1 = write_dst("\n", 1);
    std::string z2 = flush_dst();
    z = !z.empty() ? z : (!z1.empty() ? z1 : z2);
  }
  return compute_exit_code(z);
}
