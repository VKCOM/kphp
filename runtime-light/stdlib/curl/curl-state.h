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

  string EASYINFO_URL{kphp::web::curl::details::EASYINFO_URL.data(), kphp::web::curl::details::EASYINFO_URL.size()};
  string EASYINFO_CONTENT_TYPE{kphp::web::curl::details::EASYINFO_CONTENT_TYPE.data(), kphp::web::curl::details::EASYINFO_CONTENT_TYPE.size()};
  string EASYINFO_HTTP_CODE{kphp::web::curl::details::EASYINFO_HTTP_CODE.data(), kphp::web::curl::details::EASYINFO_HTTP_CODE.size()};
  string EASYINFO_HEADER_SIZE{kphp::web::curl::details::EASYINFO_HEADER_SIZE.data(), kphp::web::curl::details::EASYINFO_HEADER_SIZE.size()};
  string EASYINFO_REQUEST_SIZE{kphp::web::curl::details::EASYINFO_REQUEST_SIZE.data(), kphp::web::curl::details::EASYINFO_REQUEST_SIZE.size()};
  string EASYINFO_FILETIME{kphp::web::curl::details::EASYINFO_FILETIME.data(), kphp::web::curl::details::EASYINFO_FILETIME.size()};
  string EASYINFO_REDIRECT_COUNT{kphp::web::curl::details::EASYINFO_REDIRECT_COUNT.data(), kphp::web::curl::details::EASYINFO_REDIRECT_COUNT.size()};
  string EASYINFO_TOTAL_TIME{kphp::web::curl::details::EASYINFO_TOTAL_TIME.data(), kphp::web::curl::details::EASYINFO_TOTAL_TIME.size()};
  string EASYINFO_NAMELOOKUP_TIME{kphp::web::curl::details::EASYINFO_NAMELOOKUP_TIME.data(), kphp::web::curl::details::EASYINFO_NAMELOOKUP_TIME.size()};
  string EASYINFO_CONNECT_TIME{kphp::web::curl::details::EASYINFO_CONNECT_TIME.data(), kphp::web::curl::details::EASYINFO_CONNECT_TIME.size()};
  string EASYINFO_PRETRANSFER_TIME{kphp::web::curl::details::EASYINFO_PRETRANSFER_TIME.data(), kphp::web::curl::details::EASYINFO_PRETRANSFER_TIME.size()};
  string EASYINFO_SIZE_UPLOAD{kphp::web::curl::details::EASYINFO_SIZE_UPLOAD.data(), kphp::web::curl::details::EASYINFO_SIZE_UPLOAD.size()};
  string EASYINFO_SIZE_DOWNLOAD{kphp::web::curl::details::EASYINFO_SIZE_DOWNLOAD.data(), kphp::web::curl::details::EASYINFO_SIZE_DOWNLOAD.size()};
  string EASYINFO_DOWNLOAD_CONTENT_LENGTH{kphp::web::curl::details::EASYINFO_DOWNLOAD_CONTENT_LENGTH.data(),
                                          kphp::web::curl::details::EASYINFO_DOWNLOAD_CONTENT_LENGTH.size()};
  string EASYINFO_STARTTRANSFER_TIME{kphp::web::curl::details::EASYINFO_STARTTRANSFER_TIME.data(),
                                     kphp::web::curl::details::EASYINFO_STARTTRANSFER_TIME.size()};
  string EASYINFO_REDIRECT_TIME{kphp::web::curl::details::EASYINFO_REDIRECT_TIME.data(), kphp::web::curl::details::EASYINFO_REDIRECT_TIME.size()};
  string EASYINFO_REDIRECT_URL{kphp::web::curl::details::EASYINFO_REDIRECT_URL.data(), kphp::web::curl::details::EASYINFO_REDIRECT_URL.size()};
  string EASYINFO_PRIMARY_IP{kphp::web::curl::details::EASYINFO_PRIMARY_IP.data(), kphp::web::curl::details::EASYINFO_PRIMARY_IP.size()};
  string EASYINFO_PRIMARY_PORT{kphp::web::curl::details::EASYINFO_PRIMARY_PORT.data(), kphp::web::curl::details::EASYINFO_PRIMARY_PORT.size()};
  string EASYINFO_LOCAL_IP{kphp::web::curl::details::EASYINFO_LOCAL_IP.data(), kphp::web::curl::details::EASYINFO_LOCAL_IP.size()};
  string EASYINFO_LOCAL_PORT{kphp::web::curl::details::EASYINFO_LOCAL_PORT.data(), kphp::web::curl::details::EASYINFO_LOCAL_PORT.size()};
  string EASYINFO_REQUEST_HEADER{kphp::web::curl::details::EASYINFO_REQUEST_HEADER.data(), kphp::web::curl::details::EASYINFO_REQUEST_HEADER.size()};

  string MULTIINFO_MSG{kphp::web::curl::details::MULTIINFO_MSG.data(), kphp::web::curl::details::MULTIINFO_MSG.size()};
  string MULTIINFO_RESULT{kphp::web::curl::details::MULTIINFO_RESULT.data(), kphp::web::curl::details::MULTIINFO_RESULT.size()};
  string MULTIINFO_HANDLE{kphp::web::curl::details::MULTIINFO_HANDLE.data(), kphp::web::curl::details::MULTIINFO_HANDLE.size()};

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

    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_URL, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_URL, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_CONTENT_TYPE, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_CONTENT_TYPE, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_HTTP_CODE, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_HTTP_CODE, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_HEADER_SIZE, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_HEADER_SIZE, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_REQUEST_SIZE, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_REQUEST_SIZE, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_FILETIME, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_FILETIME, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_REDIRECT_COUNT, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_REDIRECT_COUNT, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_TOTAL_TIME, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_TOTAL_TIME, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_NAMELOOKUP_TIME, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_NAMELOOKUP_TIME, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_CONNECT_TIME, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_CONNECT_TIME, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_PRETRANSFER_TIME, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_PRETRANSFER_TIME, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_SIZE_UPLOAD, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_SIZE_UPLOAD, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_SIZE_DOWNLOAD, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_SIZE_DOWNLOAD, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_DOWNLOAD_CONTENT_LENGTH, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_DOWNLOAD_CONTENT_LENGTH, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_STARTTRANSFER_TIME, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_STARTTRANSFER_TIME, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_REDIRECT_TIME, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_REDIRECT_TIME, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_REDIRECT_URL, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_REDIRECT_URL, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_PRIMARY_IP, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_PRIMARY_IP, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_PRIMARY_PORT, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_PRIMARY_PORT, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_LOCAL_IP, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_LOCAL_IP, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_LOCAL_PORT, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_LOCAL_PORT, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(EASYINFO_REQUEST_HEADER, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(EASYINFO_REQUEST_HEADER, ExtraRefCnt::for_global_const)));

    kphp::log::assertion((kphp::core::set_reference_counter_recursive(MULTIINFO_MSG, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(MULTIINFO_MSG, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(MULTIINFO_RESULT, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(MULTIINFO_RESULT, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(MULTIINFO_HANDLE, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(MULTIINFO_HANDLE, ExtraRefCnt::for_global_const)));
  }

  static const CurlImageState& get() noexcept;
};
