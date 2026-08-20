// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <string_view>
#include <variant>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/overloaded.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/confdata/predefined-wildcards.h"

// A port of runtime/confdata-keys.h (minus the blacklist) shared by the confdata component and the kphp client.
// The client never includes this header directly; it's an implementation detail of the confdata sample reader/writer.
//
// Confdata keys are stored denormalized in two levels: a key is split into a `section`
// (the top-level storage key) and a `remainder` (the key inside the section's array):
//   - "key"           -> section "key"                                 (section_kind::simple_key)
//   - "a.b..."        -> section "a.",   remainder "b..."              (section_kind::one_dot_wildcard)
//   - "a.b.c..."      -> section "a.b.", remainder "c..."              (section_kind::two_dots_wildcard)
//   - "predefined..." -> section = the matching predefined wildcard    (section_kind::predefined_wildcard)
namespace kphp::confdata {

enum class section_kind : uint8_t { simple_key, one_dot_wildcard, two_dots_wildcard, predefined_wildcard };

inline auto classify_wildcard(std::string_view wildcard) noexcept -> section_kind {
  size_t dots{0};
  if (!wildcard.empty() && wildcard.back() == '.') {
    for (const char c : wildcard) {
      dots += (c == '.');
      if (dots > 2) {
        break;
      }
    }
  }
  switch (dots) {
  case 1:
    return section_kind::one_dot_wildcard;
  case 2:
    return section_kind::two_dots_wildcard;
  default:
    return section_kind::predefined_wildcard;
  }
}

/**
 * @brief Classifies `section`; a would-be predefined wildcard that is not configured is reported
 *        as `section_kind::simple_key`.
 */
inline auto classify_section(std::string_view section, const predefined_wildcards& wildcards) noexcept -> section_kind {
  const auto kind{classify_wildcard(section)};
  return kind != section_kind::predefined_wildcard || wildcards.contains(section) ? kind : section_kind::simple_key;
}

// ================================================================================================

enum class split_error : uint8_t { key_too_long, invalid_predefined_wildcard_length, not_a_two_dots_key };

/**
 * @brief The decomposition of a confdata key as zero-copy views into the key.
 *        Instances are produced only by the `split_key*` factories, so every `key_views`
 *        is guaranteed to satisfy the protocol length bound (`int16_t`).
 */
class key_views {
public:
  // the remainder of a key: absent for simple keys, int-normalized like a PHP array key otherwise
  using remainder_type = std::variant<std::monostate, int64_t, std::string_view>;

private:
  section_kind m_section_kind;
  std::string_view m_raw_key;
  std::string_view m_section;
  remainder_type m_remainder;

  key_views(section_kind section_kind, std::string_view raw_key, std::string_view section, remainder_type remainder) noexcept;

  friend auto split_key(std::string_view key) noexcept -> std::expected<key_views, split_error>;
  friend auto split_key(std::string_view key, const predefined_wildcards& wildcards) noexcept -> std::expected<key_views, split_error>;
  friend auto split_key_with_predefined_wildcard(std::string_view key, size_t wildcard_len) noexcept -> std::expected<key_views, split_error>;

public:
  auto kind() const noexcept -> section_kind;
  auto raw_key() const noexcept -> std::string_view;
  auto section() const noexcept -> std::string_view;
  auto remainder() const noexcept -> const remainder_type&;

