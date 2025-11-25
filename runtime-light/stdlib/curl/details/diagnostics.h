// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/curl/defs.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/web-transfer-lib/defs.h"

namespace kphp::web::curl {

template<size_t N>
inline auto print_error(const char (&msg)[N], kphp::web::error&& e) noexcept {
  static_assert(N <= CURL_ERROR_SIZE, "too long error");
  if (e.description.has_value()) {
    kphp::log::warning("{}\ncode: {}; description: {}", msg, e.code, (*e.description).c_str());
  } else {
    kphp::log::warning("{}: {}", msg, e.code);
  }
}

} // namespace kphp::web::curl
