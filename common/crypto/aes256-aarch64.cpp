// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/crypto/aes256-aarch64.h"

#include <assert.h>
#include <string.h>
#include <sys/auxv.h>

#include "common/cpuid.h"

bool crypto_aarch64_has_aes_extension() {
  const kdb_cpuid_t *cpuid = kdb_cpuid();
  assert(cpuid->type == KDB_CPUID_AARCH64);

  static bool cached = false;
  static bool has_aes_extension = false;
  if (!cached) {
    const unsigned long hwcaps = getauxval(AT_HWCAP);
    has_aes_extension = hwcaps & HWCAP_AES;
    cached = true;
  }

  return has_aes_extension;
}

void crypto_aarch64_aes256_set_encrypt_key(vk_aes_ctx_t *vk_ctx, const uint8_t key[32]) {
  static const int32_t rcon[] = {0x01, 0x01, 0x01, 0x01, 0x0c0f0e0d, 0x0c0f0e0d, 0x0c0f0e0d, 0x0c0f0e0d, 0x1b, 0x1b, 0x1b, 0x1b};
  asm volatile("mov x9, %[rcon]  ;"
               "mov x10, %[ctx] ;"
               "mov x11, %[key] ;"

               "eor	v0.16b, v0.16b, v0.16b ;"    // zero v0.16b
               "ld1	{v3.16b}, [x11], #16	;"     // load second half of userKey into v3
               "mov	w1, #8 ;"                    // reuse w1
               "ld1	{v1.4s, v2.4s}, [x9], #32	;" // place in v1 and v2
               "ld1	{v4.16b}, [x11]	;"           // load first half of userKey into v4
               "mov	w1, #7 ;"                    // place 7 in w1
               "st1	{v3.4s}, [x10], #16 ;"       // place 4 words into key[16]

               "1: ;"
               "tbl	v6.16b, {v4.16b}, v2.16b ;"
               "ext	v5.16b,v0.16b,v3.16b,#12 ;"
               "st1	{v4.4s},[x10],#16 ;"
               "aese	v6.16b,v0.16b ;"
               "subs	w1,w1,#1 ;"

               "eor	v3.16b,v3.16b,v5.16b ;"
               "ext	v5.16b,v0.16b,v5.16b,#12 ;"
               "eor	v3.16b,v3.16b,v5.16b ;"
               "ext	v5.16b,v0.16b,v5.16b,#12 ;"
               "eor	v6.16b,v6.16b,v1.16b ;"
               "eor	v3.16b,v3.16b,v5.16b ;"
               "shl	v1.16b,v1.16b,#1 ;"
               "eor	v3.16b,v3.16b,v6.16b ;"
               "st1	{v3.4s},[x10],#16 ;"
               "b.eq 2f ;"

               "dup	v6.4s,v3.s[3]	;" // just splat
               "ext	v5.16b,v0.16b,v4.16b,#12 ;"
               "aese	v6.16b,v0.16b ;"

               "eor	v4.16b,v4.16b,v5.16b ;"
               "ext	v5.16b,v0.16b,v5.16b,#12 ;"
               "eor	v4.16b,v4.16b,v5.16b ;"
               "ext	v5.16b,v0.16b,v5.16b,#12 ;"
               "eor	v4.16b,v4.16b,v5.16b ;"

               "eor	v4.16b,v4.16b,v6.16b ;"
               "b	1b ;"

               "2: ;"
               :
               : [ctx] "r"(align16(&vk_ctx->u.ctx.a)), [key] "r"(key), [rcon] "r"(rcon));
}

void crypto_aarch64_aes256_set_decrypt_key(vk_aes_ctx_t *vk_ctx, const uint8_t key[32]) {
  crypto_aarch64_aes256_set_encrypt_key(vk_ctx, key);

  unsigned char *a = static_cast<unsigned char *>(align16(&vk_ctx->u.ctx.a));
  for (int i = 1; i <= 13; i++) {
    asm volatile("mov x9, %[key] ;"
                 "ld1 {v0.16b}, [x9] ;"
                 "aesimc v0.16b, v0.16b ;"
                 "st1 {v0.16b}, [x9] ;"
                 :
                 : [key] "r"(&a[i * 16])
                 : "memory");
  }
}

