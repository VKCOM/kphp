//  Modified by LLC «V Kontakte», 2025 February 25
//
//  This file is part of the GNU C Library.
//  Copyright (C) 2002-2024 Free Software Foundation, Inc.
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "common/algorithms/hashes.h"

#include <cstddef>
#include <cstdint>

namespace {

constexpr size_t unaligned_load(const char *p) noexcept {
  size_t result = 0;
  __builtin_memcpy(&result, p, sizeof(result));
  return result;
}

// Loads n bytes, where 1 <= n < 8.
constexpr size_t load_bytes(const char *p, int n) noexcept {
  size_t result = 0;
  --n;
  do {
    result = (result << 8) + static_cast<unsigned char>(p[n]);
  } while (--n >= 0);
  return result;
}

constexpr size_t shift_mix(size_t v) noexcept {
  return v ^ (v >> 47);
}

} // namespace

namespace vk {

// MurMur hash function was taken from libstdc++
template<>
constexpr uint64_t murmur_hash<uint64_t>(const void *ptr, size_t len, size_t seed) noexcept {
  constexpr size_t mul = (static_cast<size_t>(0xc6a4a793UL) << 32UL) + static_cast<size_t>(0x5bd1e995UL);
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
constexpr uint32_t murmur_hash<uint32_t>(const void *ptr, size_t len, size_t seed) noexcept {
  uint64_t res = murmur_hash<uint64_t>(ptr, len, seed);
  uint64_t mask = (1UL << 32UL) - 1;
  auto head = static_cast<uint32_t>(res & mask);
  auto tail = static_cast<uint32_t>((res >> 32UL) & (mask));
  return head ^ tail;
}

} // namespace vk
