// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/confdata/predefined-wildcards.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-light/stdlib/confdata/detail/predefined-wildcards-layout.h"

namespace {

using metadata_header = kphp::confdata::detail::predefined_wildcards_metadata_header;
using wildcard_entry = kphp::confdata::detail::predefined_wildcard_entry;
using wildcard_group = kphp::confdata::detail::predefined_wildcard_group;

auto is_valid_predefined_wildcard(std::string_view wildcard) noexcept -> bool {
  if (wildcard.empty() || wildcard.size() > kphp::confdata::MAX_KEY_LENGTH) [[unlikely]] {
    return false;
  }

  size_t dots{};
  if (wildcard.back() == '.') {
    for (const char c : wildcard) {
      dots += (c == '.');
      if (dots > 2) {
        break;
      }
    }
  }
  return dots != 1 && dots != 2;
}

auto load_wildcard(const std::byte* data, const metadata_header& header, uint32_t index) noexcept -> std::string_view {
  const auto entry{kphp::confdata::detail::load<wildcard_entry>(data, header.entries_offset + static_cast<size_t>(index) * sizeof(wildcard_entry))};
  return {reinterpret_cast<const char*>(data + entry.string_offset), entry.string_size};
}

auto load_group(const std::byte* data, const metadata_header& header, uint32_t index) noexcept -> wildcard_group {
  return kphp::confdata::detail::load<wildcard_group>(data, header.groups_offset + static_cast<size_t>(index) * sizeof(wildcard_group));
}

auto has_valid_layout(size_t buffer_size, const metadata_header& header) noexcept -> bool {
  // Recompute all offsets instead of trusting the serialized ones. This also
  // proves that every subsequent fixed-size load fits in `header.total_size`.
  const auto layout{kphp::confdata::detail::calculate_predefined_wildcards_metadata_layout(header.wildcard_count, header.group_count, header.strings_size,
                                                                                           header.shortest_wildcard_size, header.max_matches_per_key)};
  if (!layout || header.total_size != layout->total_size || header.entries_offset != layout->entries_offset || header.groups_offset != layout->groups_offset ||
      header.strings_offset != layout->strings_offset || header.total_size > buffer_size) [[unlikely]] {
    return false;
  }

  if (header.wildcard_count == 0) {
    return header.group_count == 0 && header.strings_size == 0 && header.shortest_wildcard_size == 0 && header.max_matches_per_key == 0;
  }
  return header.group_count != 0 && header.shortest_wildcard_size != 0 && header.max_matches_per_key != 0;
}

auto has_valid_entries(const std::byte* data, const metadata_header& header) noexcept -> bool {
  size_t next_string_offset{header.strings_offset};
  size_t shortest_wildcard_size{std::numeric_limits<size_t>::max()};
  std::string_view previous_wildcard{};

  for (uint32_t i{}; i < header.wildcard_count; ++i) {
    const auto entry{kphp::confdata::detail::load<wildcard_entry>(data, header.entries_offset + static_cast<size_t>(i) * sizeof(wildcard_entry))};
    const auto string_end{kphp::confdata::detail::checked_add(entry.string_offset, entry.string_size)};

    // Strings are packed without gaps. Besides enforcing a canonical encoding,
    // this prevents entries from overlapping or referring outside the blob.
    if (entry.string_offset != next_string_offset || !string_end || *string_end > header.total_size) [[unlikely]] {
      return false;
    }

    const std::string_view wildcard{reinterpret_cast<const char*>(data + entry.string_offset), entry.string_size};
    if (!is_valid_predefined_wildcard(wildcard) || (i != 0 && previous_wildcard >= wildcard)) [[unlikely]] {
      return false;
    }
    next_string_offset = *string_end;
    shortest_wildcard_size = std::min(shortest_wildcard_size, wildcard.size());
    previous_wildcard = wildcard;
  }

  return next_string_offset == header.total_size && (header.wildcard_count == 0 || shortest_wildcard_size == header.shortest_wildcard_size);
}

auto has_valid_groups(const std::byte* data, const metadata_header& header) noexcept -> bool {
  uint32_t next_entry{};
  size_t max_matches_per_key{};
  std::string_view previous_prefix{};

  for (uint32_t i{}; i < header.group_count; ++i) {
    const auto group{load_group(data, header, i)};
    if (group.first_entry != next_entry || group.entry_count == 0 || group.entry_count > header.wildcard_count - group.first_entry) [[unlikely]] {
      return false;
    }

    // A group is exactly one run of wildcards sharing a prefix whose length is
    // the shortest wildcard size. These prefixes form the lookup index.
    const auto group_prefix{load_wildcard(data, header, group.first_entry).substr(0, header.shortest_wildcard_size)};
    if (i != 0 && previous_prefix >= group_prefix) [[unlikely]] {
      return false;
    }

    for (uint32_t j{}; j < group.entry_count; ++j) {
      const auto wildcard{load_wildcard(data, header, group.first_entry + j)};
      if (wildcard.substr(0, header.shortest_wildcard_size) != group_prefix) [[unlikely]] {
        return false;
      }

      size_t matches{1};
      for (uint32_t k{}; k < j; ++k) {
        matches += wildcard.starts_with(load_wildcard(data, header, group.first_entry + k)) ? 1 : 0;
      }
      max_matches_per_key = std::max(max_matches_per_key, matches);
    }
    next_entry += group.entry_count;
    previous_prefix = group_prefix;
  }

  return next_entry == header.wildcard_count && max_matches_per_key == header.max_matches_per_key;
}

} // namespace

