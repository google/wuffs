#include <stdlib.h>
#include <inttypes.h>

uint32_t parse(char* p, size_t n) {
  uint32_t ret = 0;
  for (size_t i = 0; (i < n) && p[i]; i++) {
    ret = (10 * ret) + (p[i] - '0');
  }
  return ret;
}
