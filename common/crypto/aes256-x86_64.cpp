// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/crypto/aes256-x86_64.h"

#include <assert.h>
#include <emmintrin.h>
#include <string.h>

#include "common/cpuid.h"
#include "common/simd-vector.h"

__attribute__((always_inline))
static inline v16qi loaddqu(const char *ptr) {
  return (v16qi)_mm_loadu_si128((const v2di *)ptr);
}

__attribute__((always_inline))
static inline void storedqu(char *out, v16qi v) {
  _mm_storeu_si128((v2di *)out, (v2di)v);
}

bool crypto_x86_64_has_aesni_extension() {
  const kdb_cpuid_t *cpuid = kdb_cpuid();
  assert(cpuid->type == KDB_CPUID_X86_64);

  return (cpuid->x86_64.ecx & (1 << 25)) && ((cpuid->x86_64.edx & 0x06000000) == 0x06000000);
}

void crypto_x86_64_aesni256_set_encrypt_key(vk_aes_ctx_t *ctx, const uint8_t key[32]) {
  int a, b;
  unsigned int *c, *d;
  asm volatile("movdqu (%4), %%xmm1\n\t"
               "movdqa %%xmm1, 0x0(%5)\n\t"
               "movdqu 0x10(%4), %%xmm3\n\t"
               "movdqa %%xmm3, 0x10(%5)\n\t"
               "addq $0x20, %5\n\t"
               "aeskeygenassist $0x01, %%xmm3, %%xmm2\n\t"
               "call 1f\n\t"
               "aeskeygenassist $0x02, %%xmm3, %%xmm2\n\t"
               "call 1f\n\t"
               "aeskeygenassist $0x04, %%xmm3, %%xmm2\n\t"
               "call 1f\n\t"
               "aeskeygenassist $0x08, %%xmm3, %%xmm2\n\t"
               "call 1f\n\t"
               "aeskeygenassist $0x10, %%xmm3, %%xmm2\n\t"
               "call 1f\n\t"
               "aeskeygenassist $0x20, %%xmm3, %%xmm2\n\t"
               "call 1f\n\t"
               "aeskeygenassist $0x40, %%xmm3, %%xmm2\n\t"
               "call 1f\n\t"
               "jmp 2f\n\t"
               "1:\n\t"
               "movq %5, %3\n\t"
               "pshufd $0xff, %%xmm2, %%xmm2\n\t"
               "pxor %%xmm1, %%xmm2\n\t"
               "movd %%xmm2, %0\n\t"
               "movl %0, (%5)\n\t"
               "addq $4, %5\n\t"
               "pshufd $0xe5, %%xmm1, %%xmm1\n\t"
               "movd %%xmm1, %1\n\t"
               "xor %1, %0\n\t"
               "movl %0, (%5)\n\t"
               "addq $4, %5\n\t"
               "pshufd $0xe6, %%xmm1, %%xmm1\n\t"
               "movd %%xmm1, %1\n\t"
               "xor %1, %0\n\t"
               "movl %0, (%5)\n\t"
               "addq $4, %5\n\t"
               "pshufd $0xe7, %%xmm1, %%xmm1\n\t"
               "movd %%xmm1, %1\n\t"
               "xor %1, %0\n\t"
               "movl %0, (%5)\n\t"
               "addq $4, %5\n\t"
               "movdqa (%3), %%xmm4\n\t"
               "aeskeygenassist $0, %%xmm4, %%xmm4\n\t"
               "pshufd $0xe6, %%xmm4, %%xmm4\n\t"
               "pxor %%xmm3, %%xmm4\n\t"
               "movd %%xmm4, %0\n\t"
               "movl %0, (%5)\n\t"
               "addq $4, %5\n\t"
               "pshufd $0xe5, %%xmm3, %%xmm3\n\t"
               "movd %%xmm3, %1\n\t"
               "xor %1, %0\n\t"
               "movl %0, (%5)\n\t"
               "addq $4, %5\n\t"
               "pshufd $0xe6, %%xmm3, %%xmm3\n\t"
               "movd %%xmm3, %1\n\t"
               "xor %1, %0\n\t"
               "movl %0, (%5)\n\t"
               "addq $4, %5\n\t"
               "pshufd $0xe7, %%xmm3, %%xmm3\n\t"
               "movd %%xmm3, %1\n\t"
               "xor %1, %0\n\t"
               "movl %0, (%5)\n\t"
               "addq $4, %5\n\t"
               "movdqa (%3), %%xmm1\n\t"
               "addq $0x10, %3\n\t"
               "movdqa (%3), %%xmm3\n\t"
               "ret\n\t"
               "2:\n\t"
               : "=&r"(a), "=&r"(b), "=&r"(c), "=&r"(d)
               : "r"(key), "2"(align16(&ctx->u.ctx.a[0]))
               : "%xmm1", "%xmm2", "%xmm3", "%xmm4", "memory");
}

