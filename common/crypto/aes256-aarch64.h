// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_COMMON_CRYPTO_AES256_AARCH64_H
#define KDB_COMMON_CRYPTO_AES256_AARCH64_H

#include <stdbool.h>
#include <sys/cdefs.h>

#include "common/crypto/aes256.h"

bool crypto_aarch64_has_aes_extension();

void crypto_aarch64_aes256_set_encrypt_key(vk_aes_ctx_t* vk_ctx, const uint8_t key[32]);
void crypto_aarch64_aes256_set_decrypt_key(vk_aes_ctx_t* vk_ctx, const uint8_t key[32]);

void crypto_aarch64_aes256_cbc_encrypt(vk_aes_ctx_t* vk_ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[16]);
void crypto_aarch64_aes256_cbc_decrypt(vk_aes_ctx_t* vk_ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[16]);
void crypto_aarch64_aes256_ige_encrypt(vk_aes_ctx_t* vk_ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[32]);
void crypto_aarch64_aes256_ige_decrypt(vk_aes_ctx_t* vk_ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[32]);
void crypto_aarch64_aes256_ctr_encrypt(vk_aes_ctx_t* vk_ctx, const uint8_t* in, uint8_t* out, int size, uint8_t iv[16], uint64_t offset);

#endif // KDB_COMMON_CRYPTO_AES256_AARCH64_H
