// Compiler for PHP (aka KPHP)
// msgpack (c) https://github.com/msgpack/msgpack-c/tree/cpp_master (copied as third-party and slightly modified)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#if defined(unix) || defined(__unix) || defined(__APPLE__) || defined(__OpenBSD__)

#include <arpa/inet.h> /* __BYTE_ORDER */
#if defined(__linux__)
#include <byteswap.h>
#endif

#endif

#ifdef __APPLE__
#include <machine/endian.h>
#else
#include <endian.h>
#endif

static_assert(BYTE_ORDER == LITTLE_ENDIAN);

#if defined(unix) || defined(__unix) || defined(__APPLE__) || defined(__OpenBSD__)
#define _msgpack_be16(x) ntohs(x)
#else
#if defined(ntohs)
#define _msgpack_be16(x) ntohs(x)
#elif defined(_byteswap_ushort)
#define _msgpack_be16(x) ((uint16_t)_byteswap_ushort((unsigned short)x))
#else
#define _msgpack_be16(x) (((((uint16_t)x) << 8)) | ((((uint16_t)x) >> 8)))
#endif
#endif

#if defined(unix) || defined(__unix) || defined(__APPLE__) || defined(__OpenBSD__)
#define _msgpack_be32(x) ntohl(x)
#else
#if defined(ntohl)
#define _msgpack_be32(x) ntohl(x)
#elif defined(_byteswap_ulong)
#define _msgpack_be32(x) ((uint32_t)_byteswap_ulong((unsigned long)x))
#else
#define _msgpack_be32(x) (((((uint32_t)x) << 24)) | ((((uint32_t)x) << 8) & 0x00ff0000U) | ((((uint32_t)x) >> 8) & 0x0000ff00U) | ((((uint32_t)x) >> 24)))
#endif
#endif

#if defined(_byteswap_uint64)
#define _msgpack_be64(x) (_byteswap_uint64(x))
#elif defined(bswap_64)
#define _msgpack_be64(x) bswap_64(x)
#elif defined(__DARWIN_OSSwapInt64)
#define _msgpack_be64(x) __DARWIN_OSSwapInt64(x)
#else
#define _msgpack_be64(x)                                                                                                                                       \
  (((((uint64_t)x) << 56)) | ((((uint64_t)x) << 40) & 0x00ff000000000000ULL) | ((((uint64_t)x) << 24) & 0x0000ff0000000000ULL) |                               \
   ((((uint64_t)x) << 8) & 0x000000ff00000000ULL) | ((((uint64_t)x) >> 8) & 0x00000000ff000000ULL) | ((((uint64_t)x) >> 24) & 0x0000000000ff0000ULL) |         \
   ((((uint64_t)x) >> 40) & 0x000000000000ff00ULL) | ((((uint64_t)x) >> 56)))
#endif

#define _msgpack_load16(cast, from, to)                                                                                                                        \
  do {                                                                                                                                                         \
    memcpy((cast*)(to), (from), sizeof(cast));                                                                                                                 \
    *(to) = _msgpack_be16(*(to));                                                                                                                              \
  } while (0);

#define _msgpack_load32(cast, from, to)                                                                                                                        \
  do {                                                                                                                                                         \
    memcpy((cast*)(to), (from), sizeof(cast));                                                                                                                 \
    *(to) = _msgpack_be32(*(to));                                                                                                                              \
  } while (0);
#define _msgpack_load64(cast, from, to)                                                                                                                        \
  do {                                                                                                                                                         \
    memcpy((cast*)(to), (from), sizeof(cast));                                                                                                                 \
    *(to) = _msgpack_be64(*(to));                                                                                                                              \
  } while (0);

#define _msgpack_store16(to, num)                                                                                                                              \
  do {                                                                                                                                                         \
    uint16_t val = _msgpack_be16(num);                                                                                                                         \
    memcpy(to, &val, 2);                                                                                                                                       \
  } while (0)
#define _msgpack_store32(to, num)                                                                                                                              \
  do {                                                                                                                                                         \
    uint32_t val = _msgpack_be32(num);                                                                                                                         \
    memcpy(to, &val, 4);                                                                                                                                       \
  } while (0)
#define _msgpack_store64(to, num)                                                                                                                              \
  do {                                                                                                                                                         \
    uint64_t val = _msgpack_be64(num);                                                                                                                         \
    memcpy(to, &val, 8);                                                                                                                                       \
  } while (0)
