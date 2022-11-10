// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/md5.h"

#include <cstdio>

#include "common/crypto/openssl-evp-digest.h"

void md5_hex(vk::span<const uint8_t> input, char output[32]) {
  uint8_t out[16];
  static char hcyf[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f'
  };
  static_assert(sizeof(out) == sizeof(hcyf), "hcyf must be 16 bytes long");

  vk::md5(input, out);
  for (int i = 0; i < 16; i++) {
    output[2 * i] = hcyf[(out[i] >> 4) & 15];
    output[2 * i + 1] = hcyf[out[i] & 15];
  }
}

/*
 * output = MD5( file contents )
 */
int md5_file(char *path, vk::span<uint8_t> output) {
  FILE *f;
  size_t n;
  vk::digest<EVP_md5> ctx;
  uint8_t buf[1024];

  if ((f = fopen(path, "rb")) == NULL) {
    return (1);
  }

  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
    ctx.hash_update({buf, n});
  }

  ctx.hash_final(output);

  if (ferror(f) != 0) {
    fclose(f);
    return (2);
  }

  fclose(f);
  return (0);
}
