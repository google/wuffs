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

$CXX imageviewer.cc -lxcb -lxcb-image -lxcb-render -lxcb-render-util && \
  ./a.out ../../test/data/bricks-*.gif; rm -f a.out

for a C++ compiler $CXX, such as clang++ or g++.

The Space and BackSpace keys cycle through the files, if more than one was
given as command line arguments. If none were given, the program reads from
stdin.

The Return key is equivalent to the Space key.

The ',' Comma and '.' Period keys cycle through background colors, which
matters if the image has fully or partially transparent pixels.

The '1' to '8' keys change the magnification zoom (or minification zoom with
the shift key). The '0' key toggles nearest neighbor and bilinear filtering.

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

// Defining the WUFFS_CONFIG__STATIC_FUNCTIONS macro is optional, but when
// combined with WUFFS_IMPLEMENTATION, it demonstrates making all of Wuffs'
// functions have static storage.
//
// This can help the compiler ignore or discard unused code, which can produce
// faster compiles and smaller binaries. Other motivations are discussed in the
// "ALLOW STATIC IMPLEMENTATION" section of
// https://raw.githubusercontent.com/nothings/stb/master/docs/stb_howto.txt
#define WUFFS_CONFIG__STATIC_FUNCTIONS

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

// X11 limits its image dimensions to uint16_t and some coordinates to int16_t.
#define MAX_INCL_DIMENSION 32767

#define NUM_BACKGROUND_COLORS 3
#define NUM_ZOOMS 8
#define SRC_BUFFER_ARRAY_SIZE (64 * 1024)

wuffs_base__color_u32_argb_premul g_background_colors[NUM_BACKGROUND_COLORS] = {
    0xFF000000,
    0xFFFFFFFF,
    0xFFA9009A,
};

uint32_t g_width = 0;
uint32_t g_height = 0;
wuffs_aux::MemOwner g_pixbuf_mem_owner(nullptr, &free);
wuffs_base__pixel_buffer g_pixbuf = {0};
uint32_t g_background_color_index = 0;
int32_t g_zoom = 0;
bool g_filter = false;

