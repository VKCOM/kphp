// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::web::details {

inline auto process_error(tl::WebError&& e) noexcept -> error {
  switch (e.code.value) {
  case tl_parse_error_code::server_side:
  case internal_error_code::unknown_backend:
  case internal_error_code::unknown_method:
  case internal_error_code::unknown_transfer:
  case backend_internal_error::cannot_take_handler_ownership:
  case backend_internal_error::post_field_value_not_string:
  case backend_internal_error::header_line_not_string:
  case backend_internal_error::unsupported_property:
    return error{.code = WEB_INTERNAL_ERROR_CODE, .description = string(e.description.value.data(), e.description.value.size())};
  default:
    // BackendError
    return error{.code = e.code.value, .description = string(e.description.value.data(), e.description.value.size())};
  }
}

} // namespace kphp::web::details