void crypto_x86_64_aesni256_set_decrypt_key(vk_aes_ctx_t *ctx, const uint8_t key[32]) {
  int i;
  crypto_x86_64_aesni256_set_encrypt_key(ctx, key);
  auto *a = static_cast<unsigned char *>(align16(&ctx->u.ctx.a[0]));
  for (i = 1; i <= 13; i++) {
    asm volatile("aesimc (%0), %%xmm1\n\t"
                 "movdqa %%xmm1, (%0)\n\t"
                 :
                 : "r"(&a[i * 16])
                 : "%xmm1", "memory");
  }
}

void crypto_x86_64_aesni256_cbc_encrypt(vk_aes_ctx_t *vk_ctx, const uint8_t *in, uint8_t *out, int size, uint8_t iv[16]) {
  aes256_ctx_t *ctx = &vk_ctx->u.ctx;
  void *p1, *p2;
  if (size < 16) {
    return;
  }
  asm volatile("movdqu (%5), %%xmm1\n\t"          // move IV in %xmm1
               "1:\n\t"                           // start of block encryption
               "subl $0x10, %3\n\t"               // decrease plaintext size on block size
               "movdqu (%4), %%xmm2\n\t"          // move block of plaintext to %xmm2
               "addq $0x10, %4\n\t"               // advance plaintext pointer
               "pxor %%xmm1, %%xmm2\n\t"          // CBC xor
               "pxor (%6), %%xmm2\n\t"            // AddRoundKey
               "aesenc 0x10(%6), %%xmm2\n\t"      // 1 round
               "aesenc 0x20(%6), %%xmm2\n\t"      // 2 round
               "aesenc 0x30(%6), %%xmm2\n\t"      // 3 round
               "aesenc 0x40(%6), %%xmm2\n\t"      // ...
               "aesenc 0x50(%6), %%xmm2\n\t"      // ...
               "aesenc 0x60(%6), %%xmm2\n\t"      // ...
               "aesenc 0x70(%6), %%xmm2\n\t"      // ..
               "aesenc 0x80(%6), %%xmm2\n\t"      // ..
               "aesenc 0x90(%6), %%xmm2\n\t"      // ..
               "aesenc 0xa0(%6), %%xmm2\n\t"      // ..
               "aesenc 0xb0(%6), %%xmm2\n\t"      // ...
               "aesenc 0xc0(%6), %%xmm2\n\t"      // ...
               "aesenc 0xd0(%6), %%xmm2\n\t"      // ...
               "aesenclast 0xe0(%6), %%xmm2\n\t"  // 14 round is last for 256 bits key
               "movaps %%xmm2, %%xmm1\n\t"        // move cyphertext to %xmm1
               "movdqu %%xmm2, (%7)\n\t"          // move cyphertext to out
               "addq $0x10, %7\n\t"               // advance out pointer on block size
               "cmpl $0x0f, %3\n\t"               // do we have more blocks in plaintext?
               "jg 1b\n\t"                        // if yes - jump to start of block processing
               "movdqu %%xmm1, (%5)\n\t"          // move block of cyphertext to IV
               : "=r"(size), "=r"(p1), "=r"(p2)
               : "0"(size), "1r"(in), "r"(iv), "r"(align16(ctx)), "2r"(out)
               : "%xmm1", "%xmm2", "memory");
}

