// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/confdata/predefined-wildcards.h"

#include <algorithm>
#include <cstddef>
#include <expected>
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

auto predefined_wildcards::initialize(std::span<const std::string_view> wildcards) noexcept -> std::expected<void, predefined_wildcards_error> {
  if (m_initialized) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::already_initialized};
  }

  std::string_view previous{};
  bool first{true};
  for (const auto& wildcard : wildcards) {
    if (const auto validated{validate_predefined_wildcard(wildcard)}; !validated) [[unlikely]] {
      return std::unexpected{validated.error()};
    }
    if (!first && previous >= wildcard) [[unlikely]] {
      return std::unexpected{predefined_wildcards_error::non_canonical_wildcards};
    }
    previous = wildcard;
    first = false;
  }

  m_wildcards.reserve(wildcards.size());
  for (const auto& wildcard : wildcards) {
    const auto [it, inserted]{m_wildcards.emplace(wildcard, wildcard_string::allocator_type{m_resource})};
    if (!inserted) [[unlikely]] {
      return std::unexpected{predefined_wildcards_error::internal};
    }

    const std::string_view stored{*it};
    if (m_shortest_wildcard_size == 0 || stored.size() < m_shortest_wildcard_size) {
      m_shortest_wildcard_size = stored.size();
    }
  }

  m_groups.reserve(m_wildcards.size());
  for (const auto& wildcard_string : m_wildcards) {
    const std::string_view wildcard{wildcard_string};
    const auto group_it{m_groups.try_emplace(wildcard.substr(0, m_shortest_wildcard_size), wildcard_group::allocator_type{m_resource}).first};
    auto& group{group_it->second};
    group.emplace_back(wildcard);
  }

  for (auto& [_, group] : m_groups) {
    std::ranges::sort(group);
    for (size_t i{}; i < group.size(); ++i) {
      size_t matches{};
      for (size_t j{}; j <= i; ++j) {
        matches += static_cast<size_t>(group[i].starts_with(group[j]));
      }
      m_max_matches_per_key = std::max(m_max_matches_per_key, matches);
    }
  }

  m_initialized = true;
  return {};
}

auto predefined_wildcards::find_matching_candidates(std::string_view key) const noexcept -> matching_candidates {
  if (m_groups.empty() || key.size() < m_shortest_wildcard_size) {
    return {};
  }
  const auto group_it{m_groups.find(key.substr(0, m_shortest_wildcard_size))};
  if (group_it == m_groups.end()) {
    return {};
  }
  return {.wildcards = group_it->second, .key_tail = key.substr(m_shortest_wildcard_size)};
}

} // namespace kphp::confdata