void crypto_aarch64_aes256_cbc_encrypt(vk_aes_ctx_t *vk_ctx, const uint8_t *in, uint8_t *out, int size, uint8_t iv[16]) {
  if (size < 16) {
    return;
  }

  asm volatile("mov x9, %[iv]  ;"       // move IV address in x9
               "mov x10, %[out] ;"      // move out address in x10
               "mov x11, %x[size] ;"    // move size value in x11
               "mov x12, %[in] ;"       // move plaintext address in x12
               "mov x13, %[key] ;"      // move key address in x13
               "ld1 {v25.16b}, [x9] ;"  // load IV to v0.16b
               "ld1 {v1.16b}, [x13], #16 ;"  // load key for round 1
               "ld1 {v2.16b}, [x13], #16 ;"  // load key for round 2
               "ld1 {v3.16b}, [x13], #16 ;"  // load key for round 3
               "ld1 {v4.16b}, [x13], #16 ;"  // load key for round 4
               "ld1 {v5.16b}, [x13], #16 ;"  // load key for round 5
               "ld1 {v6.16b}, [x13], #16 ;"  // load key for round 6
               "ld1 {v7.16b}, [x13], #16 ;"  // load key for round 7
               "ld1 {v16.16b}, [x13], #16 ;"  // load key for round 8
               "ld1 {v17.16b}, [x13], #16 ;"  // load key for round 9
               "ld1 {v18.16b}, [x13], #16 ;" // load key for round 10
               "ld1 {v19.16b}, [x13], #16 ;" // load key for round 11
               "ld1 {v20.16b}, [x13], #16 ;" // load key for round 12
               "ld1 {v21.16b}, [x13], #16 ;" // load key for round 13
               "ld1 {v22.16b}, [x13], #16 ;" // load key for round 14
               "ld1 {v23.16b}, [x13] ;" // load key for round 14

               "1: ;"                   // start block processing
               "sub x11, x11, 16 ;"     // substract block size from size
               "ld1 {v24.16b}, [x12], #16 ;" // load plaintext in v24.16b

               "eor v0.16b, v25.16b, v24.16b;" // CBC mode
               "aese v0.16b, v1.16b ;"         // Round 1
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v2.16b ;" // Round 2
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v3.16b ;" // Round 3
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v4.16b ;" // Round 4
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v5.16b ;" // Round 5
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v6.16b ;" // Round 6
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v7.16b ;" // Round 7
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v16.16b ;" // Round 8
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v17.16b ;" // Round 9
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v18.16b ;" // Round 10
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v19.16b ;" // Round 11
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v20.16b ;" // Round 12
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v21.16b ;" // Round 13
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v22.16b ;"      // Round 14
               "eor v0.16b, v0.16b, v23.16b;" // last XOR
               "mov v25.16b, v0.16b ;"
               "st1 {v0.16b}, [x10], #16 ;"
               "cmp x11, 16 ;"
               "bge 1b ;"
               "st1 {v1.16b}, [x9] ;"

               :
               : [key] "r"(align16(&vk_ctx->u.ctx.a)), [in] "r"(in), [size] "r"(size), [iv] "r"(iv), [out] "r"(out));
}

