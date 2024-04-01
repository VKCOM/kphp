// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/auxv.h>

#include <arm_acle.h>
#include <arm_neon.h>

#include "common/cpuid.h"
#include "common/crc32c.h"

#define CRC32CX(crc, value) \
  __asm__("crc32cx %w[c], %w[c], %x[v]" : [c] "+r"(crc) : [v] "r"(value))
#define CRC32CW(crc, value) \
  __asm__("crc32cw %w[c], %w[c], %w[v]" : [c] "+r"(crc) : [v] "r"(value))
#define CRC32CH(crc, value) \
  __asm__("crc32ch %w[c], %w[c], %w[v]" : [c] "+r"(crc) : [v] "r"(value))
#define CRC32CB(crc, value) \
  __asm__("crc32cb %w[c], %w[c], %w[v]" : [c] "+r"(crc) : [v] "r"(value))

static uint32_t aarch64_native_crc(const void *buffer, long int len,
                                   uint32_t crc) {
  const uint8_t *p = static_cast<const uint8_t *>(buffer);
  int64_t length = len;

  while ((length -= sizeof(uint64_t)) >= 0) {
    CRC32CX(crc, *(uint64_t *)(p));
    p += sizeof(uint64_t);
  }

  /* The following is more efficient than the straight loop */
  if (length & sizeof(uint32_t)) {
    CRC32CW(crc, *(uint32_t *)(p));
    p += sizeof(uint32_t);
  }
  if (length & sizeof(uint16_t)) {
    CRC32CH(crc, *(uint16_t *)(p));
    p += sizeof(uint16_t);
  }
  if (length & sizeof(uint8_t)) CRC32CB(crc, *p);

  return crc;
}

// Shamelessly stolen from google/crc32c

#define KBYTES 1032
#define SEGMENTBYTES 256

// compute 8bytes for each segment parallelly
#define CRC32C32BYTES(P, IND)                                             \
  do {                                                                    \
    crc1 = __crc32cd(                                                     \
        crc1, *((const uint64_t *)(P) + (SEGMENTBYTES / 8) * 1 + (IND))); \
    crc2 = __crc32cd(                                                     \
        crc2, *((const uint64_t *)(P) + (SEGMENTBYTES / 8) * 2 + (IND))); \
    crc3 = __crc32cd(                                                     \
        crc3, *((const uint64_t *)(P) + (SEGMENTBYTES / 8) * 3 + (IND))); \
    crc0 = __crc32cd(                                                     \
        crc0, *((const uint64_t *)(P) + (SEGMENTBYTES / 8) * 0 + (IND))); \
  } while (0);

// compute 8*8 bytes for each segment parallelly
#define CRC32C256BYTES(P, IND)      \
  do {                              \
    CRC32C32BYTES((P), (IND)*8 + 0) \
    CRC32C32BYTES((P), (IND)*8 + 1) \
    CRC32C32BYTES((P), (IND)*8 + 2) \
    CRC32C32BYTES((P), (IND)*8 + 3) \
    CRC32C32BYTES((P), (IND)*8 + 4) \
    CRC32C32BYTES((P), (IND)*8 + 5) \
    CRC32C32BYTES((P), (IND)*8 + 6) \
    CRC32C32BYTES((P), (IND)*8 + 7) \
  } while (0);

// compute 4*8*8 bytes for each segment parallelly
#define CRC32C1024BYTES(P)   \
  do {                       \
    CRC32C256BYTES((P), 0)   \
    CRC32C256BYTES((P), 1)   \
    CRC32C256BYTES((P), 2)   \
    CRC32C256BYTES((P), 3)   \
    (P) += 4 * SEGMENTBYTES; \
  } while (0)

static uint32_t aarch64_native_crc_pmull(const void *buf, long int size,
                                         unsigned crc) {
  int64_t length = size;
  uint32_t crc0, crc1, crc2, crc3;
  uint64_t t0, t1, t2;

  // k0=CRC(x^(3*SEGMENTBYTES*8)), k1=CRC(x^(2*SEGMENTBYTES*8)),
  // k2=CRC(x^(SEGMENTBYTES*8))
  const poly64_t k0 = 0x8d96551c, k1 = 0xbd6f81f8, k2 = 0xdcb17aa4;

  const uint8_t *p = static_cast<const uint8_t *>(buf);

  while (length >= KBYTES) {
    crc0 = crc;
    crc1 = 0;
    crc2 = 0;
    crc3 = 0;

    // Process 1024 bytes in parallel.
    CRC32C1024BYTES(p);

    // Merge the 4 partial CRC32C values.
    t2 = (uint64_t)vmull_p64(crc2, k2);
    t1 = (uint64_t)vmull_p64(crc1, k1);
    t0 = (uint64_t)vmull_p64(crc0, k0);
    crc = __crc32cd(crc3, *(uint64_t *)p);
    p += sizeof(uint64_t);
    crc ^= __crc32cd(0, t2);
    crc ^= __crc32cd(0, t1);
    crc ^= __crc32cd(0, t0);

    length -= KBYTES;
  }

  while (length >= 8) {
    crc = __crc32cd(crc, *(uint64_t *)p);
    p += 8;
    length -= 8;
  }

  if (length & 4) {
    crc = __crc32cw(crc, *(uint32_t *)p);
    p += 4;
  }

  if (length & 2) {
    crc = __crc32ch(crc, *(uint16_t *)p);
    p += 2;
  }

  if (length & 1) {
    crc = __crc32cb(crc, *p);
  }

  return crc;
}

void crc32c_init() __attribute__((constructor));
void crc32c_init() {
  const kdb_cpuid_t *p = kdb_cpuid();
  assert(p->type == KDB_CPUID_AARCH64);

  crc32c_partial = crc32c_partial_four_tables;

  const unsigned long hwcaps = getauxval(AT_HWCAP);
  if (hwcaps & HWCAP_CRC32) {
    crc32c_partial = aarch64_native_crc;

    if (hwcaps & HWCAP_PMULL) {
      crc32c_partial = aarch64_native_crc_pmull;
    }
  }
}
