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
#include <utility>

namespace kphp::confdata {

inline constexpr auto MAX_KEY_LENGTH{static_cast<size_t>(std::numeric_limits<int16_t>::max())};
inline constexpr size_t PREDEFINED_WILDCARDS_ALIGNMENT{alignof(uint64_t)};

enum class predefined_wildcards_error : uint8_t {
  empty_wildcard,
  wildcard_too_long,
  reserved_wildcard,
  non_canonical_wildcards,
  size_overflow,
  insufficient_buffer,
  misaligned_buffer,
  invalid_metadata,
  unsupported_version,
};

class predefined_wildcards final {
public:
  predefined_wildcards() noexcept = default;

  /**
   * @brief Invokes `f(wildcard)` for every configured wildcard that is a prefix of `key`.
   *        Matching wildcards are visited in ascending length order.
   */
  template<std::invocable<std::string_view> F>
  auto for_each_matching_wildcard(std::string_view key, const F& f) const noexcept -> void;

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
  const std::byte* m_data{};
  uint32_t m_entries_offset{};
  uint32_t m_groups_offset{};
  uint32_t m_wildcard_count{};
  uint32_t m_group_count{};
  uint32_t m_shortest_wildcard_size{};
  uint32_t m_max_matches_per_key{};

  predefined_wildcards(const std::byte* data, uint32_t entries_offset, uint32_t groups_offset, uint32_t wildcard_count, uint32_t group_count,
                       uint32_t shortest_wildcard_size, uint32_t max_matches_per_key) noexcept;

  auto wildcard_at(uint32_t index) const noexcept -> std::string_view;
  auto matching_group(std::string_view key) const noexcept -> std::pair<uint32_t, uint32_t>;

  friend auto open_predefined_wildcards(std::span<const std::byte>) noexcept -> std::expected<predefined_wildcards, predefined_wildcards_error>;
};

/** @brief Validates and opens relocatable immutable wildcard metadata. */
auto open_predefined_wildcards(std::span<const std::byte> buffer) noexcept -> std::expected<predefined_wildcards, predefined_wildcards_error>;

template<std::invocable<std::string_view> F>
auto predefined_wildcards::for_each_matching_wildcard(std::string_view key, const F& f) const noexcept -> void {
  const auto [first_entry, entry_count]{matching_group(key)};
  for (uint32_t i{0}; i < entry_count; ++i) {
    const auto wildcard{wildcard_at(first_entry + i)};
    if (wildcard.size() <= key.size() && key.starts_with(wildcard)) {
      std::invoke(f, wildcard);
    }
  }
}

inline auto predefined_wildcards::shortest_matching_wildcard(std::string_view key) const noexcept -> std::optional<std::string_view> {
  std::optional<std::string_view> result{};
  for_each_matching_wildcard(key, [&result](std::string_view wildcard) noexcept {
    if (!result.has_value()) {
      result = wildcard;
    }
  });
  return result;
}

inline auto predefined_wildcards::max_matches_per_key() const noexcept -> size_t {
  return m_max_matches_per_key;
}

inline auto predefined_wildcards::has_matching_wildcard(std::string_view key) const noexcept -> bool {
  return shortest_matching_wildcard(key).has_value();
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
    case predefined_wildcards_error::size_overflow:
      return std::format_to(ctx.out(), "metadata size overflow");
    case predefined_wildcards_error::insufficient_buffer:
      return std::format_to(ctx.out(), "insufficient metadata buffer");
    case predefined_wildcards_error::misaligned_buffer:
      return std::format_to(ctx.out(), "misaligned metadata buffer");
    case predefined_wildcards_error::invalid_metadata:
      return std::format_to(ctx.out(), "invalid wildcard metadata");
    case predefined_wildcards_error::unsupported_version:
      return std::format_to(ctx.out(), "unsupported wildcard metadata version");
    }
    return std::format_to(ctx.out(), "unknown wildcard metadata error");
  }
};