void crypto_aarch64_aes256_cbc_decrypt(vk_aes_ctx_t *vk_ctx, const uint8_t *in, uint8_t *out, int size, uint8_t iv[16]) {
  if (size < 16) {
    return;
  }

  asm volatile("mov x9, %[iv]  ;"       // move IV address in x9
               "mov x10, %[out] ;"      // move out address in x10
               "mov x11, %x[size] ;"    // move size value in x11
               "mov x12, %[in] ;"       // move ciphertext address in x12
               "mov x13, %[key] ;"      // move key address in x13
               "ld1 {v25.16b}, [x9] ;"  // load IV to v25.16b

               "ld1 {v23.16b}, [x13], #16 ;"
               "ld1 {v22.16b}, [x13], #16 ;"
               "ld1 {v21.16b}, [x13], #16 ;"
               "ld1 {v20.16b}, [x13], #16 ;"
               "ld1 {v19.16b}, [x13], #16 ;"
               "ld1 {v18.16b}, [x13], #16 ;"
               "ld1 {v17.16b}, [x13], #16 ;"
               "ld1 {v16.16b}, [x13], #16 ;"
               "ld1 {v7.16b}, [x13], #16 ;"
               "ld1 {v6.16b}, [x13], #16 ;"
               "ld1 {v5.16b}, [x13], #16 ;"
               "ld1 {v4.16b}, [x13], #16 ;"
               "ld1 {v3.16b}, [x13], #16 ;"
               "ld1 {v2.16b}, [x13], #16 ;"
               "ld1 {v1.16b}, [x13], #16 ;"

               "1: ;"                   // start block processing
               "sub x11, x11, 16 ;"     // substract block size from size
               "ld1 {v0.16b}, [x12] ;"  // load ciphertext in v0.16b

               "aesd v0.16b, v1.16b ;"  // Round 1
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v2.16b ;" // Round 2
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v3.16b ;" // Round 3
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v4.16b ;" // Round 4
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v5.16b ;" // Round 5
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v6.16b ;" // Round 6
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v7.16b ;" // Round 7
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v16.16b ;" // Round 8
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v17.16b ;" // Round 9
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v18.16b ;" // Round 10
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v19.16b ;" // Round 11
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v20.16b ;" // Round 12
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v21.16b ;" // Round 13
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v22.16b ;"        // Round 14
               "eor v0.16b, v0.16b, v23.16b; "  // last XOR
               "eor v0.16b, v0.16b, v25.16b; "  // CBC XOR
               "ld1 {v25.16b}, [x12], #16 ;"    // load cyphertext to v25.16b
               "st1 {v0.16b}, [x10], #16 ;"
               "cmp x11, 16 ;"
               "bge 1b ;"
               "st1 {v1.16b}, [x9] ;"

               :
               : [key] "r"(align16(&vk_ctx->u.ctx.a)), [in] "r"(in), [size] "r"(size), [iv] "r"(iv), [out] "r"(out));
}

void crypto_aarch64_aes256_ige_encrypt(vk_aes_ctx_t *vk_ctx, const uint8_t *in, uint8_t *out, int size, uint8_t iv[32]) {
  if (size < 16) {
    return;
  }

  asm volatile("mov x9, %[iv]  ;"             // move IGE IV address in x9
               "mov x10, %[out] ;"            // move out address in x10
               "mov x11, %x[size] ;"          // move size value in x11
               "mov x12, %[in] ;"             // move plaintext address in x12
               "mov x13, %[key] ;"            // move key address in x13
               "ld1 {v25.16b}, [x9], #16 ;"   // load IGE IV Y to v25.16b
               "ld1 {v26.16b}, [x9] ;"        // load IGE IV X to v26.16b

               "ld1 {v1.16b}, [x13], #16 ;"   // load key for round 1
               "ld1 {v2.16b}, [x13], #16 ;"   // load key for round 2
               "ld1 {v3.16b}, [x13], #16 ;"   // load key for round 3
               "ld1 {v4.16b}, [x13], #16 ;"   // load key for round 4
               "ld1 {v5.16b}, [x13], #16 ;"   // load key for round 5
               "ld1 {v6.16b}, [x13], #16 ;"   // load key for round 6
               "ld1 {v7.16b}, [x13], #16 ;"   // load key for round 7
               "ld1 {v16.16b}, [x13], #16 ;"   // load key for round 8
               "ld1 {v17.16b}, [x13], #16 ;"   // load key for round 9
               "ld1 {v18.16b}, [x13], #16 ;"  // load key for round 10
               "ld1 {v19.16b}, [x13], #16 ;"  // load key for round 11
               "ld1 {v20.16b}, [x13], #16 ;"  // load key for round 12
               "ld1 {v21.16b}, [x13], #16 ;"  // load key for round 13
               "ld1 {v22.16b}, [x13], #16 ;"  // load key for round 14
               "ld1 {v23.16b}, [x13] ;"       // load key for round 14

               "1: ;"                         // start block processing
               "sub x11, x11, 16 ;"           // substract block size from size
               "ld1 {v24.16b}, [x12], #16 ;"  // load plaintext in v24.16b

               "eor v0.16b, v24.16b, v25.16b ;" // IGE mode XOR Y
               "aese v0.16b, v1.16b ;"          // Round 1
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v2.16b ;" // Round 2
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v3.16b ;" // Round 3
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v4.16b ;" // Round 4
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v5.16b ;" // Round 5
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v6.16b ;" // Round 6
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v7.16b ;" // Round 7
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v16.16b ;" // Round 8
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v17.16b ;" // Round 9
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v18.16b ;" // Round 10
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v19.16b ;" // Round 11
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v20.16b ;" // Round 12
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v21.16b ;" // Round 13
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v22.16b ;"        // Round 14
               "eor v0.16b, v0.16b, v23.16b ;"  // last XOR
               "eor v25.16b, v0.16b, v26.16b ;" // IGE mode XOR X
               "mov v26.16b, v24.16b ;"
               "st1 {v25.16b}, [x10], #16 ;"
               "cmp x11, 16 ;"
               "bge 1b ;"
               "st1 {v26.16b}, [x9] ;"
               "sub x9, x9, 16 ;" // substract block size from size
               "st1 {v25.16b}, [x9] ;"

               :
               : [key] "r"(align16(&vk_ctx->u.ctx.a)), [in] "r"(in), [size] "r"(size), [iv] "r"(iv), [out] "r"(out));
}

