// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"

namespace kphp::web::curl {
using easy_type = uint64_t;

constexpr auto CURL_ERROR_SIZE = 256;

struct EasyContext {
  bool return_transfer{false};
  string private_data{};
};

} // namespace kphp::web::curl

struct CurlInstanceState final : private vk::not_copyable {
  uint64_t error_code{0};
  std::array<std::byte, kphp::web::curl::CURL_ERROR_SIZE> error_description{std::byte{0}};
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