bool  //
load_image(const char* filename) {
  FILE* file = stdin;
  const char* adj_filename = "<stdin>";
  if (filename) {
    FILE* f = fopen(filename, "rb");
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
  g_pixbuf = wuffs_base__null_pixel_buffer();

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

  // wuffs_aux::DecodeImageCallbacks's default implementation should give us an
  // interleaved (not multi-planar) pixel buffer, so that all of the pixel data
  // is in a single 2-dimensional table (plane 0). Later on, we re-interpret
  // that table as XCB image data, which isn't something we could do if we had
  // e.g. multi-planar YCbCr.
  if (!res.pixbuf.pixcfg.pixel_format().is_interleaved()) {
    printf("%s: non-interleaved pixbuf\n", adj_filename);
    return false;
  }
  wuffs_base__table_u8 tab = res.pixbuf.plane(0);
  if (tab.width != tab.stride) {
    // The xcb_image_create_native call, later on, assumes that (tab.height *
    // tab.stride) bytes are readable, which isn't quite the same as what
    // wuffs_base__table__flattened_length(tab.width, tab.height, tab.stride)
    // returns unless the table is tight (its width equals its stride).
    printf("%s: could not allocate tight pixbuf\n", adj_filename);
    return false;
  }

  g_width = res.pixbuf.pixcfg.width();
  g_height = res.pixbuf.pixcfg.height();
  g_pixbuf_mem_owner = std::move(res.pixbuf_mem_owner);
  g_pixbuf = res.pixbuf;

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

#include <xcb/render.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_renderutil.h>

#define XK_BackSpace 0xFF08
#define XK_Escape 0xFF1B
#define XK_Return 0xFF0D

uint32_t g_maximum_request_length = 0;  // Measured in 4-byte units.
xcb_atom_t g_atom_net_wm_name = XCB_NONE;
xcb_atom_t g_atom_utf8_string = XCB_NONE;
xcb_atom_t g_atom_wm_protocols = XCB_NONE;
xcb_atom_t g_atom_wm_delete_window = XCB_NONE;
xcb_pixmap_t g_pixmap = XCB_NONE;
xcb_gcontext_t g_pixmap_gc = XCB_NONE;
xcb_render_picture_t g_pixmap_picture = XCB_NONE;
xcb_render_pictforminfo_t* g_pictforminfo = NULL;
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

void  //
apply_zoom_and_filter(xcb_connection_t* c) {
  static const xcb_render_fixed_t neg_zooms[NUM_ZOOMS] = {
      0x00010000,  // 1/1 as 16.16 fixed point
      0x00020000,  // 2/1
      0x00040000,  // 4/1
      0x00080000,  // 8/1
      0x00100000,  // 16/1
      0x00200000,  // 32/1
      0x00400000,  // 64/1
      0x00800000,  // 128/1
  };
  static const xcb_render_fixed_t pos_zooms[NUM_ZOOMS] = {
      0x00010000,  // 1/1 as 16.16 fixed point
      0x00008000,  // 1/2
      0x00004000,  // 1/4
      0x00002000,  // 1/8
      0x00001000,  // 1/16
      0x00000800,  // 1/32
      0x00000400,  // 1/64
      0x00000200,  // 1/128
  };

  xcb_render_fixed_t z = g_zoom < 0
                             ? neg_zooms[((uint32_t)(-g_zoom)) % NUM_ZOOMS]
                             : pos_zooms[((uint32_t)(+g_zoom)) % NUM_ZOOMS];
  xcb_render_set_picture_transform(c, g_pixmap_picture,
                                   ((xcb_render_transform_t){
                                       z, 0, 0,        //
                                       0, z, 0,        //
                                       0, 0, 0x10000,  //
                                   }));

  uint16_t f_len = 7;
  const char* f_ptr = "nearest";
  if (g_filter && (g_zoom != 0)) {
    f_len = 8;
    f_ptr = "bilinear";
  }
  xcb_render_set_picture_filter(c, g_pixmap_picture, f_len, f_ptr, 0, NULL);
}

// zoom_shift returns (a << g_zoom), roughly speaking, but saturates at an
// arbitrary value called M.
//
// The final two arguments to xcb_render_composite have uint16_t type (and
// UINT16_MAX is 65535), but in practice, values above M sometimes don't work
// in that the xcb_render_composite call has no visible effect.
//
// Some xrender debugging could potentially derive a more accurate maximum but
// for now, the M = 30000 round number will do.
uint16_t  //
zoom_shift(uint32_t a) {
  uint16_t M = 30000;
  uint64_t b = g_zoom < 0
                   ? (((uint64_t)a) >> (((uint32_t)(-g_zoom)) % NUM_ZOOMS))
                   : (((uint64_t)a) << (((uint32_t)(+g_zoom)) % NUM_ZOOMS));
  return (b < M) ? b : M;
}

bool  //
load(xcb_connection_t* c, xcb_window_t w, const char* filename) {
  if (g_pixmap != XCB_NONE) {
    xcb_render_free_picture(c, g_pixmap_picture);
    xcb_free_gc(c, g_pixmap_gc);
    xcb_free_pixmap(c, g_pixmap);
  }

  if (!load_image(filename)) {
    return false;
  }
  wuffs_base__table_u8 tab = g_pixbuf.plane(0);

  xcb_create_pixmap(c, g_pictforminfo->depth, g_pixmap, w, g_width, g_height);
  xcb_create_gc(c, g_pixmap_gc, g_pixmap, 0, NULL);
  xcb_render_create_picture(c, g_pixmap_picture, g_pixmap, g_pictforminfo->id,
                            0, NULL);
  apply_zoom_and_filter(c);

  // Copy the pixels from the X11 client process (this process) to the X11
  // server process. For large images, this may involve multiple xcb_image_put
  // calls, each copying part of the pixels (a strip that has the same width
  // but smaller height), to avoid XCB_CONN_CLOSED_REQ_LEN_EXCEED.
  if (g_width > 0) {
    uint32_t max_strip_height = g_maximum_request_length / g_width;
    for (uint32_t y = 0; y < g_height;) {
      uint32_t h = g_height - y;
      if (h > max_strip_height) {
        h = max_strip_height;
      }

      // Make libxcb-image interpret WUFFS_BASE__PIXEL_FORMAT__BGRA_PREMUL as
      // XCB_PICT_STANDARD_ARGB_32 with byte_order XCB_IMAGE_ORDER_LSB_FIRST.
      xcb_image_t* unconverted =
          xcb_image_create(g_width,                      // width
                           h,                            // height
                           XCB_IMAGE_FORMAT_Z_PIXMAP,    // format
                           32,                           // xpad
                           g_pictforminfo->depth,        // depth
                           32,                           // bpp
                           32,                           // unit
                           XCB_IMAGE_ORDER_LSB_FIRST,    // byte_order
                           XCB_IMAGE_ORDER_MSB_FIRST,    // bit_order
                           NULL,                         // base
                           h * tab.stride,               // bytes
                           tab.ptr + (y * tab.stride));  // data

      xcb_image_t* converted =
          xcb_image_native(c, unconverted, true);  // true means to convert.
      if (converted != unconverted) {
        xcb_image_destroy(unconverted);
      }
      xcb_image_put(c, g_pixmap, g_pixmap_gc, converted, 0, y, 0);
      xcb_image_destroy(converted);

      y += h;
    }
  }

  return true;
}

int  //
main(int argc, char** argv) {
  xcb_connection_t* c = xcb_connect(NULL, NULL);

  g_maximum_request_length = xcb_get_maximum_request_length(c);
  // Our X11 requests (especially xcb_image_put) also need a header, in terms
  // of wire format. 256 4-byte units should be big enough.
  const uint32_t max_req_len_adjustment = 256;
  if (g_maximum_request_length < max_req_len_adjustment) {
    printf("XCB failure (maximum request length is too short)\n");
    exit(EXIT_FAILURE);
  }
  g_maximum_request_length -= max_req_len_adjustment;

  const xcb_setup_t* z = xcb_get_setup(c);
  xcb_screen_t* s = xcb_setup_roots_iterator(z).data;

  const xcb_render_query_pict_formats_reply_t* pict_formats =
      xcb_render_util_query_formats(c);
  g_pictforminfo = xcb_render_util_find_standard_format(
      pict_formats, XCB_PICT_STANDARD_ARGB_32);

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
  xcb_render_picture_t p = xcb_generate_id(c);
  xcb_render_create_picture(
      c, p, w,
      xcb_render_util_find_visual_format(pict_formats, s->root_visual)->format,
      0, NULL);
  init_keymap(c, z);
  xcb_flush(c);

  g_pixmap = xcb_generate_id(c);
  g_pixmap_gc = xcb_generate_id(c);
  g_pixmap_picture = xcb_generate_id(c);

  bool loaded = load(c, w, (argc > 1) ? argv[1] : NULL);
  int arg = 1;

  while (true) {
    xcb_generic_event_t* event = xcb_wait_for_event(c);
    if (!event) {
      printf("XCB failure (error code %d)\n", xcb_connection_has_error(c));
      exit(EXIT_FAILURE);
    }

    bool reload = false;
    switch (event->response_type & 0x7F) {
      case XCB_EXPOSE: {
        xcb_expose_event_t* e = (xcb_expose_event_t*)event;
        if (loaded && (e->count == 0)) {
          xcb_render_composite(c, XCB_RENDER_PICT_OP_SRC, g_pixmap_picture,
                               XCB_NONE, p, 0, 0, 0, 0, 0, 0,
                               zoom_shift(g_width), zoom_shift(g_height));
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

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
              if (i == '0') {
                g_filter = !g_filter;
              } else {
                int32_t z = i - '1';
                if (e->state & XCB_MOD_MASK_SHIFT) {
                  z = -z;
                }
                if (g_zoom == z) {
                  break;
                }
                g_zoom = z;
              }
              apply_zoom_and_filter(c);
              xcb_clear_area(c, 1, w, 0, 0, 0xFFFF, 0xFFFF);
              xcb_flush(c);
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
      loaded = load(c, w, argv[arg]);
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