void crypto_aarch64_aes256_ige_decrypt(vk_aes_ctx_t *vk_ctx, const uint8_t *in, uint8_t *out, int size, uint8_t iv[32]) {
  if (size < 16) {
    return;
  }

  asm volatile("mov x9, %[iv]  ;"           // move IGE IV address in x9
               "mov x10, %[out] ;"          // move out address in x10
               "mov x11, %x[size] ;"        // move size value in x11
               "mov x12, %[in] ;"           // move cyphertext address in x12
               "mov x13, %[key] ;"          // move key address in x13
               "ld1 {v25.16b}, [x9], #16 ;" // load IGE IV Y to v25.16b
               "ld1 {v26.16b}, [x9] ;"      // load IGE IV X to v26.16b

               "ld1 {v23.16b}, [x13], #16 ;"
               "ld1 {v22.16b}, [x13], #16 ;"
               "ld1 {v21.16b}, [x13], #16 ;"
               "ld1 {v20.16b}, [x13], #16 ;"
               "ld1 {v19.16b}, [x13], #16 ;"
               "ld1 {v18.16b}, [x13], #16 ;"
               "ld1 {v17.16b}, [x13], #16 ;"
               "ld1 {v16.16b}, [x13], #16 ;"
               "ld1 {v7.16b}, [x13], #16 ;"
               "ld1 {v6.16b}, [x13], #16 ;"
               "ld1 {v5.16b}, [x13], #16 ;"
               "ld1 {v4.16b}, [x13], #16 ;"
               "ld1 {v3.16b}, [x13], #16 ;"
               "ld1 {v2.16b}, [x13], #16 ;"
               "ld1 {v1.16b}, [x13], #16 ;"

               "1: ;"                           // start block processing
               "sub x11, x11, 16 ;"             // substract block size from size
               "ld1 {v24.16b}, [x12], #16 ;"    // load cyphertext in v24.16b
               "eor v0.16b, v24.16b, v26.16b ;" // IGE mode XOR Y
               "aesd v0.16b, v1.16b ;"          // Round 1
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v2.16b ;" // Round 2
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v3.16b ;" // Round 3
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v4.16b ;" // Round 4
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v5.16b ;" // Round 5
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v6.16b ;" // Round 6
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v7.16b ;" // Round 7
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v16.16b ;" // Round 8
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v17.16b ;" // Round 9
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v18.16b ;" // Round 10
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v19.16b ;" // Round 11
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v20.16b ;" // Round 12
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v21.16b ;" // Round 13
               "aesimc v0.16b, v0.16b ;"
               "aesd  v0.16b, v22.16b ;"        // Round 14
               "eor v0.16b, v0.16b, v23.16b ;"  // last XOR
               "eor v26.16b, v0.16b, v25.16b ;" // IGE mode XOR X
               "mov v25.16b, v24.16b ;"
               "st1 {v26.16b}, [x10], #16 ;"
               "cmp x11, 16 ;"
               "bge 1b ;"
               "st1 {v26.16b}, [x9] ;"
               "sub x9, x9, 16 ;" // substract block size from size
               "st1 {v25.16b}, [x9] ;"
               :
               : [key] "r"(align16(&vk_ctx->u.ctx.a)), [in] "r"(in), [size] "r"(size), [iv] "r"(iv), [out] "r"(out));
}