namespace kphp::confdata {

predefined_wildcards::predefined_wildcards(const std::byte* data, uint32_t entries_offset, uint32_t groups_offset, uint32_t wildcard_count,
                                           uint32_t group_count, uint32_t shortest_wildcard_size, uint32_t max_matches_per_key) noexcept
    : m_data{data},
      m_entries_offset{entries_offset},
      m_groups_offset{groups_offset},
      m_wildcard_count{wildcard_count},
      m_group_count{group_count},
      m_shortest_wildcard_size{shortest_wildcard_size},
      m_max_matches_per_key{max_matches_per_key} {}

auto predefined_wildcards::wildcard_at(uint32_t index) const noexcept -> std::string_view {
  const auto entry{
      detail::load<detail::predefined_wildcard_entry>(m_data, m_entries_offset + static_cast<size_t>(index) * sizeof(detail::predefined_wildcard_entry))};
  return {reinterpret_cast<const char*>(m_data + entry.string_offset), entry.string_size};
}

auto predefined_wildcards::matching_group(std::string_view key) const noexcept -> std::pair<uint32_t, uint32_t> {
  if (m_group_count == 0 || key.size() < m_shortest_wildcard_size) {
    return {};
  }

  const auto key_prefix{key.substr(0, m_shortest_wildcard_size)};
  uint32_t first{};
  uint32_t last{m_group_count};
  while (first < last) {
    const uint32_t middle{first + (last - first) / 2};
    const auto group{
        detail::load<detail::predefined_wildcard_group>(m_data, m_groups_offset + static_cast<size_t>(middle) * sizeof(detail::predefined_wildcard_group))};
    const auto group_prefix{wildcard_at(group.first_entry).substr(0, m_shortest_wildcard_size)};
    if (group_prefix < key_prefix) {
      first = middle + 1;
    } else {
      last = middle;
    }
  }
  if (first == m_group_count) {
    return {};
  }
  const auto group{
      detail::load<detail::predefined_wildcard_group>(m_data, m_groups_offset + static_cast<size_t>(first) * sizeof(detail::predefined_wildcard_group))};
  if (wildcard_at(group.first_entry).substr(0, m_shortest_wildcard_size) != key_prefix) {
    return {};
  }
  return {group.first_entry, group.entry_count};
}

auto predefined_wildcards::contains(std::string_view wildcard) const noexcept -> bool {
  uint32_t first{};
  uint32_t last{m_wildcard_count};
  while (first < last) {
    const uint32_t middle{first + (last - first) / 2};
    if (wildcard_at(middle) < wildcard) {
      first = middle + 1;
    } else {
      last = middle;
    }
  }
  return first != m_wildcard_count && wildcard_at(first) == wildcard;
}

auto predefined_wildcards::is_top_level_wildcard(std::string_view wildcard) const noexcept -> bool {
  if (!contains(wildcard)) {
    return false;
  }
  size_t matches{};
  for_each_matching_wildcard(wildcard, [&matches](std::string_view /*unused*/) noexcept { ++matches; });
  return matches == 1;
}

auto open_predefined_wildcards(std::span<const std::byte> buffer) noexcept -> std::expected<predefined_wildcards, predefined_wildcards_error> {
  if (!detail::is_predefined_wildcards_metadata_aligned(buffer.data())) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::misaligned_buffer};
  }
  if (buffer.size() < sizeof(detail::predefined_wildcards_metadata_header)) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::invalid_metadata};
  }

  const auto header{detail::load<detail::predefined_wildcards_metadata_header>(buffer.data(), 0)};
  if (header.magic != detail::PREDEFINED_WILDCARDS_METADATA_MAGIC) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::invalid_metadata};
  }
  if (header.version != detail::PREDEFINED_WILDCARDS_METADATA_VERSION) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::unsupported_version};
  }
  if (!has_valid_layout(buffer.size(), header)) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::invalid_metadata};
  }
  if (!has_valid_entries(buffer.data(), header) || !has_valid_groups(buffer.data(), header)) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::invalid_metadata};
  }
  return predefined_wildcards{buffer.data(),      header.entries_offset,         header.groups_offset,      header.wildcard_count,
                              header.group_count, header.shortest_wildcard_size, header.max_matches_per_key};
}

} // namespace kphp::confdata
