// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <array>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <utility>

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/utils/logs.h"

namespace {
void php_warning_impl(kphp::log::impl::level level, char const* message, va_list args) noexcept {
  if (std::to_underlying(level) > k2::log_level_enabled()) {
    return;
  }

  constexpr size_t LOG_BUFFER_SIZE = 512;
  std::array<char, LOG_BUFFER_SIZE> log_buffer;
  const auto recorded{std::vsnprintf(log_buffer.data(), log_buffer.size(), message, args)};
  if (recorded <= 0) {
    return;
  }

  kphp::log::impl::write_log(level, "{}", log_buffer.data());
}
} // namespace

void php_debug(char const* message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(kphp::log::impl::level::debug, message, args);
  va_end(args);
}

void php_notice(char const* message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(kphp::log::impl::level::info, message, args);
  va_end(args);
}

void php_warning(char const* message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(kphp::log::impl::level::warn, message, args);
  va_end(args);
}

void php_error(char const* message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(kphp::log::impl::level::error, message, args);
  va_end(args);
}

void runtime_error(char const* message, ...) {
  va_list args;
  va_start(args, message);
  php_warning_impl(false, std::to_underlying(LogLevel::Error), message, args); // TODO: fix error code and think about internal / user errors separation in K2
  va_end(args);
}

void php_assert__(const char* msg, const char* file, int line) {
  php_error("Assertion \"%s\" failed in file %s on line %d", msg, file, line);
  critical_error_handler();
  k2::exit(1);
}

void critical_error_handler() {
  k2::exit(1);
}
