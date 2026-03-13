// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <format>
#include <functional>
#include <optional>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"

namespace kphp::log {

class contextual_tags final : vk::not_copyable {
  using tag_key_t = kphp::stl::string<kphp::memory::script_allocator>;
  using tag_value_t = kphp::stl::string<kphp::memory::script_allocator>;

  struct extra_tags_hash final {
    using is_transparent = void;

    size_t operator()(std::string_view sv) const noexcept {
      return std::hash<std::string_view>{}(sv);
    }

    size_t operator()(const tag_key_t& s) const noexcept {
      return std::hash<tag_key_t>{}(s);
    }
  };

  struct extra_tags_equal final {
    using is_transparent = void;

    constexpr bool operator()(std::string_view lhs, std::string_view rhs) const noexcept {
      return lhs == rhs;
    }

    constexpr bool operator()(const tag_key_t& lhs, const tag_key_t& rhs) const noexcept {
      return lhs == rhs;
    }

    constexpr bool operator()(std::string_view lhs, const tag_key_t& rhs) const noexcept {
      return lhs == rhs;
    }

    constexpr bool operator()(const tag_key_t& lhs, std::string_view rhs) const noexcept {
      return lhs == rhs;
    }
  };

  static constexpr std::string_view BACKTRACE_KEY = "trace";

  kphp::stl::unordered_map<tag_key_t, tag_value_t, kphp::memory::script_allocator, extra_tags_hash, extra_tags_equal> extra_tags;

public:
  size_t size() const noexcept {
    return extra_tags.size();
  }

  auto begin() const noexcept {
    return extra_tags.begin();
  }

  auto end() const noexcept {
    return extra_tags.end();
  }

  void add_extra_tag(std::string_view key, std::string_view value) noexcept;
  void remove_extra_tag(std::string_view key) noexcept;
  void clear_extra_tags() noexcept;

  static std::optional<std::reference_wrapper<contextual_tags>> try_get() noexcept;
};

inline void contextual_tags::clear_extra_tags() noexcept {
  extra_tags.clear();
}

} // namespace kphp::log
