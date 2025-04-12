// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/crypto/aes256.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "common/cpuid.h"
#include "common/crypto/aes256-aarch64.h"
#include "common/crypto/aes256-generic.h"
#include "common/crypto/aes256-x86_64.h"
#include "common/crypto/aes256-arm64.h"
#include "common/secure-bzero.h"

static bool use_hw_acceleration = true;

static void check_settings() __attribute__((constructor));

static void check_settings() {
  if (getenv("KDB_CRYPTO_DONT_USE_HW_ACCELERATION")) {
    use_hw_acceleration = false;
  }
}

void vk_aes_set_encrypt_key(vk_aes_ctx_t *ctx, const uint8_t *key, int bits) {
  assert(bits == AES256_KEY_BITS);

  if (use_hw_acceleration) {
#if defined(__x86_64__)
    if (crypto_x86_64_has_aesni_extension() && bits == AES256_KEY_BITS) {
      crypto_x86_64_aesni256_set_encrypt_key(ctx, key);
      ctx->cbc_crypt = crypto_x86_64_aesni256_cbc_encrypt;
      ctx->ige_crypt = crypto_x86_64_aesni256_ige_encrypt;
      ctx->ctr_crypt = crypto_x86_64_aesni256_ctr_encrypt;
      return;
    }
#elif defined(__arm64__)
    if (crypto_arm64_has_aes_extension()) {
      assert(0 && "unexpected");
    }
    // generic functions will be used on M1, see below
#elif defined(__aarch64__)
    if (crypto_aarch64_has_aes_extension()) {
      crypto_aarch64_aes256_set_encrypt_key(ctx, key);
      ctx->cbc_crypt = crypto_aarch64_aes256_cbc_encrypt;
      ctx->ige_crypt = crypto_aarch64_aes256_ige_encrypt;
      ctx->ctr_crypt = crypto_aarch64_aes256_ctr_encrypt;
      return;
    }
#endif 
  }

  crypto_generic_aes256_set_encrypt_key(ctx, key);
  ctx->cbc_crypt = crypto_generic_aes256_cbc_encrypt;
  ctx->ige_crypt = crypto_generic_aes256_ige_encrypt;
  ctx->ctr_crypt = crypto_generic_aes256_ctr_encrypt;
}

void vk_aes_set_decrypt_key(vk_aes_ctx_t *ctx, const uint8_t *key, int bits) {
  assert(bits == AES256_KEY_BITS);

  ctx->ctr_crypt = NULL; /* NOTICE: vk_aes_set_encrypt_key should be used for CTR decryption */

  if (use_hw_acceleration) {
#if defined(__x86_64__)
    if (crypto_x86_64_has_aesni_extension()) {
      crypto_x86_64_aesni256_set_decrypt_key(ctx, key);
      ctx->cbc_crypt = crypto_x86_64_aesni256_cbc_decrypt;
      ctx->ige_crypt = crypto_x86_64_aesni256_ige_decrypt;
      return;
    }
#elif defined(__arm64__)
    if (crypto_arm64_has_aes_extension()) {
      assert(0 && "unexpected");
    }
    // generic functions will be used on M1, see below
#elif defined(__aarch64__)
    if (crypto_aarch64_has_aes_extension()) {
      crypto_aarch64_aes256_set_decrypt_key(ctx, key);
      ctx->cbc_crypt = crypto_aarch64_aes256_cbc_decrypt;
      ctx->ige_crypt = crypto_aarch64_aes256_ige_decrypt;
      return;
    }
#endif 
  }

  crypto_generic_aes256_set_decrypt_key(ctx, key);
  ctx->cbc_crypt = crypto_generic_aes256_cbc_decrypt;
  ctx->ige_crypt = crypto_generic_aes256_ige_decrypt;
}

void vk_aes_ctx_cleanup(vk_aes_ctx_t *ctx) {
  secure_bzero(ctx, sizeof(*ctx));
}

void vk_aes_ctx_copy(vk_aes_ctx_t *to, const vk_aes_ctx_t *from) {
  to->cbc_crypt = from->cbc_crypt;
  to->ige_crypt = from->ige_crypt;
  to->ctr_crypt = from->ctr_crypt;

  if (from->cbc_crypt == crypto_generic_aes256_cbc_encrypt || from->cbc_crypt == crypto_generic_aes256_cbc_decrypt) {
    to->u.key = from->u.key;
  } else {
    memcpy(align16(&to->u.ctx.a[0]), align16((void *)&from->u.ctx.a[0]), AES256_KEY_BITS);
  }
}
