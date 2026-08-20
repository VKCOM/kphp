// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/components/confdata/state/predefined-wildcards-builder.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <limits>
#include <span>
#include <string_view>

#include "runtime-light/stdlib/confdata/detail/predefined-wildcards-layout.h"
#include "runtime-light/stdlib/confdata/predefined-wildcards.h"

namespace {

auto analyze_canonical_wildcards(std::span<const std::string_view> wildcards) noexcept
    -> std::expected<kphp::confdata::detail::predefined_wildcards_metadata_layout, kphp::confdata::predefined_wildcards_error> {
  using kphp::confdata::predefined_wildcards_error;
  using kphp::confdata::validate_predefined_wildcard;

  if (wildcards.size() > std::numeric_limits<uint32_t>::max()) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::size_overflow};
  }
  if (wildcards.empty()) {
    return *kphp::confdata::detail::calculate_predefined_wildcards_metadata_layout(0, 0, 0, 0, 0);
  }

  size_t strings_size{};
  size_t shortest_wildcard_size{std::numeric_limits<size_t>::max()};
  for (size_t i{}; i < wildcards.size(); ++i) {
    if (const auto validated{validate_predefined_wildcard(wildcards[i])}; !validated) [[unlikely]] {
      return std::unexpected{validated.error()};
    }
    if (i != 0 && wildcards[i - 1] >= wildcards[i]) [[unlikely]] {
      return std::unexpected{predefined_wildcards_error::non_canonical_wildcards};
    }
    const auto new_strings_size{kphp::confdata::detail::checked_add(strings_size, wildcards[i].size())};
    if (!new_strings_size || *new_strings_size > std::numeric_limits<uint32_t>::max()) [[unlikely]] {
      return std::unexpected{predefined_wildcards_error::size_overflow};
    }
    strings_size = *new_strings_size;
    shortest_wildcard_size = std::min(shortest_wildcard_size, wildcards[i].size());
  }

  size_t group_count{1};
  size_t max_matches_per_key{};
  size_t group_begin{};
  for (size_t i{}; i < wildcards.size(); ++i) {
    const auto prefix{wildcards[i].substr(0, shortest_wildcard_size)};
    if (i != 0 && wildcards[i - 1].substr(0, shortest_wildcard_size) != prefix) {
      ++group_count;
      group_begin = i;
    }

    size_t matches{1};
    for (size_t j{group_begin}; j < i; ++j) {
      matches += wildcards[i].starts_with(wildcards[j]) ? 1 : 0;
    }
    max_matches_per_key = std::max(max_matches_per_key, matches);
  }

  if (group_count > std::numeric_limits<uint32_t>::max()) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::size_overflow};
  }
  const auto layout{kphp::confdata::detail::calculate_predefined_wildcards_metadata_layout(
      static_cast<uint32_t>(wildcards.size()), static_cast<uint32_t>(group_count), static_cast<uint32_t>(strings_size),
      static_cast<uint32_t>(shortest_wildcard_size), static_cast<uint32_t>(max_matches_per_key))};
  if (!layout) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::size_overflow};
  }
  return *layout;
}

auto header_from_layout(const kphp::confdata::detail::predefined_wildcards_metadata_layout& layout) noexcept
    -> kphp::confdata::detail::predefined_wildcards_metadata_header {
  return {.magic = kphp::confdata::detail::PREDEFINED_WILDCARDS_METADATA_MAGIC,
          .version = kphp::confdata::detail::PREDEFINED_WILDCARDS_METADATA_VERSION,
          .total_size = layout.total_size,
          .entries_offset = layout.entries_offset,
          .groups_offset = layout.groups_offset,
          .strings_offset = layout.strings_offset,
          .strings_size = layout.strings_size,
          .wildcard_count = layout.wildcard_count,
          .group_count = layout.group_count,
          .shortest_wildcard_size = layout.shortest_wildcard_size,
          .max_matches_per_key = layout.max_matches_per_key};
}

} // namespace

namespace kphp::confdata {

auto validate_predefined_wildcard(std::string_view wildcard) noexcept -> std::expected<void, predefined_wildcards_error> {
  if (wildcard.empty()) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::empty_wildcard};
  }
  if (wildcard.size() > MAX_KEY_LENGTH) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::wildcard_too_long};
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
  if (dots == 1 || dots == 2) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::reserved_wildcard};
  }
  return {};
}

auto predefined_wildcards_metadata_size(std::span<const std::string_view> wildcards) noexcept -> std::expected<size_t, predefined_wildcards_error> {
  const auto layout{analyze_canonical_wildcards(wildcards)};
  if (!layout) [[unlikely]] {
    return std::unexpected{layout.error()};
  }
  return layout->total_size;
}

auto write_predefined_wildcards(std::span<std::byte> buffer,
                                std::span<const std::string_view> wildcards) noexcept -> std::expected<predefined_wildcards, predefined_wildcards_error> {
  if (!detail::is_predefined_wildcards_metadata_aligned(buffer.data())) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::misaligned_buffer};
  }
  const auto layout{analyze_canonical_wildcards(wildcards)};
  if (!layout) [[unlikely]] {
    return std::unexpected{layout.error()};
  }
  if (buffer.size() < layout->total_size) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::insufficient_buffer};
  }

  detail::store(buffer.data(), 0, header_from_layout(*layout));
  size_t string_offset{layout->strings_offset};
  uint32_t group_index{};
  uint32_t group_begin{};
  for (uint32_t i{}; i < layout->wildcard_count; ++i) {
    const auto wildcard{wildcards[i]};
    detail::store(
        buffer.data(), layout->entries_offset + static_cast<size_t>(i) * sizeof(detail::predefined_wildcard_entry),
        detail::predefined_wildcard_entry{.string_offset = static_cast<uint32_t>(string_offset), .string_size = static_cast<uint32_t>(wildcard.size())});
    std::memcpy(buffer.data() + string_offset, wildcard.data(), wildcard.size());
    string_offset += wildcard.size();

    const bool group_finished{i + 1 == layout->wildcard_count ||
                              wildcard.substr(0, layout->shortest_wildcard_size) != wildcards[i + 1].substr(0, layout->shortest_wildcard_size)};
    if (group_finished) {
      detail::store(buffer.data(), layout->groups_offset + static_cast<size_t>(group_index) * sizeof(detail::predefined_wildcard_group),
                    detail::predefined_wildcard_group{.first_entry = group_begin, .entry_count = i - group_begin + 1});
      ++group_index;
      group_begin = i + 1;
    }
  }

  return open_predefined_wildcards(buffer.first(layout->total_size));
}

} // namespace kphp::confdata
