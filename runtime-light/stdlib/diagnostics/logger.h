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

struct Record {
  Level level;
  std::string_view message;
  std::optional<std::span<void* const>> backtrace;
};

struct Logger final : vk::not_copyable {

  static void log(Record record) noexcept;

  template<typename... Args>
  static void format_log(Level level, std::optional<std::span<void* const>> trace, std::format_string<impl::wrapped_arg_t<Args>...> fmt,
                         Args&&... args) noexcept;

  static bool enabled(Level level) noexcept;

  Logger& set_extra_tags(std::string_view extra_tags_view) noexcept;
  Logger& set_extra_info(std::string_view extra_info_view) noexcept;
  Logger& set_environment(std::string_view environment_view) noexcept;

  static std::optional<std::reference_wrapper<Logger>> try_get() noexcept;

private:
  static void stateless_log(Record record) noexcept {
    k2::log(std::to_underlying(record.level), record.message, std::nullopt);
  }

  void statefull_log(Record record) const noexcept;

  kphp::stl::string<kphp::memory::script_allocator> extra_tags_str;
  kphp::stl::string<kphp::memory::script_allocator> extra_info_str;
  kphp::stl::string<kphp::memory::script_allocator> environment;
};

inline void Logger::log(kphp::log::Record record) noexcept {
  if (!enabled(record.level)) {
    return;
  }

  if (auto logger{Logger::try_get()}; logger.has_value()) [[likely]] {
    (*logger).get().statefull_log(std::move(record));
  } else {
    stateless_log(std::move(record));
  }
}

template<typename... Args>
void Logger::format_log(kphp::log::Level level, std::optional<std::span<void* const>> trace, std::format_string<impl::wrapped_arg_t<Args>...> fmt,
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

inline bool Logger::enabled(kphp::log::Level level) noexcept {
  return std::to_underlying(level) <= k2::log_level_enabled();
}

inline Logger& Logger::set_extra_tags(std::string_view extra_tags_view) noexcept {
  extra_tags_str = extra_tags_view;
  return *this;
}

inline Logger& Logger::set_extra_info(std::string_view extra_info_view) noexcept {
  extra_info_str = extra_info_view;
  return *this;
}

inline Logger& Logger::set_environment(std::string_view environment_view) noexcept {
  environment = environment_view;
  return *this;
}

} // namespace kphp::log
