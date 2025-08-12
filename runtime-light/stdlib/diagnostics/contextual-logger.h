// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/detail/logs.h"

namespace kphp::log {

struct contextual_logger {
  using tag_key_t = kphp::stl::string<kphp::memory::script_allocator>;
  using tag_value_t = kphp::stl::string<kphp::memory::script_allocator>;

  template<typename... Args>
  void log(level level, std::optional<std::span<void* const>> trace, std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) const noexcept;

  void add_extra_tag(tag_key_t key, tag_value_t value) noexcept;
  void remove_extra_tag(tag_key_t key) noexcept;
  void clear_extra_tag() noexcept;

  static std::optional<std::reference_wrapper<contextual_logger>> try_get() noexcept;

private:
  kphp::stl::unordered_map<tag_key_t, tag_value_t, kphp::memory::script_allocator> extra_tags{};

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
  auto [out, size]{std::format_to_n<decltype(log_buffer.data()), impl::wrapped_arg_t<Args>...>(log_buffer.data(), log_buffer.size() - 1, fmt,
                                                                                               impl::wrap_log_argument(std::forward<Args>(args))...)};
  *out = '\0';
  auto message{std::string_view{log_buffer.data(), static_cast<std::string_view::size_type>(size)}};
  log_with_tags(level, trace, message);
}

inline void contextual_logger::add_extra_tag(tag_key_t key, tag_key_t value) noexcept {
  extra_tags.insert_or_assign(std::move(key), std::move(value));
}

inline void contextual_logger::remove_extra_tag(tag_key_t key) noexcept {
  extra_tags.erase(std::move(key));
}

inline void contextual_logger::clear_extra_tag() noexcept {
  extra_tags.clear();
}
} // namespace kphp::log
