// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/core/reference-counter/reference-counter-functions.h"
#include "runtime-light/stdlib/curl/curl-context.h"
#include "runtime-light/stdlib/curl/defs.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

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

struct CurlImageState final : private vk::not_copyable {
  string CURLME_CALL_MULTI_PERFORM{kphp::web::curl::details::CURLME_CALL_MULTI_PERFORM.data(), kphp::web::curl::details::CURLME_CALL_MULTI_PERFORM.size()};
  string CURLME_OK{kphp::web::curl::details::CURLME_OK.data(), kphp::web::curl::details::CURLME_OK.size()};
  string CURLME_BAD_HANDLE{kphp::web::curl::details::CURLME_BAD_HANDLE.data(), kphp::web::curl::details::CURLME_BAD_HANDLE.size()};
  string CURLME_BAD_EASY_HANDLE{kphp::web::curl::details::CURLME_BAD_EASY_HANDLE.data(), kphp::web::curl::details::CURLME_BAD_EASY_HANDLE.size()};
  string CURLME_OUT_OF_MEMORY{kphp::web::curl::details::CURLME_OUT_OF_MEMORY.data(), kphp::web::curl::details::CURLME_OUT_OF_MEMORY.size()};
  string CURLME_INTERNAL_ERROR{kphp::web::curl::details::CURLME_INTERNAL_ERROR.data(), kphp::web::curl::details::CURLME_INTERNAL_ERROR.size()};
  string CURLME_BAD_SOCKET{kphp::web::curl::details::CURLME_BAD_SOCKET.data(), kphp::web::curl::details::CURLME_BAD_SOCKET.size()};
  string CURLME_UNKNOWN_OPTION{kphp::web::curl::details::CURLME_UNKNOWN_OPTION.data(), kphp::web::curl::details::CURLME_UNKNOWN_OPTION.size()};
  string CURLME_ADDED_ALREADY{kphp::web::curl::details::CURLME_ADDED_ALREADY.data(), kphp::web::curl::details::CURLME_ADDED_ALREADY.size()};
  string CURLME_RECURSIVE_API_CALL{kphp::web::curl::details::CURLME_RECURSIVE_API_CALL.data(), kphp::web::curl::details::CURLME_RECURSIVE_API_CALL.size()};
  string CURLME_WAKEUP_FAILURE{kphp::web::curl::details::CURLME_WAKEUP_FAILURE.data(), kphp::web::curl::details::CURLME_WAKEUP_FAILURE.size()};
  string CURLME_BAD_FUNCTION_ARGUMENT{kphp::web::curl::details::CURLME_BAD_FUNCTION_ARGUMENT.data(),
                                      kphp::web::curl::details::CURLME_BAD_FUNCTION_ARGUMENT.size()};
  string CURLME_ABORTED_BY_CALLBACK{kphp::web::curl::details::CURLME_ABORTED_BY_CALLBACK.data(), kphp::web::curl::details::CURLME_ABORTED_BY_CALLBACK.size()};
  string CURLME_UNRECOVERABLE_POLL{kphp::web::curl::details::CURLME_UNRECOVERABLE_POLL.data(), kphp::web::curl::details::CURLME_UNRECOVERABLE_POLL.size()};

  CurlImageState() noexcept {
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_CALL_MULTI_PERFORM, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_CALL_MULTI_PERFORM, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_OK, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_OK, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_BAD_HANDLE, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_BAD_HANDLE, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_BAD_EASY_HANDLE, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_BAD_EASY_HANDLE, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_OUT_OF_MEMORY, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_OUT_OF_MEMORY, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_INTERNAL_ERROR, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_INTERNAL_ERROR, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_BAD_SOCKET, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_BAD_SOCKET, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_UNKNOWN_OPTION, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_UNKNOWN_OPTION, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_ADDED_ALREADY, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_ADDED_ALREADY, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_RECURSIVE_API_CALL, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_RECURSIVE_API_CALL, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_WAKEUP_FAILURE, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_WAKEUP_FAILURE, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_BAD_FUNCTION_ARGUMENT, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_BAD_FUNCTION_ARGUMENT, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_ABORTED_BY_CALLBACK, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_ABORTED_BY_CALLBACK, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(CURLME_UNRECOVERABLE_POLL, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(CURLME_UNRECOVERABLE_POLL, ExtraRefCnt::for_global_const)));
  }

  static const CurlImageState& get() noexcept;
};
