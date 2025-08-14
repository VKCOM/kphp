// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <format>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/detail/logs.h"

namespace kphp::log {

class contextual_logger final : vk::not_copyable {
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

public:
  template<typename... Args>
  void log(level level, std::optional<std::span<void* const>> trace, std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) const noexcept;

  void add_extra_tag(std::string_view key, std::string_view value) noexcept;
  void remove_extra_tag(std::string_view key) noexcept;
  void clear_extra_tags() noexcept;

  static std::optional<std::reference_wrapper<contextual_logger>> try_get() noexcept;

private:
  static constexpr std::string_view BACKTRACE_KEY = "trace";

  kphp::stl::unordered_map<tag_key_t, tag_value_t, kphp::memory::script_allocator, extra_tags_hash, extra_tags_equal> extra_tags{};

  void log_with_tags(level level, std::optional<std::span<void* const>> trace, std::string_view message) const noexcept;
};

template<typename... Args>
void contextual_logger::log(level level, std::optional<std::span<void* const>> trace, std::format_string<impl::wrapped_arg_t<Args>...> fmt,
                            Args&&... args) const noexcept {
  if (std::to_underlying(level) > k2::log_level_enabled()) {
    return;
  }

  static constexpr size_t LOG_BUFFER_SIZE = 512UZ;
  std::array<char, LOG_BUFFER_SIZE> log_buffer; // NOLINT
  size_t message_size{impl::format_log_message(log_buffer, fmt, std::forward<Args>(args)...)};
  auto message{std::string_view{log_buffer.data(), static_cast<std::string_view::size_type>(message_size)}};
  log_with_tags(level, trace, message);
}

inline void contextual_logger::add_extra_tag(std::string_view key, std::string_view value) noexcept {
  if (auto it{extra_tags.find(key)}; it == extra_tags.end()) {
    extra_tags.emplace(key, value);
  } else {
    it->second = value;
  }
}

inline void contextual_logger::remove_extra_tag(std::string_view key) noexcept {
  if (auto it{extra_tags.find(key)}; it != extra_tags.end()) {
    extra_tags.erase(it);
  }
}

inline void contextual_logger::clear_extra_tags() noexcept {
  extra_tags.clear();
}
} // namespace kphp::log
