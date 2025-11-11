// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/rpc/rpc-parse.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

bool f$rpc_parse(const mixed& new_rpc_data) noexcept {
  if (!new_rpc_data.is_string()) {
    kphp::log::warning("Parameter 1 of function rpc_parse must be a string, %s is given", new_rpc_data.get_type_c_str());
    return false;
  }

  return f$rpc_parse(new_rpc_data.to_string());
}

bool f$rpc_parse(bool new_rpc_data) noexcept {
  return f$rpc_parse(mixed{new_rpc_data});
}

bool f$rpc_parse(const Optional<string>& new_rpc_data) noexcept {
  auto rpc_parse_lambda = [](const auto& v) { return f$rpc_parse(v); };
  return call_fun_on_optional_value(rpc_parse_lambda, new_rpc_data);
}
