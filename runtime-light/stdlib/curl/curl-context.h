// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "common/mixin/movable_only.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/curl/defs.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"

namespace kphp::web::curl {

struct curl_context : vk::movable_only {
  int64_t error_code{0};
  std::array<std::byte, kphp::web::curl::CURL_ERROR_SIZE> error_description{std::byte{0}};

  inline auto set_errno(int64_t code, std::string_view description) noexcept {
    // If Web Transfer Lib specific error
    if (code == kphp::web::WEB_INTERNAL_ERROR_CODE) [[unlikely]] {
      return;
    }
    error_code = static_cast<int64_t>(code);
    std::memcpy(error_description.data(), description.data(), std::min(description.size(), static_cast<size_t>(CURL_ERROR_SIZE)));
  }

  inline auto set_errno(int64_t code, std::optional<string> description = std::nullopt) noexcept {
    if (description.has_value()) {
      set_errno(code, (*description).c_str());
      return;
    }
    set_errno(code, "");
  }

  inline auto set_errno(kphp::web::curl::CURLE code, std::string_view description) noexcept {
    set_errno(static_cast<int64_t>(code), description);
  }

  inline auto set_errno(kphp::web::curl::CURLE code, std::optional<string> description = std::nullopt) noexcept {
    set_errno(static_cast<int64_t>(code), std::move(description));
  }

  template<size_t N>
  inline auto bad_option_error(const char (&msg)[N]) noexcept {
    static_assert(N <= CURL_ERROR_SIZE, "too long error");
    kphp::log::warning("{}", msg);
    set_errno(CURLE::BAD_FUNCTION_ARGUMENT, {{msg, N}});
  }

  inline auto errors_reset() noexcept {
    error_code = 0;
    error_description.fill(std::byte{0});
  }
};

struct easy_context : curl_context {
  using curl_context::curl_context;

  bool return_transfer{false};
  std::optional<string> private_data{std::nullopt};
  bool has_been_executed{false}; // Need for providing same as in PHP semantics of curl_getinfo(..., CURLINFO_EFFECTIVE_URL)

  inline auto reset() noexcept {
    curl_context::errors_reset();
    return_transfer = false;
    private_data = std::nullopt;
    has_been_executed = false;
  }
};

struct multi_context : curl_context {
  using curl_context::curl_context;
};

} // namespace kphp::web::curl