void crypto_x86_64_aesni256_cbc_decrypt(vk_aes_ctx_t *vk_ctx, const uint8_t *in, uint8_t *out, int size, uint8_t iv[16]) {
  aes256_ctx_t *ctx = &vk_ctx->u.ctx;
  void *p1, *p2;
  if (size < 16) {
    return;
  }
  asm volatile("movdqu (%5), %%xmm1\n\t"          // move IV in %xmm1
               "1:\n\t"                           // start of block processing
               "subl $0x10, %3\n\t"               // decrease plaintext size on block size
               "movdqu (%4), %%xmm2\n\t"          // move block of cyphertext to %xmm2
               "pxor 0xe0(%6), %%xmm2\n\t"        // AddRoundKey
               "aesdec 0xd0(%6), %%xmm2\n\t"      // 1 round
               "aesdec 0xc0(%6), %%xmm2\n\t"      // 2 round
               "aesdec 0xb0(%6), %%xmm2\n\t"      // 3 round
               "aesdec 0xa0(%6), %%xmm2\n\t"      // ...
               "aesdec 0x90(%6), %%xmm2\n\t"      // ...
               "aesdec 0x80(%6), %%xmm2\n\t"      // ...
               "aesdec 0x70(%6), %%xmm2\n\t"      // ..
               "aesdec 0x60(%6), %%xmm2\n\t"      // ..
               "aesdec 0x50(%6), %%xmm2\n\t"      // ..
               "aesdec 0x40(%6), %%xmm2\n\t"      // ..
               "aesdec 0x30(%6), %%xmm2\n\t"      // ...
               "aesdec 0x20(%6), %%xmm2\n\t"      // ...
               "aesdec 0x10(%6), %%xmm2\n\t"      // ...
               "aesdeclast 0x00(%6), %%xmm2\n\t"  // 14 round is last for 256 bits key
               "pxor %%xmm1, %%xmm2\n\t"          // CBC xor
               "movdqu (%4), %%xmm1\n\t"          // move next block of cyphertext to %xmm1
               "movdqu %%xmm2, (%7)\n\t"          // move block on plaintext to out
               "addq $0x10, %4\n\t"               // advance in pointer on block size
               "addq $0x10, %7\n\t"               // advance out pointer on block size
               "cmpl $0x0f, %3\n\t"               // do we have more blocks of cyphertext?
               "jg 1b\n\t"                        // if yes - jump to start of block processing
               "movdqu %%xmm1, (%5)\n\t"          // move block of cyphertext to IV
               : "=r"(size), "=r"(p1), "=r"(p2)
               : "0"(size), "1r"(in), "r"(iv), "r"(align16(ctx)), "2r"(out)
               : "%xmm1", "%xmm2", "memory");
}

void crypto_x86_64_aesni256_ige_encrypt(vk_aes_ctx_t *vk_ctx, const uint8_t *in, uint8_t *out, int size, uint8_t iv[32]) {
  if (size < 16) {
    return;
  }
  aes256_ctx_t *ctx = &vk_ctx->u.ctx;
  char *a = static_cast<char *>(align16(ctx));
  v16qi Y = loaddqu((const char *) iv);
  v16qi X = loaddqu((const char *) iv + 16);
  do {
    v16qi I = loaddqu((const char *) in), O;
    size -= 16;
    asm("pxor (%2), %0\n\t"
        "aesenc 0x10(%2), %0\n\t"
        "aesenc 0x20(%2), %0\n\t"
        "aesenc 0x30(%2), %0\n\t"
        "aesenc 0x40(%2), %0\n\t"
        "aesenc 0x50(%2), %0\n\t"
        "aesenc 0x60(%2), %0\n\t"
        "aesenc 0x70(%2), %0\n\t"
        "aesenc 0x80(%2), %0\n\t"
        "aesenc 0x90(%2), %0\n\t"
        "aesenc 0xa0(%2), %0\n\t"
        "aesenc 0xb0(%2), %0\n\t"
        "aesenc 0xc0(%2), %0\n\t"
        "aesenc 0xd0(%2), %0\n\t"
        "aesenclast 0xe0(%2), %0\n\t"
        : "=x"(O)
        : "0"(I ^ Y), "r"(a));
    Y = O ^ X;
    X = I;
    storedqu((char *) out, Y);
    in += 16;
    out += 16;
  } while (size >= 16);
  storedqu((char *) iv, Y);
  storedqu((char *) iv + 16, X);
}

