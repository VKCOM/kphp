//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-tl-builtins.h"

#include "runtime-light/stdlib/rpc/rpc-state.h"
#include "runtime-light/tl/tl-core.h"

mixed tl_arr_get(const mixed& arr, const string& str_key, int64_t num_key, int64_t precomputed_hash) noexcept {
  auto& cur_query{CurrentTlQuery::get()};
  if (!arr.is_array()) {
    cur_query.raise_storing_error("Array expected, when trying to access field #%" PRIi64 " : %s", num_key, str_key.c_str());
    return {};
  }

  if (const auto& elem{arr.get_value(num_key)}; !elem.is_null()) {
    return elem;
  }
  if (const auto& elem{precomputed_hash == 0 ? arr.get_value(str_key) : arr.get_value(str_key, precomputed_hash)}; !elem.is_null()) {
    return elem;
  }

  cur_query.raise_storing_error("Field %s (#%" PRIi64 ") not found", str_key.c_str(), num_key);
  return {};
}

int32_t t_Int::prepare_int_for_storing(int64_t v) noexcept {
  auto v32{static_cast<int32_t>(v)};
  if (tl::is_int32_overflow(v)) [[unlikely]] {
    if (RpcInstanceState::get().fail_rpc_on_int32_overflow) {
      CurrentTlQuery::get().raise_storing_error("Got int32 overflow with value '%" PRIi64 "'. Serialization will fail.", v);
    } else {
      php_warning("Got int32 overflow on storing %s: the value '%" PRIi64 "' will be casted to '%d'. Serialization will succeed.",
                  CurrentTlQuery::get().get_current_tl_function_name().c_str(), v, v32);
    }
  }
  return v32;
}
