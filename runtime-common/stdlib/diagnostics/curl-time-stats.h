// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "runtime-common/stdlib/diagnostics/builtin-time-stats.h"

enum class CurlBuiltin : uint8_t {
  curl_init,
  curl_reset,
  curl_setopt,
  curl_setopt_array,
  curl_exec,
  curl_getinfo,
  curl_error,
  curl_errno,
  curl_close,
  curl_exec_concurrently,
  curl_multi_init,
  curl_multi_add_handle,
  curl_multi_getcontent,
  curl_multi_setopt,
  curl_multi_exec,
  curl_multi_select,
  curl_multi_info_read,
  curl_multi_remove_handle,
  curl_multi_errno,
  curl_multi_close,
  curl_multi_strerror,
  count,
};

inline constexpr std::array<std::string_view, static_cast<size_t>(CurlBuiltin::count)> CURL_BUILTIN_NAMES{
    "curl_init",
    "curl_reset",
    "curl_setopt",
    "curl_setopt_array",
    "curl_exec",
    "curl_getinfo",
    "curl_error",
    "curl_errno",
    "curl_close",
    "curl_exec_concurrently",
    "curl_multi_init",
    "curl_multi_add_handle",
    "curl_multi_getcontent",
    "curl_multi_setopt",
    "curl_multi_exec",
    "curl_multi_select",
    "curl_multi_info_read",
    "curl_multi_remove_handle",
    "curl_multi_errno",
    "curl_multi_close",
    "curl_multi_strerror",
};

struct CurlTimeStats final : BuiltinTimeStats<CurlBuiltin, static_cast<size_t>(CurlBuiltin::count)> {
  static CurlTimeStats& get() noexcept;
};
