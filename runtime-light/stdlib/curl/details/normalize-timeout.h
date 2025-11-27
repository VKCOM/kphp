// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>

// WARNING: must be synchronized with runtime-light/stdlib/fork/fork-functions.h
namespace kphp::web::curl::details {

inline constexpr double MAX_TIMEOUT = 86400.0;
inline constexpr double DEFAULT_TIMEOUT = MAX_TIMEOUT;

inline constexpr auto MAX_TIMEOUT_NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{MAX_TIMEOUT});
inline constexpr auto DEFAULT_TIMEOUT_NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>{DEFAULT_TIMEOUT});

inline std::chrono::nanoseconds normalize_timeout(std::chrono::nanoseconds timeout) noexcept {
  using namespace std::chrono_literals;
  if (timeout <= 0ns || timeout > MAX_TIMEOUT_NS) {
    return DEFAULT_TIMEOUT_NS;
  }
  return timeout;
}

} // namespace kphp::web::curl::details
