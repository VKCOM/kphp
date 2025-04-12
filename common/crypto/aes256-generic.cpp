// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <string.h>

#include "openssl/aes.h"

#include "common/crypto/aes256-generic.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

void crypto_generic_aes256_set_encrypt_key(vk_aes_ctx_t* vk_ctx, const uint8_t key[32]) {
  AES_set_encrypt_key(key, AES256_KEY_BITS, &vk_ctx->u.key);
}

void crypto_generic_aes256_set_decrypt_key(vk_aes_ctx_t* vk_ctx, const uint8_t key[32]) {
  AES_set_decrypt_key(key, AES256_KEY_BITS, &vk_ctx->u.key);
}

void crypto_generic_aes256_ige_encrypt(vk_aes_ctx_t* vk_ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[32]) {
  AES_ige_encrypt(in, out, size, &vk_ctx->u.key, iv, AES_ENCRYPT);
}

void crypto_generic_aes256_ige_decrypt(vk_aes_ctx_t* vk_ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[32]) {
  AES_ige_encrypt(in, out, size, &vk_ctx->u.key, iv, AES_DECRYPT);
}

void crypto_generic_aes256_cbc_encrypt(vk_aes_ctx_t* vk_ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[16]) {
  AES_cbc_encrypt(in, out, size, &vk_ctx->u.key, iv, AES_ENCRYPT);
}

void crypto_generic_aes256_cbc_decrypt(vk_aes_ctx_t* vk_ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[16]) {
  AES_cbc_encrypt(in, out, size, &vk_ctx->u.key, iv, AES_DECRYPT);
}

void crypto_generic_aes256_ctr_encrypt(vk_aes_ctx_t* vk_ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[16], uint64_t offset) {
  uint8_t iv_copy[16];
  memcpy(iv_copy, iv, 16);
  uint64_t* p = (uint64_t*)(iv_copy + 8);
  (*p) += offset >> 4;
  union {
    uint8_t c[16];
    uint64_t d[2];
  } u;
  int i = offset & 15, l;
  if (i) {
    AES_encrypt(iv_copy, u.c, &vk_ctx->u.key);
    (*p)++;
    l = i + size;
    if (l > 16) {
      l = 16;
    }
    size -= l - i;
    do {
      *out++ = (*in++) ^ u.c[i++];
    } while (i < l);
  }
  const uint64_t* I = (const uint64_t*)in;
  uint64_t* O = (uint64_t*)out;
  int n = size >> 4;
  while (--n >= 0) {
    AES_encrypt(iv_copy, (uint8_t*)u.d, &vk_ctx->u.key);
    (*p)++;
    *O++ = (*I++) ^ u.d[0];
    *O++ = (*I++) ^ u.d[1];
  }
  in = (const uint8_t*)I;
  out = (uint8_t*)O;
  l = size & 15;
  if (l) {
    AES_encrypt(iv_copy, u.c, &vk_ctx->u.key);
    i = 0;
    do {
      *out++ = (*in++) ^ u.c[i++];
    } while (i < l);
  }
}

#pragma GCC diagnostic pop
