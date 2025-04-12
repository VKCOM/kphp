// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef __NET_CRYPTO_AES256_H
#define __NET_CRYPTO_AES256_H

#include <stdint.h>
#include <sys/cdefs.h>

#include "openssl/aes.h"

#define AES256_KEY_BITS 256

struct aes256_ctx {
  uint8_t a[AES256_KEY_BITS + 16];
};
typedef struct aes256_ctx aes256_ctx_t;

static inline void* align16(void* ptr) {
  return (void*)(((uintptr_t)ptr + 15) & -16L);
}

typedef struct vk_aes_ctx {
  void (*cbc_crypt)(struct vk_aes_ctx* ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[16]);
  void (*ige_crypt)(struct vk_aes_ctx* ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[32]);
  void (*ctr_crypt)(struct vk_aes_ctx* ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[16], uint64_t offset);
  union {
    AES_KEY key;
    aes256_ctx_t ctx;
  } u;
} vk_aes_ctx_t;

void vk_aes_set_encrypt_key(vk_aes_ctx_t* ctx, const uint8_t* key, int bits);
void vk_aes_set_decrypt_key(vk_aes_ctx_t* ctx, const uint8_t* key, int bits);
void vk_aes_ctx_cleanup(vk_aes_ctx_t* ctx);
void vk_aes_ctx_copy(vk_aes_ctx_t* to, const vk_aes_ctx_t* from);

/******************** Test only declarations  ********************/
void vk_ssl_aes_ctr_crypt(vk_aes_ctx_t* ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[16], uint64_t offset);

#endif