/* in, out should be aligned 16 */
static inline void crypto_aarch64_aes256_encrypt_single_block(vk_aes_ctx_t *vk_ctx, const uint8_t *in, uint8_t *out) {
  asm volatile("mov x9, %[out] ;"      // move out address in x9
               "mov x10, %[in] ;"       // move plaintext address in x10
               "mov x11, %[key] ;"      // move key address in x11

               "ld1 {v1.16b}, [x11], #16 ;"   // load key for round 1
               "ld1 {v2.16b}, [x11], #16 ;"   // load key for round 2
               "ld1 {v3.16b}, [x11], #16 ;"   // load key for round 3
               "ld1 {v4.16b}, [x11], #16 ;"   // load key for round 4
               "ld1 {v5.16b}, [x11], #16 ;"   // load key for round 5
               "ld1 {v6.16b}, [x11], #16 ;"   // load key for round 6
               "ld1 {v7.16b}, [x11], #16 ;"   // load key for round 7
               "ld1 {v16.16b}, [x11], #16 ;"   // load key for round 8
               "ld1 {v17.16b}, [x11], #16 ;"   // load key for round 9
               "ld1 {v18.16b}, [x11], #16 ;"  // load key for round 10
               "ld1 {v19.16b}, [x11], #16 ;"  // load key for round 11
               "ld1 {v20.16b}, [x11], #16 ;"  // load key for round 12
               "ld1 {v21.16b}, [x11], #16 ;"  // load key for round 13
               "ld1 {v22.16b}, [x11], #16 ;"  // load key for round 14
               "ld1 {v23.16b}, [x11] ;"       // load key for round 14

               "aese v0.16b, v1.16b ;"  // Round 1
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v2.16b ;" // Round 2
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v3.16b ;" // Round 3
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v4.16b ;" // Round 4
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v5.16b ;" // Round 5
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v6.16b ;" // Round 6
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v7.16b ;" // Round 7
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v16.16b ;" // Round 8
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v17.16b ;" // Round 9
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v18.16b ;" // Round 10
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v19.16b ;" // Round 11
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v20.16b ;" // Round 12
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v21.16b ;" // Round 13
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v22.16b ;"      // Round 14
               "eor v0.16b, v0.16b, v23.16b;" // last XOR
               "st1 {v0.16b}, [x9] ;"
               :
               : [key] "r"(align16(&vk_ctx->u.ctx.a)), [in] "r"(in), [out] "r"(out));
}

