// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/stdlib/curl/curl-context.h"
#include "runtime-light/stdlib/curl/defs.h"

struct CurlInstanceState final : private vk::not_copyable {
public:
  CurlInstanceState() noexcept = default;

  static CurlInstanceState& get() noexcept;

  class easy_ctx {
  public:
    inline auto get_or_init(kphp::web::curl::easy_type easy_id) noexcept -> kphp::web::curl::easy_context&;
    inline auto has(kphp::web::curl::easy_type easy_id) noexcept -> bool;

  private:
    kphp::stl::unordered_map<kphp::web::curl::easy_type, kphp::web::curl::easy_context, kphp::memory::script_allocator> ctx{};
  } easy_ctx;

  class multi_ctx {
  public:
    inline auto get_or_init(kphp::web::curl::multi_type multi_id) noexcept -> kphp::web::curl::multi_context&;
    inline auto has(kphp::web::curl::multi_type multi_id) noexcept -> bool;

  private:
    kphp::stl::unordered_map<kphp::web::curl::multi_type, kphp::web::curl::multi_context, kphp::memory::script_allocator> ctx{};
  } multi_ctx;
};

inline auto CurlInstanceState::easy_ctx::get_or_init(kphp::web::curl::easy_type easy_id) noexcept -> kphp::web::curl::easy_context& {
  const auto& it{ctx.find(easy_id)};
  if (it == ctx.end()) {
    return ctx.emplace(easy_id, kphp::web::curl::easy_context{}).first->second;
  }
  return it->second;
}

inline auto CurlInstanceState::easy_ctx::has(kphp::web::curl::easy_type easy_id) noexcept -> bool {
  return ctx.find(easy_id) != ctx.end();
}

inline auto CurlInstanceState::multi_ctx::get_or_init(kphp::web::curl::multi_type multi_id) noexcept -> kphp::web::curl::multi_context& {
  const auto& it{ctx.find(multi_id)};
  if (it == ctx.end()) {
    return ctx.emplace(multi_id, kphp::web::curl::multi_context{}).first->second;
  }
  return it->second;
}

inline auto CurlInstanceState::multi_ctx::has(kphp::web::curl::multi_type multi_id) noexcept -> bool {
  return ctx.find(multi_id) != ctx.end();
}
