// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/json-functions.h"
#include "runtime/exception.h"

template<class T>
string f$vk_json_encode_safe(const T &v, bool simple_encode = true) noexcept {
  auto &ctx = RuntimeContext::get();
  ctx.static_SB.clean();
  ctx.sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_ON;
  impl_::JsonEncoder(0, simple_encode).encode(v, RuntimeContext::get().static_SB);
  if (unlikely(ctx.sb_lib_context.error_flag == STRING_BUFFER_ERROR_FLAG_FAILED)) {
    ctx.static_SB.clean();
    ctx.sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
    THROW_EXCEPTION(new_Exception(string(__FILE__), __LINE__, string("json_encode buffer overflow", 27)));
    return {};
  }
  ctx.sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
  return ctx.static_SB.str();
}
