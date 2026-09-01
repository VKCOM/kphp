// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/resource_allocator.h"
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-light/stdlib/confdata/wildcard-kind.h"

namespace kphp::confdata {

inline constexpr auto MAX_KEY_LENGTH{static_cast<size_t>(std::numeric_limits<int16_t>::max())};

enum class predefined_wildcards_error : uint8_t {
  empty_wildcard,
  wildcard_too_long,
  reserved_wildcard,
  non_canonical_wildcards,
  already_initialized,
  internal,
};

inline auto validate_predefined_wildcard(std::string_view wildcard) noexcept -> std::expected<void, predefined_wildcards_error> {
  if (wildcard.empty()) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::empty_wildcard};
  }
  if (wildcard.size() > MAX_KEY_LENGTH) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::wildcard_too_long};
  }
  if (classify_wildcard_form(wildcard) != section_kind::predefined_wildcard) [[unlikely]] {
    return std::unexpected{predefined_wildcards_error::reserved_wildcard};
  }
  return {};
}

class storage;

/**
 * An immutable index of configured predefined wildcards.
 *
 * The owning strings and both lookup indexes retain the storage resource in
 * their allocators, so their allocation domain does not depend on whichever
 * script resource happens to be installed by the caller.
 */
class predefined_wildcards final : private vk::not_copyable {
  using resource_type = memory_resource::unsynchronized_pool_resource;
  using wildcard_string = memory_resource::stl::string<resource_type>;

  struct transparent_string_hash final {
    using is_transparent = void;

    auto operator()(std::string_view value) const noexcept -> size_t {
      return std::hash<std::string_view>{}(value);
    }
  };

  struct transparent_string_equal final {
    using is_transparent = void;

    auto operator()(std::string_view lhs, std::string_view rhs) const noexcept -> bool {
      return lhs == rhs;
    }
  };

  using wildcard_set = memory_resource::stl::unordered_set<wildcard_string, resource_type, transparent_string_hash, transparent_string_equal>;
  using wildcard_group = memory_resource::stl::vector<std::string_view, resource_type>;
  using wildcard_groups =
      memory_resource::stl::unordered_map<std::string_view, wildcard_group, resource_type, transparent_string_hash, transparent_string_equal>;

  /** Candidates from one shortest-prefix group and the unmatched part of the queried key. */
  struct matching_candidates final {
    std::span<const std::string_view> wildcards;
    std::string_view key_tail;
  };

  resource_type& m_resource;
  // Owns each complete wildcard exactly once. References remain stable across
  // unordered-set rehashes and the set is never mutated after initialization.
  wildcard_set m_wildcards;
  // Maps a shortest-length prefix to sorted views into `m_wildcards`.
  wildcard_groups m_groups;
  size_t m_shortest_wildcard_size{};
  size_t m_max_matches_per_key{};
  bool m_initialized{};

public:
  explicit predefined_wildcards(resource_type& resource) noexcept;

  /**
   * Invokes `f(wildcard)` for every configured wildcard that is a prefix of
   * `key`, in ascending length order.
   *
   * @return True if at least one wildcard matched.
   */
  template<std::invocable<std::string_view> F>
  auto for_each_matching_wildcard(std::string_view key, const F& f) const noexcept -> bool;

  /** @return The shortest configured wildcard that is a prefix of `key`, if any. */
  auto shortest_matching_wildcard(std::string_view key) const noexcept -> std::optional<std::string_view>;

  /** @return The exact maximum number of configured wildcards that can match one key. */
  auto max_matches_per_key() const noexcept -> size_t;

  /** @return True if `wildcard` is configured. */
  auto contains(std::string_view wildcard) const noexcept -> bool;

  /** @return True if `wildcard` is configured and has no shorter configured wildcard prefix. */
  auto is_top_level_wildcard(std::string_view wildcard) const noexcept -> bool;

  /** @return True if at least one configured wildcard is a prefix of `key`. */
  auto has_matching_wildcard(std::string_view key) const noexcept -> bool;

private:
  /** Initializes the index from sorted, unique wildcards under the storage resource. */
  auto initialize(std::span<const std::string_view> wildcards) noexcept -> std::expected<void, predefined_wildcards_error>;

  auto find_matching_candidates(std::string_view key) const noexcept -> matching_candidates;

  friend class storage;
};

inline predefined_wildcards::predefined_wildcards(resource_type& resource) noexcept
    : m_resource{resource},
      m_wildcards{wildcard_set::allocator_type{resource}},
      m_groups{wildcard_groups::allocator_type{resource}} {}

template<std::invocable<std::string_view> F>
auto predefined_wildcards::for_each_matching_wildcard(std::string_view key, const F& f) const noexcept -> bool {
  const auto candidates{find_matching_candidates(key)};
  bool matched{};
  for (const auto& wildcard : candidates.wildcards) {
    const auto wildcard_tail{wildcard.substr(m_shortest_wildcard_size)};
    if (candidates.key_tail.starts_with(wildcard_tail)) {
      std::invoke(f, wildcard);
      matched = true;
    }
  }
  return matched;
}

inline auto predefined_wildcards::max_matches_per_key() const noexcept -> size_t {
  return m_max_matches_per_key;
}

inline auto predefined_wildcards::contains(std::string_view wildcard) const noexcept -> bool {
  return m_wildcards.contains(wildcard);
}

} // namespace kphp::confdata

template<>
struct std::formatter<kphp::confdata::predefined_wildcards_error> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const noexcept {
    return ctx.begin();
  }

  template<typename FmtContext>
  auto format(kphp::confdata::predefined_wildcards_error error, FmtContext& ctx) const noexcept {
    using kphp::confdata::predefined_wildcards_error;

    switch (error) {
    case predefined_wildcards_error::empty_wildcard:
      return std::format_to(ctx.out(), "empty wildcard");
    case predefined_wildcards_error::wildcard_too_long:
      return std::format_to(ctx.out(), "wildcard is longer than the confdata key protocol limit");
    case predefined_wildcards_error::reserved_wildcard:
      return std::format_to(ctx.out(), "wildcard uses the implicit one-dot or two-dot form");
    case predefined_wildcards_error::non_canonical_wildcards:
      return std::format_to(ctx.out(), "wildcards are not sorted and unique");
    case predefined_wildcards_error::already_initialized:
      return std::format_to(ctx.out(), "wildcards are already initialized");
    case predefined_wildcards_error::internal:
      return std::format_to(ctx.out(), "unexpected internal error");
    }
    return std::format_to(ctx.out(), "unknown wildcard error");
  }
};
