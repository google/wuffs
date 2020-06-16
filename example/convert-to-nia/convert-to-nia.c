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
convert-to-nia converts an image from stdin (e.g. in the BMP, GIF, JPEG or PNG
format) to stdout (in the NIA/NIE format).

See the "const char* g_usage" string below for details.

An equivalent program (using the Chromium image codecs) is at:
https://chromium-review.googlesource.com/c/chromium/src/+/2210331

An equivalent program (using the Skia image codecs) is at:
https://skia-review.googlesource.com/c/skia/+/290618
*/

#include <errno.h>
#include <inttypes.h>
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
#define WUFFS_CONFIG__MODULE__BMP
#define WUFFS_CONFIG__MODULE__GIF
#define WUFFS_CONFIG__MODULE__LZW
#define WUFFS_CONFIG__MODULE__WBMP

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../release/c/wuffs-unsupported-snapshot.c"

// ----

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

static const char* g_usage =
    "Usage: convert-to-nia -flags < src.img > dst.nia\n"
    "\n"
    "Flags:\n"
    "    -1      -first-frame-only\n"
    "            -fail-if-unsandboxed\n"
    "\n"
    "convert-to-nia converts an image from stdin (e.g. in the BMP, GIF, JPEG\n"
    "or PNG format) to stdout (in the NIA format, or in the NIE format if\n"
    "the -first-frame-only flag is given).\n"
    "\n"
    "NIA/NIE is a trivial animated/still image file format, specified at\n"
    "https://github.com/google/wuffs/blob/master/doc/spec/nie-spec.md\n"
    "\n"
    "The -fail-if-unsandboxed flag causes the program to exit if it does not\n"
    "self-impose a sandbox. On Linux, it self-imposes a SECCOMP_MODE_STRICT\n"
    "sandbox, regardless of whether this flag was set.";

// ----

#define STDIN_FD 0
#define STDOUT_FD 1
#define STDERR_FD 2

bool g_sandboxed = false;

wuffs_base__pixel_buffer g_pixbuf = {0};
wuffs_base__slice_u8 g_pixbuf_slice = {0};
wuffs_base__slice_u8 g_pixbuf_backup_slice = {0};
wuffs_base__io_buffer g_src = {0};
wuffs_base__slice_u8 g_workbuf_slice = {0};

wuffs_base__image_config g_image_config = {0};
wuffs_base__frame_config g_frame_config = {0};
uint32_t g_width = 0;
uint32_t g_height = 0;

wuffs_base__image_decoder* g_image_decoder = NULL;
union {
  wuffs_bmp__decoder bmp;
  wuffs_gif__decoder gif;
  wuffs_wbmp__decoder wbmp;
} g_potential_decoders;

// ----

#define BYTES_PER_PIXEL 4

#ifndef MAX_DIMENSION
#define MAX_DIMENSION 65535
#endif

#ifndef SRC_BUFFER_ARRAY_SIZE
#define SRC_BUFFER_ARRAY_SIZE (64 * 1024)
#endif

#ifndef WORKBUF_ARRAY_SIZE
#define WORKBUF_ARRAY_SIZE (1024 * 1024)
#endif

#ifndef PIXBUF_ARRAY_SIZE
#define PIXBUF_ARRAY_SIZE (256 * 1024 * 1024)
#endif

uint8_t g_src_buffer_array[SRC_BUFFER_ARRAY_SIZE] = {0};
uint8_t g_workbuf_array[WORKBUF_ARRAY_SIZE] = {0};
uint8_t g_pixbuf_array[PIXBUF_ARRAY_SIZE] = {0};

// ----

struct {
  int remaining_argc;
  char** remaining_argv;

  bool fail_if_unsandboxed;
  bool first_frame_only;
} g_flags = {0};

const char*  //
parse_flags(int argc, char** argv) {
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

    if (!strcmp(arg, "fail-if-unsandboxed")) {
      g_flags.fail_if_unsandboxed = true;
      continue;
    }
    if (!strcmp(arg, "1") || !strcmp(arg, "first-frame-only")) {
      g_flags.first_frame_only = true;
      continue;
    }

    return g_usage;
  }

  g_flags.remaining_argc = argc - c;
  g_flags.remaining_argv = argv + c;
  return NULL;
}

// ----

