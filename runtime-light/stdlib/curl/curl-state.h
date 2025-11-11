// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/stdlib/curl/defs.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"

#include <array>
#include <cstdint>
#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"

namespace kphp::web::curl {

struct EasyContext {
  bool return_transfer{false};
  std::optional<string> private_data{false};
  int64_t error_code{0};
  std::array<std::byte, kphp::web::curl::CURL_ERROR_SIZE> error_description{std::byte{0}};

  inline auto set_errno(int64_t code, std::string_view description) noexcept {
    // If Web Transfer Lib specific error
    if (code == kphp::web::WEB_INTERNAL_ERROR_CODE) [[unlikely]] {
      return;
    }
    error_code = code;
    std::memcpy(error_description.data(), description.data(), std::min(description.size(), static_cast<size_t>(CURL_ERROR_SIZE)));
  }

  inline auto set_errno(int64_t code, std::optional<string> description = std::nullopt) noexcept {
    if (description.has_value()) {
      set_errno(code, description.value().c_str());
      return;
    }
    set_errno(code, "");
  }

  template<size_t N>
  inline auto bad_option_error(const char (&msg)[N]) noexcept {
    static_assert(N <= CURL_ERROR_SIZE, "Too long error");
    kphp::log::warning("{}", msg);
    set_errno(CURLE::BAD_FUNCTION_ARGUMENT, {{msg, N}});
  }
};

} // namespace kphp::web::curl

struct CurlInstanceState final : private vk::not_copyable {
  kphp::stl::unordered_map<kphp::web::curl::easy_type, kphp::web::curl::EasyContext, kphp::memory::script_allocator> easy2ctx{};

  CurlInstanceState() noexcept = default;

  static CurlInstanceState& get() noexcept;
  inline auto easyctx_get_or_init(kphp::web::curl::easy_type easy_id) noexcept -> kphp::web::curl::EasyContext&;
};

inline auto CurlInstanceState::easyctx_get_or_init(kphp::web::curl::easy_type easy_id) noexcept -> kphp::web::curl::EasyContext& {
  if (!easy2ctx.contains(easy_id)) {
    easy2ctx[easy_id] = kphp::web::curl::EasyContext{};
  }
  return easy2ctx[easy_id];
}
