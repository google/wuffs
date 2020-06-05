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
imageviewer is a simple GUI program for viewing images. On Linux, GUI means
X11. To run:

$CC imageviewer.c -lxcb -lxcb-image && \
  ./a.out ../../test/data/bricks-*.gif; rm -f a.out

for a C compiler $CC, such as clang or gcc.

The Space and BackSpace keys cycle through the files, if more than one was
given as command line arguments. If none were given, the program reads from
stdin.

The Return key is equivalent to the Space key.

The Escape key quits.
*/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

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

// X11 limits its image dimensions to uint16_t.
#define MAX_DIMENSION 65535

#define NUM_BACKGROUND_COLORS 3
#define SRC_BUFFER_ARRAY_SIZE (64 * 1024)

wuffs_base__color_u32_argb_premul g_background_colors[NUM_BACKGROUND_COLORS] = {
    0xFF000000,
    0xFFFFFFFF,
    0xFFA9009A,
};

FILE* g_file = NULL;
const char* g_filename = NULL;
uint32_t g_width = 0;
uint32_t g_height = 0;
wuffs_base__slice_u8 g_workbuf_slice = {0};
wuffs_base__slice_u8 g_pixbuf_slice = {0};
wuffs_base__pixel_buffer g_pixbuf = {0};
uint8_t g_src_buffer_array[SRC_BUFFER_ARRAY_SIZE] = {0};
wuffs_base__io_buffer g_src = {0};
wuffs_base__image_config g_image_config = {0};
wuffs_base__frame_config g_frame_config = {0};
wuffs_base__image_decoder* g_image_decoder = NULL;
uint32_t g_background_color_index = 0;

union {
  wuffs_bmp__decoder bmp;
  wuffs_gif__decoder gif;
  wuffs_wbmp__decoder wbmp;
} g_potential_decoders;

bool  //
read_more_src() {
  if (g_src.meta.closed) {
    printf("%s: unexpected end of file\n", g_filename);
    return false;
  }
  wuffs_base__io_buffer__compact(&g_src);
  g_src.meta.wi += fread(g_src.data.ptr + g_src.meta.wi, sizeof(uint8_t),
                         g_src.data.len - g_src.meta.wi, g_file);
  if (feof(g_file)) {
    g_src.meta.closed = true;
  } else if (ferror(g_file)) {
    printf("%s: read error\n", g_filename);
    return false;
  }
  return true;
}

