#include <stdio.h>

// TODO: this 'rough edge' shouldn't be necessary. See
// https://github.com/google/wuffs/issues/24
#define WUFFS_CONFIG__MODULE__BASE

#define WUFFS_IMPLEMENTATION
#include "./parse.c"

uint32_t parse(char* p, size_t n) {
  wuffs_base__status status;
  // If you want this on the heap instead, use
  // wuffs_demo__parser *parser = wuffs_demo__parser__alloc();
  // and don't run __initialize();
  wuffs_demo__parser parser;

  status = wuffs_demo__parser__initialize(&parser, sizeof__wuffs_demo__parser(),
                                          WUFFS_VERSION, 0);
  if (!wuffs_base__status__is_ok(&status)) {
    printf("initialize: %s\n", wuffs_base__status__message(&status));
    return 0;
  }

  wuffs_base__io_buffer iobuf;
  iobuf.data.ptr = (uint8_t*)p;
  iobuf.data.len = n;
  iobuf.meta.wi = n;
  iobuf.meta.ri = 0;
  iobuf.meta.pos = 0;
  iobuf.meta.closed = true;

  status = wuffs_demo__parser__parse(&parser, &iobuf);
  if (!wuffs_base__status__is_ok(&status)) {
    printf("parse: %s\n", wuffs_base__status__message(&status));
    return 0;
  }

  uint32_t ret = wuffs_demo__parser__value(&parser);
  return ret;
}
