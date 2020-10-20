#include "common/secure-bzero.h"

void secure_bzero(void *s, size_t n) {
  auto *p = reinterpret_cast<volatile unsigned char *>(s);

  while (n--) {
    *p++ = 0;
  }
}