void crypto_x86_64_aesni256_ige_decrypt(vk_aes_ctx_t *vk_ctx, const uint8_t *in, uint8_t *out, int size, uint8_t iv[32]) {
  if (size < 16) {
    return;
  }
  aes256_ctx_t *ctx = &vk_ctx->u.ctx;
  char *a = static_cast<char *>(align16(ctx));
  v16qi Y = loaddqu((const char *)iv);
  v16qi X = loaddqu((const char *)iv + 16);
  do {
    v16qi I = loaddqu((const char *)in), O;
    size -= 16;
    asm("pxor 0xe0(%2), %0\n\t"
        "aesdec 0xd0(%2), %0\n\t"
        "aesdec 0xc0(%2), %0\n\t"
        "aesdec 0xb0(%2), %0\n\t"
        "aesdec 0xa0(%2), %0\n\t"
        "aesdec 0x90(%2), %0\n\t"
        "aesdec 0x80(%2), %0\n\t"
        "aesdec 0x70(%2), %0\n\t"
        "aesdec 0x60(%2), %0\n\t"
        "aesdec 0x50(%2), %0\n\t"
        "aesdec 0x40(%2), %0\n\t"
        "aesdec 0x30(%2), %0\n\t"
        "aesdec 0x20(%2), %0\n\t"
        "aesdec 0x10(%2), %0\n\t"
        "aesdeclast 0x00(%2), %0\n\t"
        : "=x"(O)
        : "0"(I ^ X), "r"(a));
    X = O ^ Y;
    Y = I;
    storedqu((char *)out, X);
    in += 16;
    out += 16;
  } while (size >= 16);
  storedqu((char *)iv, Y);
  storedqu((char *)iv + 16, X);
}

/* in, out should be aligned 16 */
static inline void crypto_x86_64_aesni256_encrypt(uint8_t *ctx, const uint8_t *in, uint8_t *out) {
  asm("pxor (%2), %1\n\t"
      "aesenc 0x10(%2), %1\n\t"
      "aesenc 0x20(%2), %1\n\t"
      "aesenc 0x30(%2), %1\n\t"
      "aesenc 0x40(%2), %1\n\t"
      "aesenc 0x50(%2), %1\n\t"
      "aesenc 0x60(%2), %1\n\t"
      "aesenc 0x70(%2), %1\n\t"
      "aesenc 0x80(%2), %1\n\t"
      "aesenc 0x90(%2), %1\n\t"
      "aesenc 0xa0(%2), %1\n\t"
      "aesenc 0xb0(%2), %1\n\t"
      "aesenc 0xc0(%2), %1\n\t"
      "aesenc 0xd0(%2), %1\n\t"
      "aesenclast 0xe0(%2), %1\n\t"
      : "=x"(*((v2di *) out))
      : "0"(*((v2di *) in)), "r"(ctx)
      :);
}

void crypto_x86_64_aesni256_ctr_encrypt(vk_aes_ctx_t *ctx, const uint8_t *in, uint8_t *out, int size, uint8_t iv[16], uint64_t offset) {
  unsigned char *a = static_cast<unsigned char *>(align16(&ctx->u.ctx.a[0]));
  unsigned char iv_copy[16], u[16];
  memcpy(iv_copy, iv, 16);
  unsigned long long *p = (unsigned long long *)(iv_copy + 8);
  (*p) += offset >> 4;
  int i = offset & 15, l;
  if (i) {
    crypto_x86_64_aesni256_encrypt(a, iv_copy, u);
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
    v2di IV = *((v2di *)iv_copy), INC;
    asm("movd %1, %0\n\t"
        "pshufd $0xcf, %0, %0\n\t"
        : "=x"(INC)
        : "r"(1));
    do {
      v2di I = (v2di)loaddqu((const char *)in), T;
      asm("pxor (%2), %1\n\t"
          "aesenc 0x10(%2), %1\n\t"
          "aesenc 0x20(%2), %1\n\t"
          "aesenc 0x30(%2), %1\n\t"
          "aesenc 0x40(%2), %1\n\t"
          "aesenc 0x50(%2), %1\n\t"
          "aesenc 0x60(%2), %1\n\t"
          "aesenc 0x70(%2), %1\n\t"
          "aesenc 0x80(%2), %1\n\t"
          "aesenc 0x90(%2), %1\n\t"
          "aesenc 0xa0(%2), %1\n\t"
          "aesenc 0xb0(%2), %1\n\t"
          "aesenc 0xc0(%2), %1\n\t"
          "aesenc 0xd0(%2), %1\n\t"
          "aesenclast 0xe0(%2), %1\n\t"
          : "=x"(T)
          : "0"(IV), "r"(a));
      in += 16;
      storedqu((char *)out, (v16qi)(I ^ T));
      IV += INC;
      out += 16;
    } while (--n);
    *((v2di *)iv_copy) = IV;
  }

  l = size & 15;
  if (l) {
    crypto_x86_64_aesni256_encrypt(a, iv_copy, u);
    i = 0;
    do {
      *out++ = (*in++) ^ u[i++];
    } while (i < l);
  }
}
