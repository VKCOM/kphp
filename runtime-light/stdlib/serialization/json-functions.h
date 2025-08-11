// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/serialization/json-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

inline void f$set_json_log_on_timeout_mode([[maybe_unused]] bool enabled) noexcept {}

template<class T>
string f$vk_json_encode_safe(const T& v, bool simple_encode = true) noexcept {
  auto& rt_ctx{RuntimeContext::get()};
  rt_ctx.static_SB.clean();
  rt_ctx.sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_ON;
  impl_::JsonEncoder(0, simple_encode).encode(v, RuntimeContext::get().static_SB);
  if (rt_ctx.sb_lib_context.error_flag == STRING_BUFFER_ERROR_FLAG_FAILED) [[unlikely]] {
    rt_ctx.static_SB.clean();
    rt_ctx.sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
    kphp::log::error("vk_json_encode_safe tried to throw exception but it unsupported in runtime light");
    return {};
  }
  rt_ctx.sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
  return rt_ctx.static_SB.str();
}
