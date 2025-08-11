// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::log {

namespace impl {
template<typename T>
requires std::is_floating_point_v<std::remove_cvref_t<T>>
struct floating_wrapper final {
  T value{};
};

template<typename T>
constexpr auto wrap_log_argument(T&& t) noexcept -> decltype(auto) {
  if constexpr (std::is_floating_point_v<std::remove_cvref_t<T>>) {
    return impl::floating_wrapper<std::remove_cvref_t<T>>{.value = std::forward<T>(t)};
  } else {
    return std::forward<T>(t);
  }
}

template<typename T>
using wrapped_arg_t = std::invoke_result_t<decltype(impl::wrap_log_argument<T>), T>;

} // namespace impl

enum class Level : size_t { error = 1, warn, info, debug, trace }; // NOLINT

struct record {
  Level level;
  std::string_view message;
  std::optional<std::span<void* const>> backtrace;
};

struct logger final : vk::not_copyable {
  using tag_key_t = kphp::stl::string<kphp::memory::script_allocator>;
  using tag_value_t = kphp::stl::string<kphp::memory::script_allocator>;

  static void log(record record) noexcept;

  template<typename... Args>
  static void format_log(Level level, std::optional<std::span<void* const>> trace, std::format_string<impl::wrapped_arg_t<Args>...> fmt,
                         Args&&... args) noexcept;

  void add_extra_tag(tag_key_t key, tag_value_t value) noexcept;
  void remove_extra_tag(tag_key_t key) noexcept;
  void clear_extra_tag() noexcept;

  static std::optional<std::reference_wrapper<logger>> try_get() noexcept;

private:
  kphp::stl::unordered_map<tag_key_t, tag_value_t, kphp::memory::script_allocator> extra_tags{};

  static bool enabled(Level level) noexcept;

  static void log_without_tags(record record) noexcept {
    k2::log(std::to_underlying(record.level), record.message, std::nullopt);
  }

  void log_with_tags(record record) const noexcept;
};

inline void logger::log(kphp::log::record record) noexcept {
  if (!enabled(record.level)) {
    return;
  }

  if (auto logger{logger::try_get()}; logger.has_value()) [[likely]] {
    (*logger).get().log_with_tags(std::move(record));
  } else {
    log_without_tags(std::move(record));
  }
}

template<typename... Args>
void logger::format_log(kphp::log::Level level, std::optional<std::span<void* const>> trace, std::format_string<impl::wrapped_arg_t<Args>...> fmt,
                        Args&&... args) noexcept {
  if (!enabled(level)) {
    return;
  }

  static constexpr size_t LOG_BUFFER_SIZE = 512UZ;
  std::array<char, LOG_BUFFER_SIZE> log_buffer; // NOLINT
  auto [out, size]{std::format_to_n<decltype(log_buffer.data()), impl::wrapped_arg_t<Args>...>(log_buffer.data(), log_buffer.size() - 1, fmt,
                                                                                               impl::wrap_log_argument(std::forward<Args>(args))...)};
  *out = '\0';
  auto message{std::string_view{log_buffer.data(), static_cast<std::string_view::size_type>(size)}};

  log({.level = level, .message = message, .backtrace = trace});
}

inline bool logger::enabled(kphp::log::Level level) noexcept {
  return std::to_underlying(level) <= k2::log_level_enabled();
}

inline void logger::add_extra_tag(tag_key_t key, tag_key_t value) noexcept {
  extra_tags.insert_or_assign(std::move(key), std::move(value));
}

inline void logger::remove_extra_tag(tag_key_t key) noexcept {
  extra_tags.erase(std::move(key));
}

inline void logger::clear_extra_tag() noexcept {
  extra_tags.clear();
}

} // namespace kphp::log
