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

$CXX imageviewer.cc -lxcb -lxcb-image && \
  ./a.out ../../test/data/bricks-*.gif; rm -f a.out

for a C++ compiler $CXX, such as clang++ or g++.

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
// release/c/etc.c choose which parts of Wuffs to build. That file contains the
// entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULES
#define WUFFS_CONFIG__MODULE__ADLER32
#define WUFFS_CONFIG__MODULE__AUX__BASE
#define WUFFS_CONFIG__MODULE__AUX__IMAGE
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__BMP
#define WUFFS_CONFIG__MODULE__CRC32
#define WUFFS_CONFIG__MODULE__DEFLATE
#define WUFFS_CONFIG__MODULE__GIF
#define WUFFS_CONFIG__MODULE__LZW
#define WUFFS_CONFIG__MODULE__NIE
#define WUFFS_CONFIG__MODULE__PNG
#define WUFFS_CONFIG__MODULE__WBMP
#define WUFFS_CONFIG__MODULE__ZLIB

// If building this program in an environment that doesn't easily accommodate
// relative includes, you can use the script/inline-c-relative-includes.go
// program to generate a stand-alone C file.
#include "../../release/c/wuffs-unsupported-snapshot.c"

// X11 limits its image dimensions to uint16_t.
#define MAX_INCL_DIMENSION 65535

#define NUM_BACKGROUND_COLORS 3
#define SRC_BUFFER_ARRAY_SIZE (64 * 1024)

wuffs_base__color_u32_argb_premul g_background_colors[NUM_BACKGROUND_COLORS] = {
    0xFF000000,
    0xFFFFFFFF,
    0xFFA9009A,
};

uint32_t g_width = 0;
uint32_t g_height = 0;
wuffs_aux::MemOwner g_pixbuf_mem_owner(nullptr, &free);
wuffs_base__slice_u8 g_pixbuf_mem_slice = {0};
uint32_t g_background_color_index = 0;

bool  //
load_image(const char* filename) {
  FILE* file = stdin;
  const char* adj_filename = "<stdin>";
  if (filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
      printf("%s: could not open file\n", filename);
      return false;
    }
    file = f;
    adj_filename = filename;
  }

  g_width = 0;
  g_height = 0;
  g_pixbuf_mem_owner.reset();
  g_pixbuf_mem_slice = wuffs_base__empty_slice_u8();

  wuffs_aux::DecodeImageCallbacks callbacks;
  wuffs_aux::sync_io::FileInput input(file);
  wuffs_aux::DecodeImageResult res = wuffs_aux::DecodeImage(
      callbacks, input,
      // Use PIXEL_BLEND__SRC_OVER, not the default PIXEL_BLEND__SRC, because
      // we also pass a background color.
      WUFFS_BASE__PIXEL_BLEND__SRC_OVER,
      g_background_colors[g_background_color_index], MAX_INCL_DIMENSION);
  if (filename) {
    fclose(file);
  }

  g_width = res.pixbuf.pixcfg.width();
  g_height = res.pixbuf.pixcfg.height();
  g_pixbuf_mem_owner = std::move(res.pixbuf_mem_owner);
  g_pixbuf_mem_slice = res.pixbuf_mem_slice;

  if (res.error_message.empty()) {
    printf("%s: ok (%" PRIu32 " x %" PRIu32 ")\n", adj_filename, g_width,
           g_height);
  } else {
    printf("%s: %s\n", adj_filename, res.error_message.c_str());
  }
  return res.pixbuf.pixcfg.is_valid();
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
      g_pixbuf_mem_slice.len, g_pixbuf_mem_slice.ptr);
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
