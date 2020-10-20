#include "common/sha1.h"

#include "common/secure-bzero.h"

void sha1(unsigned char *input, int ilen, unsigned char output[20]) {
  SHA1(input, ilen, output);
}

