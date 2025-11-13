// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/stdlib/curl/curl.h"
#include "runtime-light/stdlib/curl/defs.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"

#include <array>
#include <cstdint>
#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"

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
