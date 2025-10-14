// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/allocator/allocator.h"

using curl_easy_t = uint64_t;

constexpr auto CURL_ERROR_SIZE = 256;

struct EasyContext {
  bool return_transfer{false};
  string private_data{};
};

struct CurlInstanceState final : private vk::not_copyable {
  uint64_t error_code{0};
  std::array<std::byte, CURL_ERROR_SIZE> error_description{std::byte{0}};
  kphp::stl::unordered_map<curl_easy_t, EasyContext, kphp::memory::script_allocator> easy2ctx{};

  CurlInstanceState() noexcept = default;

  static CurlInstanceState& get() noexcept;
};

inline auto easyctx_get_or_init(curl_easy_t easy_id) noexcept -> EasyContext& {
  auto& ctx{CurlInstanceState::get()};
  if (!ctx.easy2ctx.contains(easy_id)) {
    ctx.easy2ctx[easy_id] = EasyContext{};
  }
  return ctx.easy2ctx[easy_id];
}
