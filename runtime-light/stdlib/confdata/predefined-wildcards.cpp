// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/confdata/predefined-wildcards.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <expected>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace kphp::confdata {

auto predefined_wildcards::shortest_matching_wildcard(std::string_view key) const noexcept -> std::optional<std::string_view> {
  const auto candidates{find_matching_candidates(key)};
  for (const auto& wildcard : candidates.wildcards) {
    if (candidates.key_tail.starts_with(wildcard.substr(m_shortest_wildcard_size))) {
      return wildcard;
    }
  }
  return std::nullopt;
}

auto predefined_wildcards::is_top_level_wildcard(std::string_view wildcard) const noexcept -> bool {
  const auto shortest{shortest_matching_wildcard(wildcard)};
  return shortest.has_value() && *shortest == wildcard;
}

auto predefined_wildcards::has_matching_wildcard(std::string_view key) const noexcept -> bool {
  return shortest_matching_wildcard(key).has_value();
}

auto predefined_wildcards::initialize(std::span<const std::string_view> wildcards,
                                      size_t memory_budget) noexcept -> std::expected<void, predefined_wildcards_error> {
  if (m_initialized) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::already_initialized};
  }

  constexpr auto max_size{std::numeric_limits<size_t>::max()};
  size_t required_bytes{};
  size_t shortest_wildcard_size{max_size};

  bool first{true};
  std::string_view previous{};
  for (const auto& wildcard : wildcards) {
    if (const auto validated{validate_predefined_wildcard(wildcard)}; !validated) [[unlikely]] {
      return std::unexpected{validated.error()};
    }
    if (!first && previous >= wildcard) [[unlikely]] {
      return std::unexpected{predefined_wildcards_error::non_canonical_wildcards};
    }
    // Validation bounds the individual string length; also guard the total size.
    const auto entry_bytes{sizeof(std::string_view) + wildcard.size()};
    if (required_bytes > max_size - entry_bytes) [[unlikely]] {
      return std::unexpected{predefined_wildcards_error::not_enough_memory};
    }
    required_bytes += entry_bytes;
    shortest_wildcard_size = std::min(shortest_wildcard_size, wildcard.size());
    previous = wildcard;
    first = false;
  }

  previous = {};
  size_t group_count{};
  for (const auto& wildcard : wildcards) {
    const auto prefix{wildcard.substr(0, shortest_wildcard_size)};
    if (prefix != previous) {
      ++group_count;
      previous = prefix;
    }
  }
  if (group_count > (max_size - required_bytes) / sizeof(wildcard_group)) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::not_enough_memory};
  }
  required_bytes += group_count * sizeof(wildcard_group);

  const auto aligned_bytes{memory_resource::details::align_for_chunk(required_bytes)};
  if (aligned_bytes < required_bytes || aligned_bytes >= memory_budget || !m_resource.is_enough_memory_for(aligned_bytes)) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::not_enough_memory};
  }
  if (wildcards.empty()) {
    m_initialized = true;
    return {};
  }

  // One pool allocation contains three consecutive regions, followed by alignment padding:
  // [string_view[wildcards.size()]][wildcard_group[group_count]][copied string bytes][padding]
  //  ^ views                      ^ groups                     ^ bytes
  // Each view points into the copied bytes; strings have no NUL terminators and
  // retain the input's lexicographic order. Each group is a span into `views`
  // covering consecutive wildcards with the same `shortest_wildcard_size` prefix.
  // For example, {"ab", "abc", "xy"} stores "ababcxy" and has groups spanning
  // views [0, 2) (prefix "ab") and [2, 3) (prefix "xy"). Neither array owns its
  // pointees separately: the entire allocation lives with the shared-memory piece.
  //
  // Preflight the single allocation before entering the resource's fatal OOM path.
  // No allocations or recoverable failures occur while populating the index.
  static_assert(memory_resource::details::align_for_chunk(1) % alignof(std::string_view) == 0);
  static_assert(sizeof(std::string_view) % alignof(wildcard_group) == 0);
  auto* memory{static_cast<std::byte*>(m_resource.allocate(aligned_bytes))};
  auto* views{reinterpret_cast<std::string_view*>(memory)};
  auto* groups{reinterpret_cast<wildcard_group*>(memory + wildcards.size() * sizeof(std::string_view))};
  auto* bytes{reinterpret_cast<char*>(memory + wildcards.size() * sizeof(std::string_view) + group_count * sizeof(wildcard_group))};
  for (size_t i{}; i < wildcards.size(); ++i) {
    const auto wildcard{wildcards[i]};
    std::memcpy(bytes, wildcard.data(), wildcard.size());
    std::construct_at(views + i, bytes, wildcard.size());
    bytes += wildcard.size();
  }

  size_t group_index{};
  for (size_t begin{}; begin < wildcards.size();) {
    const auto prefix{views[begin].substr(0, shortest_wildcard_size)};
    size_t end{begin + 1};
    while (end < wildcards.size() && views[end].starts_with(prefix)) {
      ++end;
    }
    const wildcard_group group{views + begin, end - begin};
    std::construct_at(groups + group_index++, group);
    for (size_t i{}; i < group.size(); ++i) {
      size_t matches{};
      for (size_t j{}; j <= i; ++j) {
        matches += static_cast<size_t>(group[i].starts_with(group[j]));
      }
      m_max_matches_per_key = std::max(m_max_matches_per_key, matches);
    }
    begin = end;
  }

  m_wildcards = {views, wildcards.size()};
  m_groups = {groups, group_count};
  m_shortest_wildcard_size = shortest_wildcard_size;
  m_initialized = true;
  return {};
}

auto predefined_wildcards::find_matching_candidates(std::string_view key) const noexcept -> matching_candidates {
  if (m_groups.empty() || key.size() < m_shortest_wildcard_size) {
    return {};
  }
  const auto prefix{key.substr(0, m_shortest_wildcard_size)};
  const auto group_prefix{[this](wildcard_group group) noexcept { return group.front().substr(0, m_shortest_wildcard_size); }};
  const auto group_it{std::ranges::lower_bound(m_groups, prefix, {}, group_prefix)};
  if (group_it == m_groups.end() || group_prefix(*group_it) != prefix) {
    return {};
  }
  return {.wildcards = *group_it, .key_tail = key.substr(m_shortest_wildcard_size)};
}

} // namespace kphp::confdata
