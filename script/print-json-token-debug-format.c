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

// print-json-token-debug-format.c parses JSON from stdin and prints the
// resulting token stream, eliding any non-essential (e.g. whitespace) tokens.
//
// The output format is only for debugging, and certainly not for long term
// storage. It isn't guaranteed to be stable between versions of this program
// and of the Wuffs standard library.
//
// It prints 16 bytes (4 big-endian uint32's) per token: the token's position
// (total length of all previous tokens), length, major value and minor value.
// Together with the hexadecimal WUFFS_BASE__TOKEN__ETC constants defined in
// token-public.h, this format is somewhat human-readable when piped through a
// hex-dump program (e.g. /usr/bin/hd).
//
// If the input or output is larger than the program's buffers (64 MiB and
// 131072 tokens by default), there may be multiple valid tokenizations of any
// given input. For example, if a source string "abcde" straddles an I/O
// boundary, it may be tokenized as a 5-length complete string or as a 3-length
// incomplete string followed by a 2-length complete string.
//
// A Wuffs token stream, in general, can support inputs more than (1 << 32)
// bytes long, but this program can not, as it tracks the tokens' cumulative
// position as a uint32.

#include <inttypes.h>
#include <stdio.h>
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
#include "../release/c/wuffs-unsupported-snapshot.c"

#ifndef SRC_BUFFER_SIZE
#define SRC_BUFFER_SIZE (64 * 1024 * 1024)
#endif
#ifndef TOKEN_BUFFER_SIZE
#define TOKEN_BUFFER_SIZE (128 * 1024)
#endif

uint8_t src_buffer[SRC_BUFFER_SIZE];
wuffs_base__token tok_buffer[TOKEN_BUFFER_SIZE];

wuffs_base__io_buffer src;
wuffs_base__token_buffer tok;

wuffs_json__decoder dec;
wuffs_base__status dec_status;

#define TRY(error_msg)         \
  do {                         \
    const char* z = error_msg; \
    if (z) {                   \
      return z;                \
    }                          \
  } while (false)

const char*  //
read_src() {
  if (src.meta.closed) {
    return "main: internal error: read requested on a closed source";
  }
  wuffs_base__io_buffer__compact(&src);
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
  return NULL;
}

// ----

const char*  //
main1(int argc, char** argv) {
  src = wuffs_base__make_io_buffer(
      wuffs_base__make_slice_u8(src_buffer, SRC_BUFFER_SIZE),
      wuffs_base__empty_io_buffer_meta());

  tok = wuffs_base__make_token_buffer(
      wuffs_base__make_slice_token(tok_buffer, TOKEN_BUFFER_SIZE),
      wuffs_base__empty_token_buffer_meta());

  wuffs_base__status init_status = wuffs_json__decoder__initialize(
      &dec, sizeof__wuffs_json__decoder(), WUFFS_VERSION, 0);
  if (!wuffs_base__status__is_ok(&init_status)) {
    return wuffs_base__status__message(&init_status);
  }

  uint64_t pos = 0;
  while (true) {
    wuffs_base__status status =
        wuffs_json__decoder__decode_tokens(&dec, &tok, &src);

    while (tok.meta.ri < tok.meta.wi) {
      wuffs_base__token* t = &tok.data.ptr[tok.meta.ri++];
      uint64_t len = wuffs_base__token__length(t);

      if (wuffs_base__token__value(t) != 0) {
        uint64_t maj = wuffs_base__token__value_major(t);
        uint64_t min = wuffs_base__token__value_minor(t);

        uint8_t buf[16];
        wuffs_base__store_u32be__no_bounds_check(&buf[0 * 4], (uint32_t)(pos));
        wuffs_base__store_u32be__no_bounds_check(&buf[1 * 4], (uint32_t)(len));
        wuffs_base__store_u32be__no_bounds_check(&buf[2 * 4], (uint32_t)(maj));
        wuffs_base__store_u32be__no_bounds_check(&buf[3 * 4], (uint32_t)(min));
        const int stdout_fd = 1;
        write(stdout_fd, &buf[0], 16);
      }

      pos += len;
      if (pos > 0xFFFFFFFF) {
        return "main: input is too long";
      }
    }

    if (status.repr == NULL) {
      return NULL;
    } else if (status.repr == wuffs_base__suspension__short_read) {
      TRY(read_src());
    } else if (status.repr == wuffs_base__suspension__short_write) {
      wuffs_base__token_buffer__compact(&tok);
    } else {
      return wuffs_base__status__message(&status);
    }
  }
}

// ----

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
  const char* z = main1(argc, argv);
  int exit_code = compute_exit_code(z);
  return exit_code;
}
