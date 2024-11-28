// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/json-functions.h"
#include "runtime/context/runtime-context.h"
#include "runtime/exception.h"

template<class T>
string f$vk_json_encode_safe(const T &v, bool simple_encode = true) noexcept {
  kphp_runtime_context.static_SB.clean();
  kphp_runtime_context.sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_ON;
  impl_::JsonEncoder(0, simple_encode).encode(v);
  if (unlikely(kphp_runtime_context.sb_lib_context.error_flag == STRING_BUFFER_ERROR_FLAG_FAILED)) {
    kphp_runtime_context.static_SB.clean();
    kphp_runtime_context.sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
    THROW_EXCEPTION(new_Exception(string(__FILE__), __LINE__, string("json_encode buffer overflow", 27)));
    return {};
  }
  kphp_runtime_context.sb_lib_context.error_flag = STRING_BUFFER_ERROR_FLAG_OFF;
  return kphp_runtime_context.static_SB.str();
}