static inline void crypto_aarch64_aes256_encrypt_n_blocks(vk_aes_ctx_t *vk_ctx, const uint8_t *in, uint8_t *out, uint8_t iv[16], int n) {
  asm volatile("mov x9, %[out] ;"       // move out address in x9
               "mov x10, %x[in] ;"      // move plaintext address in x10
               "mov x11, %[key] ;"      // move key address in x11
               "mov x12, %[iv]  ;"      // move IV address in x12
               "mov x13, %[n] ;"        // move n value in x11
               "ld1 {v26.16b}, [x12] ;" // load IV to v0.16b
               "eor v25.16b, v25.16b, v25.16b ;"
               "mov w15, #1 ;"
               "mov v25.s[2], w15 ;"

               "ld1 {v1.16b}, [x11], #16 ;"   // load key for round 1
               "ld1 {v2.16b}, [x11], #16 ;"   // load key for round 2
               "ld1 {v3.16b}, [x11], #16 ;"   // load key for round 3
               "ld1 {v4.16b}, [x11], #16 ;"   // load key for round 4
               "ld1 {v5.16b}, [x11], #16 ;"   // load key for round 5
               "ld1 {v6.16b}, [x11], #16 ;"   // load key for round 6
               "ld1 {v7.16b}, [x11], #16 ;"   // load key for round 7
               "ld1 {v16.16b}, [x11], #16 ;"   // load key for round 8
               "ld1 {v17.16b}, [x11], #16 ;"   // load key for round 9
               "ld1 {v18.16b}, [x11], #16 ;"  // load key for round 10
               "ld1 {v19.16b}, [x11], #16 ;"  // load key for round 11
               "ld1 {v20.16b}, [x11], #16 ;"  // load key for round 12
               "ld1 {v21.16b}, [x11], #16 ;"  // load key for round 13
               "ld1 {v22.16b}, [x11], #16 ;"  // load key for round 14
               "ld1 {v23.16b}, [x11] ;"       // load key for round 14

               "1:" // block processing
               "ld1 {v24.16b}, [x10], #16 ;" // load plaintext in v24.16b
               "mov v0.16b, v26.16b ;"
               "aese v0.16b, v1.16b ;" // Round 1
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v2.16b ;" // Round 2
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v3.16b ;" // Round 3
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v4.16b ;" // Round 4
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v5.16b ;" // Round 5
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v6.16b ;" // Round 6
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v7.16b ;" // Round 7
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v16.16b ;" // Round 8
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v17.16b ;" // Round 9
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v18.16b ;" // Round 10
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v19.16b ;" // Round 11
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v20.16b ;" // Round 12
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v21.16b ;" // Round 13
               "aesmc v0.16b, v0.16b ;"
               "aese  v0.16b, v22.16b ;"       // Round 14
               "eor v0.16b, v0.16b, v23.16b ;" // last XOR
               "eor v0.16b, v0.16b, v24.16b ;" // CTR mode XOR
               "st1 {v0.16b}, [x9], #16 ;"
               "add v26.16b, v26.16b, v25.16b ;"
               "sub x13, x13, 1 ;"
               "cmp x13, 1 ;"
               "bge 1b ;"
               "st1 {v26.16b}, [x12];"
               :
               : [key] "r"(align16(&vk_ctx->u.ctx.a)), [in] "r"(in), [out] "r"(out), [iv] "r"(iv), [n] "r"(n));
  ;
}

void crypto_aarch64_aes256_ctr_encrypt(vk_aes_ctx_t *vk_ctx, const uint8_t *in, uint8_t *out, int size, uint8_t iv[16], uint64_t offset) {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
  unsigned char iv_copy[16], u[16];
  memcpy(iv_copy, iv, 16);
  unsigned long long *p = (unsigned long long *)(iv_copy + 8);
  (*p) += offset >> 4;
  int i = offset & 15, l;
  if (i) {
    crypto_aarch64_aes256_encrypt_single_block(vk_ctx, iv_copy, u);
    (*p)++;
    l = i + size;
    if (l > 16) {
      l = 16;
    }
    size -= l - i;
    do {
      *out++ = (*in++) ^ u[i++];
    } while (i < l);
  }
  int n = size >> 4;

  if (n > 0) {
    crypto_aarch64_aes256_encrypt_n_blocks(vk_ctx, in, out, iv, n);
  }

  l = size & 15;
  if (l) {
    crypto_aarch64_aes256_encrypt_single_block(vk_ctx, iv_copy, u);
    i = 0;
    do {
      *out++ = (*in++) ^ u[i++];
    } while (i < l);
  }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}