// ignore_return_value suppresses errors from -Wall -Werror.
static void  //
ignore_return_value(int ignored) {}

const char*  //
read_more_src() {
  if (g_src.meta.closed) {
    return "main: unexpected end of file";
  }
  wuffs_base__io_buffer__compact(&g_src);
  ssize_t n = read(STDIN_FD, g_src.data.ptr + g_src.meta.wi,
                   g_src.data.len - g_src.meta.wi);
  if (n > 0) {
    g_src.meta.wi += n;
  } else if (errno == 0) {
    if (n < 0) {
      return "main: unexpected negative-count read";
    }
    g_src.meta.closed = true;
  } else if (errno != EINTR) {
    return strerror(errno);
  }
  return NULL;
}

const char*  //
load_image_type() {
  while (g_src.meta.ri >= g_src.meta.wi) {
    TRY(read_more_src());
  }

  wuffs_base__status status;
  switch (g_src_buffer_array[0]) {
    case '\x00':
      status = wuffs_wbmp__decoder__initialize(
          &g_potential_decoders.wbmp, sizeof g_potential_decoders.wbmp,
          WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
      TRY(wuffs_base__status__message(&status));
      g_image_decoder =
          wuffs_wbmp__decoder__upcast_as__wuffs_base__image_decoder(
              &g_potential_decoders.wbmp);
      break;

    case 'B':
      status = wuffs_bmp__decoder__initialize(
          &g_potential_decoders.bmp, sizeof g_potential_decoders.bmp,
          WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
      TRY(wuffs_base__status__message(&status));
      g_image_decoder =
          wuffs_bmp__decoder__upcast_as__wuffs_base__image_decoder(
              &g_potential_decoders.bmp);
      break;

    case 'G':
      status = wuffs_gif__decoder__initialize(
          &g_potential_decoders.gif, sizeof g_potential_decoders.gif,
          WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
      TRY(wuffs_base__status__message(&status));
      g_image_decoder =
          wuffs_gif__decoder__upcast_as__wuffs_base__image_decoder(
              &g_potential_decoders.gif);
      break;

    default:
      return "main: unrecognized file format";
  }
  return NULL;
}

const char*  //
load_image_config() {
  // Decode the wuffs_base__image_config.
  while (true) {
    wuffs_base__status status = wuffs_base__image_decoder__decode_image_config(
        g_image_decoder, &g_image_config, &g_src);
    if (status.repr == NULL) {
      break;
    } else if (status.repr != wuffs_base__suspension__short_read) {
      return wuffs_base__status__message(&status);
    }
    TRY(read_more_src());
  }

  // Read the dimensions.
  uint32_t w = wuffs_base__pixel_config__width(&g_image_config.pixcfg);
  uint32_t h = wuffs_base__pixel_config__height(&g_image_config.pixcfg);
  if ((w > MAX_DIMENSION) || (h > MAX_DIMENSION)) {
    return "main: image is too large";
  }
  g_width = w;
  g_height = h;

  // Override the image's native pixel format to be BGRA_NONPREMUL.
  wuffs_base__pixel_config__set(&g_image_config.pixcfg,
                                WUFFS_BASE__PIXEL_FORMAT__BGRA_NONPREMUL,
                                WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, w, h);

  // Configure the work buffer.
  uint64_t workbuf_len =
      wuffs_base__image_decoder__workbuf_len(g_image_decoder).max_incl;
  if (workbuf_len > WORKBUF_ARRAY_SIZE) {
    return "main: image is too large (to configure work buffer)";
  }
  g_workbuf_slice.ptr = &g_workbuf_array[0];
  g_workbuf_slice.len = workbuf_len;

  // Configure the pixel buffer and (if there's capacity) its backup buffer.
  uint64_t num_pixels = ((uint64_t)w) * ((uint64_t)h);
  if (num_pixels > (PIXBUF_ARRAY_SIZE / BYTES_PER_PIXEL)) {
    return "main: image is too large (to configure pixel buffer)";
  }
  g_pixbuf_slice.ptr = &g_pixbuf_array[0];
  g_pixbuf_slice.len = num_pixels * BYTES_PER_PIXEL;
  size_t pixbuf_array_remaining = PIXBUF_ARRAY_SIZE - g_pixbuf_slice.len;
  if (pixbuf_array_remaining >= g_pixbuf_slice.len) {
    g_pixbuf_backup_slice.ptr = &g_pixbuf_array[g_pixbuf_slice.len];
    g_pixbuf_backup_slice.len = g_pixbuf_slice.len;
  }

  // Configure the wuffs_base__pixel_buffer struct.
  wuffs_base__status status = wuffs_base__pixel_buffer__set_from_slice(
      &g_pixbuf, &g_image_config.pixcfg, g_pixbuf_slice);
  TRY(wuffs_base__status__message(&status));

  wuffs_base__table_u8 tab = wuffs_base__pixel_buffer__plane(&g_pixbuf, 0);
  if ((tab.width != (g_width * BYTES_PER_PIXEL)) || (tab.height != g_height)) {
    return "main: inconsistent pixel buffer dimensions";
  }

  return NULL;
}

void  //
fill_rectangle(wuffs_base__rect_ie_u32 rect,
               wuffs_base__color_u32_argb_premul color) {
  if (rect.max_excl_x > g_width) {
    rect.max_excl_x = g_width;
  }
  if (rect.max_excl_y > g_height) {
    rect.max_excl_y = g_height;
  }
  uint32_t nonpremul =
      wuffs_base__color_u32_argb_premul__as__color_u32_argb_nonpremul(color);
  wuffs_base__table_u8 tab = wuffs_base__pixel_buffer__plane(&g_pixbuf, 0);

  uint32_t y;
  for (y = rect.min_incl_y; y < rect.max_excl_y; y++) {
    uint8_t* p =
        tab.ptr + (y * tab.stride) + (rect.min_incl_x * BYTES_PER_PIXEL);
    uint32_t x;
    for (x = rect.min_incl_x; x < rect.max_excl_x; x++) {
      wuffs_base__store_u32le__no_bounds_check(p, nonpremul);
      p += BYTES_PER_PIXEL;
    }
  }
}

void  //
print_nix_header(uint32_t magic_u32le) {
  static const uint32_t version1_bn4_u32le = 0x346E62FF;
  uint8_t data[16];
  wuffs_base__store_u32le__no_bounds_check(data + 0x00, magic_u32le);
  wuffs_base__store_u32le__no_bounds_check(data + 0x04, version1_bn4_u32le);
  wuffs_base__store_u32le__no_bounds_check(data + 0x08, g_width);
  wuffs_base__store_u32le__no_bounds_check(data + 0x0C, g_height);
  ignore_return_value(write(STDOUT_FD, &data[0], 16));
}

void  //
print_nia_duration(wuffs_base__flicks duration) {
  uint8_t data[8];
  wuffs_base__store_u64le__no_bounds_check(data + 0x00, duration);
  ignore_return_value(write(STDOUT_FD, &data[0], 8));
}

void  //
print_nie_frame() {
  print_nix_header(0x45AFC36E);  // "nïE" as a u32le.
  wuffs_base__table_u8 tab = wuffs_base__pixel_buffer__plane(&g_pixbuf, 0);
  if (tab.width == tab.stride) {
    ignore_return_value(write(STDOUT_FD, tab.ptr, tab.width * tab.height));
  } else {
    size_t y;
    for (y = 0; y < tab.height; y++) {
      ignore_return_value(
          write(STDOUT_FD, tab.ptr + (y * tab.stride), tab.width));
      break;
    }
  }
}

void  //
print_nia_padding() {
  if (g_width & g_height & 1) {
    uint8_t data[4];
    wuffs_base__store_u32le__no_bounds_check(data + 0x00, 0);
    ignore_return_value(write(STDOUT_FD, &data[0], 4));
  }
}

void  //
print_nia_footer() {
  uint8_t data[8];
  wuffs_base__store_u32le__no_bounds_check(
      data + 0x00,
      wuffs_base__image_decoder__num_animation_loops(g_image_decoder));
  wuffs_base__store_u32le__no_bounds_check(data + 0x04, 0x80000000);
  ignore_return_value(write(STDOUT_FD, &data[0], 8));
}

const char*  //
main1(int argc, char** argv) {
  TRY(parse_flags(argc, argv));
  if (g_flags.fail_if_unsandboxed && !g_sandboxed) {
    return "main: unsandboxed";
  }

  g_src.data.ptr = g_src_buffer_array;
  g_src.data.len = SRC_BUFFER_ARRAY_SIZE;

  TRY(load_image_type());
  TRY(load_image_config());
  if (!g_flags.first_frame_only) {
    print_nix_header(0x41AFC36E);  // "nïA" as a u32le.
  }

  wuffs_base__flicks total_duration = 0;
  while (true) {
    // Decode the wuffs_base__frame_config.
    while (true) {
      wuffs_base__status dfc_status =
          wuffs_base__image_decoder__decode_frame_config(
              g_image_decoder, &g_frame_config, &g_src);
      if (dfc_status.repr == NULL) {
        break;
      } else if (dfc_status.repr == wuffs_base__note__end_of_data) {
        goto done;
      } else if (dfc_status.repr != wuffs_base__suspension__short_read) {
        return wuffs_base__status__message(&dfc_status);
      }
      TRY(read_more_src());
    }

    wuffs_base__flicks duration =
        wuffs_base__frame_config__duration(&g_frame_config);
    if (duration < 0) {
      return "main: animation frame duration is negative";
    } else if (total_duration > (INT64_MAX - duration)) {
      return "main: animation frame duration overflow";
    }
    total_duration += duration;
    if (!g_flags.first_frame_only) {
      print_nia_duration(total_duration);
    }

    if (wuffs_base__frame_config__index(&g_frame_config) == 0) {
      fill_rectangle(
          wuffs_base__pixel_config__bounds(&g_image_config.pixcfg),
          wuffs_base__frame_config__background_color(&g_frame_config));
    }

    switch (wuffs_base__frame_config__disposal(&g_frame_config)) {
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS: {
        if (g_pixbuf_slice.len != g_pixbuf_backup_slice.len) {
          return "main: image is too large (to configure pixel backup buffer)";
        }
        memcpy(g_pixbuf_backup_slice.ptr, g_pixbuf_slice.ptr,
               g_pixbuf_slice.len);
        break;
      }
    }

    // Decode the frame (the pixels).
    wuffs_base__status df_status;
    while (true) {
      df_status = wuffs_base__image_decoder__decode_frame(
          g_image_decoder, &g_pixbuf, &g_src,
          wuffs_base__frame_config__overwrite_instead_of_blend(&g_frame_config)
              ? WUFFS_BASE__PIXEL_BLEND__SRC
              : WUFFS_BASE__PIXEL_BLEND__SRC_OVER,
          g_workbuf_slice, NULL);
      if (df_status.repr != wuffs_base__suspension__short_read) {
        break;
      }
      TRY(read_more_src());
    }

    print_nie_frame();

    if (df_status.repr != NULL) {
      return wuffs_base__status__message(&df_status);
    } else if (g_flags.first_frame_only) {
      return NULL;
    }
    print_nia_padding();

    switch (wuffs_base__frame_config__disposal(&g_frame_config)) {
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_BACKGROUND: {
        fill_rectangle(
            wuffs_base__frame_config__bounds(&g_frame_config),
            wuffs_base__frame_config__background_color(&g_frame_config));
        break;
      }
      case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS: {
        if (g_pixbuf_slice.len != g_pixbuf_backup_slice.len) {
          return "main: image is too large (to configure pixel backup buffer)";
        }
        memcpy(g_pixbuf_slice.ptr, g_pixbuf_backup_slice.ptr,
               g_pixbuf_slice.len);
        break;
      }
    }
  }

done:
  print_nia_footer();
  return NULL;
}

int  //
compute_exit_code(const char* status_msg) {
  if (!status_msg) {
    return 0;
  }
  size_t n;
  if (status_msg == g_usage) {
    n = strlen(status_msg);
  } else {
    n = strnlen(status_msg, 2047);
    if (n >= 2047) {
      status_msg = "main: internal error: error message is too long";
      n = strnlen(status_msg, 2047);
    }
  }
  ignore_return_value(write(STDERR_FD, status_msg, n));
  ignore_return_value(write(STDERR_FD, "\n", 1));
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
  g_sandboxed = true;
#endif

  int exit_code = compute_exit_code(main1(argc, argv));

#if defined(WUFFS_EXAMPLE_USE_SECCOMP)
  // Call SYS_exit explicitly, instead of calling SYS_exit_group implicitly by
  // either calling _exit or returning from main. SECCOMP_MODE_STRICT allows
  // only SYS_exit.
  syscall(SYS_exit, exit_code);
#endif
  return exit_code;
}
