// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>

#include "runtime-light/stdlib/confdata/predefined-wildcards.h"

namespace kphp::confdata::detail {

inline constexpr uint64_t PREDEFINED_WILDCARDS_METADATA_MAGIC{0x444c49574443324bULL}; // "K2CDWILD" in little-endian byte order
inline constexpr uint32_t PREDEFINED_WILDCARDS_METADATA_VERSION{1};

struct alignas(PREDEFINED_WILDCARDS_ALIGNMENT) predefined_wildcards_metadata_header {
  uint64_t magic;
  uint32_t version;
  uint32_t total_size;
  uint32_t entries_offset;
  uint32_t groups_offset;
  uint32_t strings_offset;
  uint32_t strings_size;
  uint32_t wildcard_count;
  uint32_t group_count;
  uint32_t shortest_wildcard_size;
  uint32_t max_matches_per_key;
};

struct predefined_wildcard_entry {
  uint32_t string_offset;
  uint32_t string_size;
};

struct predefined_wildcard_group {
  uint32_t first_entry;
  uint32_t entry_count;
};

struct predefined_wildcards_metadata_layout {
  uint32_t total_size;
  uint32_t entries_offset;
  uint32_t groups_offset;
  uint32_t strings_offset;
  uint32_t strings_size;
  uint32_t wildcard_count;
  uint32_t group_count;
  uint32_t shortest_wildcard_size;
  uint32_t max_matches_per_key;
};

template<typename T>
auto load(const std::byte* data, size_t offset) noexcept -> T {
  T value{};
  std::memcpy(std::addressof(value), data + offset, sizeof(value));
  return value;
}

template<typename T>
auto store(std::byte* data, size_t offset, const T& value) noexcept -> void {
  std::memcpy(data + offset, std::addressof(value), sizeof(value));
}

inline auto checked_add(size_t lhs, size_t rhs) noexcept -> std::optional<size_t> {
  if (rhs > std::numeric_limits<size_t>::max() - lhs) [[unlikely]] {
    return std::nullopt;
  }
  return lhs + rhs;
}

inline auto checked_mul(size_t lhs, size_t rhs) noexcept -> std::optional<size_t> {
  if (lhs != 0 && rhs > std::numeric_limits<size_t>::max() / lhs) [[unlikely]] {
    return std::nullopt;
  }
  return lhs * rhs;
}

inline auto calculate_predefined_wildcards_metadata_layout(uint32_t wildcard_count, uint32_t group_count, uint32_t strings_size,
                                                           uint32_t shortest_wildcard_size,
                                                           uint32_t max_matches_per_key) noexcept -> std::optional<predefined_wildcards_metadata_layout> {
  const auto entries_size{checked_mul(wildcard_count, sizeof(predefined_wildcard_entry))};
  const auto groups_size{checked_mul(group_count, sizeof(predefined_wildcard_group))};
  if (!entries_size || !groups_size) [[unlikely]] {
    return std::nullopt;
  }

  const size_t entries_offset{sizeof(predefined_wildcards_metadata_header)};
  const auto groups_offset{checked_add(entries_offset, *entries_size)};
  const auto strings_offset{groups_offset.and_then([groups_size](size_t offset) noexcept { return checked_add(offset, *groups_size); })};
  const auto total_size{strings_offset.and_then([strings_size](size_t offset) noexcept { return checked_add(offset, strings_size); })};
  if (!groups_offset || !strings_offset || !total_size || *total_size > std::numeric_limits<uint32_t>::max()) [[unlikely]] {
    return std::nullopt;
  }

  return predefined_wildcards_metadata_layout{.total_size = static_cast<uint32_t>(*total_size),
                                              .entries_offset = static_cast<uint32_t>(entries_offset),
                                              .groups_offset = static_cast<uint32_t>(*groups_offset),
                                              .strings_offset = static_cast<uint32_t>(*strings_offset),
                                              .strings_size = strings_size,
                                              .wildcard_count = wildcard_count,
                                              .group_count = group_count,
                                              .shortest_wildcard_size = shortest_wildcard_size,
                                              .max_matches_per_key = max_matches_per_key};
}

inline auto is_predefined_wildcards_metadata_aligned(const void* pointer) noexcept -> bool {
  return reinterpret_cast<uintptr_t>(pointer) % PREDEFINED_WILDCARDS_ALIGNMENT == 0;
}

} // namespace kphp::confdata::detail