bool  //
load_image_type() {
  while (g_src.meta.wi == 0) {
    if (!read_more_src()) {
      return false;
    }
  }

  wuffs_base__status status;
  switch (g_src_buffer_array[0]) {
    case '\x00':
      status = wuffs_wbmp__decoder__initialize(
          &g_potential_decoders.wbmp, sizeof g_potential_decoders.wbmp,
          WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
      if (!wuffs_base__status__is_ok(&status)) {
        printf("%s: %s\n", g_filename, wuffs_base__status__message(&status));
        return false;
      }
      g_image_decoder =
          wuffs_wbmp__decoder__upcast_as__wuffs_base__image_decoder(
              &g_potential_decoders.wbmp);
      break;

    case 'B':
      status = wuffs_bmp__decoder__initialize(
          &g_potential_decoders.bmp, sizeof g_potential_decoders.bmp,
          WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
      if (!wuffs_base__status__is_ok(&status)) {
        printf("%s: %s\n", g_filename, wuffs_base__status__message(&status));
        return false;
      }
      g_image_decoder =
          wuffs_bmp__decoder__upcast_as__wuffs_base__image_decoder(
              &g_potential_decoders.bmp);
      break;

    case 'G':
      status = wuffs_gif__decoder__initialize(
          &g_potential_decoders.gif, sizeof g_potential_decoders.gif,
          WUFFS_VERSION, WUFFS_INITIALIZE__DEFAULT_OPTIONS);
      if (!wuffs_base__status__is_ok(&status)) {
        printf("%s: %s\n", g_filename, wuffs_base__status__message(&status));
        return false;
      }
      g_image_decoder =
          wuffs_gif__decoder__upcast_as__wuffs_base__image_decoder(
              &g_potential_decoders.gif);
      break;

    default:
      printf("%s: unrecognized file format\n", g_filename);
      return false;
  }
  return true;
}

bool  //
load_image_config() {
  // Decode the wuffs_base__image_config.
  while (true) {
    wuffs_base__status status = wuffs_base__image_decoder__decode_image_config(
        g_image_decoder, &g_image_config, &g_src);

    if (status.repr == NULL) {
      break;
    } else if (status.repr != wuffs_base__suspension__short_read) {
      // TODO: handle wuffs_base__note__i_o_redirect.
      printf("%s: %s\n", g_filename, wuffs_base__status__message(&status));
      return false;
    }

    if (!read_more_src()) {
      return false;
    }
  }

  // Read the dimensions.
  uint32_t w = wuffs_base__pixel_config__width(&g_image_config.pixcfg);
  uint32_t h = wuffs_base__pixel_config__height(&g_image_config.pixcfg);
  if ((w > MAX_DIMENSION) || (h > MAX_DIMENSION)) {
    printf("%s: image is too large\n", g_filename);
    return false;
  }
  g_width = w;
  g_height = h;

  // Override the image's native pixel format to be BGRA_PREMUL.
  wuffs_base__pixel_config__set(&g_image_config.pixcfg,
                                WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL,
                                WUFFS_BASE__PIXEL_SUBSAMPLING__NONE, w, h);

  // Allocate the work buffer memory.
  uint64_t workbuf_len =
      wuffs_base__image_decoder__workbuf_len(g_image_decoder).max_incl;
  if (workbuf_len > SIZE_MAX) {
    printf("%s: out of memory\n", g_filename);
    return false;
  }
  if (workbuf_len > 0) {
    void* p = malloc(workbuf_len);
    if (!p) {
      printf("%s: out of memory\n", g_filename);
      return false;
    }
    g_workbuf_slice.ptr = (uint8_t*)p;
    g_workbuf_slice.len = workbuf_len;
  }

  // Allocate the pixel buffer memory.
  uint64_t num_pixels = ((uint64_t)w) * ((uint64_t)h);
  if (num_pixels > (SIZE_MAX / sizeof(wuffs_base__color_u32_argb_premul))) {
    printf("%s: image is too large\n", g_filename);
    return false;
  }
  size_t n = num_pixels * sizeof(wuffs_base__color_u32_argb_premul);
  void* p = malloc(n);
  if (!p) {
    printf("%s: out of memory\n", g_filename);
    return false;
  }
  {
    uint8_t* ptr = (uint8_t*)p;
    wuffs_base__color_u32_argb_premul color =
        g_background_colors[g_background_color_index];
    for (size_t i = 0; i < num_pixels; i++) {
      wuffs_base__store_u32le__no_bounds_check(ptr, color);
      ptr += 4;
    }
  }
  g_pixbuf_slice.ptr = (uint8_t*)p;
  g_pixbuf_slice.len = n;

  // Configure the wuffs_base__pixel_buffer struct.
  wuffs_base__status status = wuffs_base__pixel_buffer__set_from_slice(
      &g_pixbuf, &g_image_config.pixcfg, g_pixbuf_slice);
  if (!wuffs_base__status__is_ok(&status)) {
    printf("%s: %s\n", g_filename, wuffs_base__status__message(&status));
    return false;
  }

  return true;
}

bool  //
load_image_frame() {
  // Decode the wuffs_base__frame_config.
  while (true) {
    wuffs_base__status status = wuffs_base__image_decoder__decode_frame_config(
        g_image_decoder, &g_frame_config, &g_src);

    if (status.repr == NULL) {
      break;
    } else if (status.repr != wuffs_base__suspension__short_read) {
      printf("%s: %s\n", g_filename, wuffs_base__status__message(&status));
      return false;
    }

    if (!read_more_src()) {
      return false;
    }
  }

  // From here on, this function always returns true. If we get this far, we
  // still display a partial image, even if we encounter an error.

  // Decode the frame (the pixels).
  while (true) {
    wuffs_base__status status = wuffs_base__image_decoder__decode_frame(
        g_image_decoder, &g_pixbuf, &g_src,
        wuffs_base__frame_config__overwrite_instead_of_blend(&g_frame_config)
            ? WUFFS_BASE__PIXEL_BLEND__SRC
            : WUFFS_BASE__PIXEL_BLEND__SRC_OVER,
        g_workbuf_slice, NULL);

    if (status.repr == NULL) {
      break;
    } else if (status.repr != wuffs_base__suspension__short_read) {
      printf("%s: %s\n", g_filename, wuffs_base__status__message(&status));
      return true;
    }

    if (!read_more_src()) {
      return true;
    }
  }

  uint32_t w = wuffs_base__pixel_config__width(&g_image_config.pixcfg);
  uint32_t h = wuffs_base__pixel_config__height(&g_image_config.pixcfg);
  printf("%s: ok (%" PRIu32 " x %" PRIu32 ")\n", g_filename, w, h);
  return true;
}

bool  //
load_image(const char* filename) {
  if (g_workbuf_slice.ptr != NULL) {
    free(g_workbuf_slice.ptr);
    g_workbuf_slice.ptr = NULL;
    g_workbuf_slice.len = 0;
  }
  if (g_pixbuf_slice.ptr != NULL) {
    free(g_pixbuf_slice.ptr);
    g_pixbuf_slice.ptr = NULL;
    g_pixbuf_slice.len = 0;
  }
  g_width = 0;
  g_height = 0;
  g_src.data.ptr = g_src_buffer_array;
  g_src.data.len = SRC_BUFFER_ARRAY_SIZE;
  g_src.meta.wi = 0;
  g_src.meta.ri = 0;
  g_src.meta.pos = 0;
  g_src.meta.closed = false;
  g_image_config = wuffs_base__null_image_config();
  g_image_decoder = NULL;

  g_file = stdin;
  g_filename = "<stdin>";
  if (filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
      printf("%s: could not open file\n", filename);
      return false;
    }
    g_file = f;
    g_filename = filename;
  }

  bool ret = load_image_type() && load_image_config() && load_image_frame();
  if (filename) {
    fclose(g_file);
    g_file = NULL;
  }
  return ret;
}

// ---------------------------------------------------------------------

#if defined(__linux__)
#define SUPPORTED_OPERATING_SYSTEM

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

#define XK_BackSpace 0xFF08
#define XK_Escape 0xFF1B
#define XK_Return 0xFF0D

xcb_atom_t g_atom_net_wm_name = XCB_NONE;
xcb_atom_t g_atom_utf8_string = XCB_NONE;
xcb_atom_t g_atom_wm_protocols = XCB_NONE;
xcb_atom_t g_atom_wm_delete_window = XCB_NONE;
xcb_pixmap_t g_pixmap = XCB_NONE;
xcb_keysym_t* g_keysyms = NULL;
xcb_get_keyboard_mapping_reply_t* g_keyboard_mapping = NULL;

void  //
init_keymap(xcb_connection_t* c, const xcb_setup_t* z) {
  xcb_get_keyboard_mapping_cookie_t cookie = xcb_get_keyboard_mapping(
      c, z->min_keycode, z->max_keycode - z->min_keycode + 1);
  g_keyboard_mapping = xcb_get_keyboard_mapping_reply(c, cookie, NULL);
  g_keysyms = (xcb_keysym_t*)(g_keyboard_mapping + 1);
}

xcb_window_t  //
make_window(xcb_connection_t* c, xcb_screen_t* s) {
  xcb_window_t w = xcb_generate_id(c);
  uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t value_list[2];
  value_list[0] = s->black_pixel;
  value_list[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;
  xcb_create_window(c, 0, w, s->root, 0, 0, 1024, 768, 0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT, s->root_visual, value_mask,
                    value_list);
  xcb_change_property(c, XCB_PROP_MODE_REPLACE, w, g_atom_net_wm_name,
                      g_atom_utf8_string, 8, 12, "Image Viewer");
  xcb_change_property(c, XCB_PROP_MODE_REPLACE, w, g_atom_wm_protocols,
                      XCB_ATOM_ATOM, 32, 1, &g_atom_wm_delete_window);
  xcb_map_window(c, w);
  return w;
}

bool  //
load(xcb_connection_t* c,
     xcb_screen_t* s,
     xcb_window_t w,
     xcb_gcontext_t g,
     const char* filename) {
  if (g_pixmap != XCB_NONE) {
    xcb_free_pixmap(c, g_pixmap);
  }

  if (!load_image(filename)) {
    return false;
  }

  xcb_create_pixmap(c, s->root_depth, g_pixmap, w, g_width, g_height);
  xcb_image_t* image = xcb_image_create_native(
      c, g_width, g_height, XCB_IMAGE_FORMAT_Z_PIXMAP, s->root_depth, NULL,
      g_pixbuf_slice.len, g_pixbuf_slice.ptr);
  xcb_image_put(c, g_pixmap, g, image, 0, 0, 0);
  xcb_image_destroy(image);
  return true;
}

int  //
main(int argc, char** argv) {
  xcb_connection_t* c = xcb_connect(NULL, NULL);
  const xcb_setup_t* z = xcb_get_setup(c);
  xcb_screen_t* s = xcb_setup_roots_iterator(z).data;

  {
    xcb_intern_atom_cookie_t cookie0 =
        xcb_intern_atom(c, 1, 12, "_NET_WM_NAME");
    xcb_intern_atom_cookie_t cookie1 = xcb_intern_atom(c, 1, 11, "UTF8_STRING");
    xcb_intern_atom_cookie_t cookie2 =
        xcb_intern_atom(c, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_cookie_t cookie3 =
        xcb_intern_atom(c, 1, 16, "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t* reply0 = xcb_intern_atom_reply(c, cookie0, NULL);
    xcb_intern_atom_reply_t* reply1 = xcb_intern_atom_reply(c, cookie1, NULL);
    xcb_intern_atom_reply_t* reply2 = xcb_intern_atom_reply(c, cookie2, NULL);
    xcb_intern_atom_reply_t* reply3 = xcb_intern_atom_reply(c, cookie3, NULL);
    g_atom_net_wm_name = reply0->atom;
    g_atom_utf8_string = reply1->atom;
    g_atom_wm_protocols = reply2->atom;
    g_atom_wm_delete_window = reply3->atom;
    free(reply0);
    free(reply1);
    free(reply2);
    free(reply3);
  }

  xcb_window_t w = make_window(c, s);
  xcb_gcontext_t g = xcb_generate_id(c);
  xcb_create_gc(c, g, w, 0, NULL);
  init_keymap(c, z);
  xcb_flush(c);
  g_pixmap = xcb_generate_id(c);

  bool loaded = load(c, s, w, g, (argc > 1) ? argv[1] : NULL);
  int arg = 1;

  while (true) {
    xcb_generic_event_t* event = xcb_wait_for_event(c);
    bool reload = false;

    switch (event->response_type & 0x7F) {
      case XCB_EXPOSE: {
        xcb_expose_event_t* e = (xcb_expose_event_t*)event;
        if (loaded && (e->count == 0)) {
          xcb_copy_area(c, g_pixmap, w, g, 0, 0, 0, 0, g_width, g_height);
          xcb_flush(c);
        }
        break;
      }

      case XCB_KEY_PRESS: {
        xcb_key_press_event_t* e = (xcb_key_press_event_t*)event;
        uint32_t i = e->detail;
        if ((z->min_keycode <= i) && (i <= z->max_keycode)) {
          i = g_keysyms[(i - z->min_keycode) *
                        g_keyboard_mapping->keysyms_per_keycode];
          switch (i) {
            case XK_Escape:
              return 0;

            case ' ':
            case XK_BackSpace:
            case XK_Return:
              if (argc <= 2) {
                break;
              }
              arg += (i != XK_BackSpace) ? +1 : -1;
              if (arg == 0) {
                arg = argc - 1;
              } else if (arg == argc) {
                arg = 1;
              }
              reload = true;
              break;

            case ',':
            case '.':
              g_background_color_index +=
                  (i == ',') ? (NUM_BACKGROUND_COLORS - 1) : 1;
              g_background_color_index %= NUM_BACKGROUND_COLORS;
              reload = true;
              break;
          }
        }
        break;
      }

      case XCB_CLIENT_MESSAGE: {
        xcb_client_message_event_t* e = (xcb_client_message_event_t*)event;
        if (e->data.data32[0] == g_atom_wm_delete_window) {
          return 0;
        }
        break;
      }
    }

    free(event);

    if (reload) {
      loaded = load(c, s, w, g, argv[arg]);
      xcb_clear_area(c, 1, w, 0, 0, 0xFFFF, 0xFFFF);
      xcb_flush(c);
    }
  }
  return 0;
}

#endif  // defined(__linux__)

// ---------------------------------------------------------------------

#if !defined(SUPPORTED_OPERATING_SYSTEM)

int  //
main(int argc, char** argv) {
  printf("unsupported operating system\n");
  return 1;
}

#endif  // !defined(SUPPORTED_OPERATING_SYSTEM)
