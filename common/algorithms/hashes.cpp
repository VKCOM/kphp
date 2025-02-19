// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/algorithms/hashes.h"

#include <cstddef>
#include <cstdint>

namespace {

size_t unaligned_load(const char *p) {
  size_t result = 0;
  __builtin_memcpy(&result, p, sizeof(result));
  return result;
}

// Loads n bytes, where 1 <= n < 8.
size_t load_bytes(const char *p, int n) {
  size_t result = 0;
  --n;
  do {
    result = (result << 8) + static_cast<unsigned char>(p[n]);
  } while (--n >= 0);
  return result;
}

size_t shift_mix(size_t v) {
  return v ^ (v >> 47);
}

} // namespace

namespace vk {

// MurMur hash function was taken from libstdc++
template<>
uint64_t murmur_hash<uint64_t>(const void *ptr, size_t len, size_t seed) noexcept {
  static const size_t mul = (static_cast<size_t>(0xc6a4a793UL) << 32UL) + static_cast<size_t>(0x5bd1e995UL);
  const char *const buf = static_cast<const char *>(ptr);
  // Remove the bytes not divisible by the sizeof(size_t).  This
  // allows the main loop to process the data as 64-bit integers.
  const int len_aligned = len & ~0x7;
  const char *const end = buf + len_aligned;
  size_t hash = seed ^ (len * mul);
  for (const char *p = buf; p != end; p += 8) {
    const size_t data = shift_mix(unaligned_load(p) * mul) * mul;
    hash ^= data;
    hash *= mul;
  }
  if ((len & 0x7) != 0) {
    const size_t data = load_bytes(end, len & 0x7);
    hash ^= data;
    hash *= mul;
  }
  hash = shift_mix(hash) * mul;
  hash = shift_mix(hash);
  return hash;
}

template<>
uint32_t murmur_hash<uint32_t>(const void *ptr, size_t len, size_t seed) noexcept {
  uint64_t res = murmur_hash<uint64_t>(ptr, len, seed);
  uint64_t mask = (1UL << 32UL) - 1;
  auto head = static_cast<uint32_t>(res & mask);
  auto tail = static_cast<uint32_t>((res >> 32UL) & (mask));
  return head ^ tail;
}

} // namespace vk