  /**
   * @return The one-dot duplicate of a two-dot key (`a.b.c...` -> section `a.`, remainder `b.c...`),
   *         or `split_error::not_a_two_dots_key`.
   */
  auto reinterpret_two_dots_as_one_dot() const noexcept -> std::expected<key_views, split_error>;
};

inline key_views::key_views(section_kind section_kind, std::string_view raw_key, std::string_view section, remainder_type remainder) noexcept
    : m_section_kind{section_kind},
      m_raw_key{raw_key},
      m_section{section},
      m_remainder{remainder} {}

inline auto key_views::kind() const noexcept -> section_kind {
  return m_section_kind;
}

inline auto key_views::raw_key() const noexcept -> std::string_view {
  return m_raw_key;
}

inline auto key_views::section() const noexcept -> std::string_view {
  return m_section;
}

inline auto key_views::remainder() const noexcept -> const remainder_type& {
  return m_remainder;
}

inline auto key_views::reinterpret_two_dots_as_one_dot() const noexcept -> std::expected<key_views, split_error> {
  if (m_section_kind != section_kind::two_dots_wildcard) {
    return std::unexpected{split_error::not_a_two_dots_key};
  }
  // a two-dot key always contains a dot, and the remainder after the first dot always contains another one,
  // so the remainder is never numeric and needs no int-normalization
  const auto first_dot{m_raw_key.find('.')};
  const auto remainder{m_raw_key.substr(first_dot + 1)};
  return key_views{section_kind::one_dot_wildcard, m_raw_key, m_raw_key.substr(0, first_dot + 1), remainder_type{remainder}};
}

// ================================================================================================

/**
 * @brief Splits `key` into the section (up to the first/second dot) and the int-normalized remainder.
 */
auto split_key(std::string_view key) noexcept -> std::expected<key_views, split_error>;

/**
 * @brief Splits `key` using the shortest matching predefined wildcard as the section, if any.
 */
auto split_key(std::string_view key, const predefined_wildcards& wildcards) noexcept -> std::expected<key_views, split_error>;

/**
 * @brief Splits `key` using the explicitly given predefined wildcard length as the section.
 */
auto split_key_with_predefined_wildcard(std::string_view key, size_t wildcard_len) noexcept -> std::expected<key_views, split_error>;

// ================================================================================================

/**
 * @brief Materializes validated key views into runtime handles (`string`/`mixed`) for storage lookups,
 *        allocation-free: the handles are placement-constructed into the internal stack buffers.
 *        Immovable, since the handles point into the object's own buffers.
 */
class key_handles : vk::not_copyable { // NOLINT(*member-init)
  // Buffers precede the handles so that the handles are destroyed before the storage they refer to.
  alignas(std::max_align_t) std::array<std::byte, string::inner_sizeof() + std::numeric_limits<int16_t>::max() + 1> m_section_buffer;
  alignas(std::max_align_t) std::array<std::byte, string::inner_sizeof() + std::numeric_limits<int16_t>::max() + 1> m_remainder_buffer;

  string m_section;
  mixed m_remainder;

public:
  explicit key_handles(const key_views& views) noexcept; // NOLINT(*member-init)

  auto section() const noexcept -> const string&;

  auto remainder() const noexcept -> const mixed&;

  /**
   * @return A heap copy of the section; the internal section aliases the stack buffer and the raw key,
   *         so it must not escape the handles object.
   */
  auto make_section_copy() const noexcept -> string;

  /**
   * @return A heap copy of the remainder; the internal remainder aliases the stack buffer and the raw key,
   *         so it must not escape the handles object.
   */
  auto make_remainder_copy() const noexcept -> mixed;
};

inline key_handles::key_handles(const key_views& views) noexcept { // NOLINT(*member-init)
  m_section = views.section().empty() ? string{}
                                      : string::make_const_string_on_memory(views.section().data(), static_cast<string::size_type>(views.section().size()),
                                                                            m_section_buffer.data(), m_section_buffer.size());
  m_remainder = std::visit(overloaded{
                               [](std::monostate) noexcept -> mixed { return mixed{}; },
                               [](int64_t remainder) noexcept -> mixed { return mixed{remainder}; },
                               [this](std::string_view remainder) noexcept -> mixed {
                                 return remainder.empty()
                                            ? mixed{string{}}
                                            : mixed{string::make_const_string_on_memory(remainder.data(), static_cast<string::size_type>(remainder.size()),
                                                                                        m_remainder_buffer.data(), m_remainder_buffer.size())};
                               },
                           },
                           views.remainder());
}

inline auto key_handles::section() const noexcept -> const string& {
  return m_section;
}

inline auto key_handles::remainder() const noexcept -> const mixed& {
  return m_remainder;
}

inline auto key_handles::make_section_copy() const noexcept -> string {
  return m_section.copy_and_make_not_shared();
}

inline auto key_handles::make_remainder_copy() const noexcept -> mixed {
  return m_remainder.is_string() ? mixed{m_remainder.as_string().copy_and_make_not_shared()} : m_remainder;
}

} // namespace kphp::confdata
