// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/curl/defs.h"
#include "runtime-light/stdlib/curl/details/diagnostics.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/web-transfer-lib/composite-transfer.h"

inline auto f$curl_multi_init() noexcept -> kphp::coro::task<kphp::web::curl::multi_type> {
  auto open_res{co_await kphp::forks::id_managed(kphp::web::composite_transfer_open(kphp::web::transfer_backend::CURL))};
  if (!open_res.has_value()) [[unlikely]] {
    kphp::web::curl::print_error("Could not initialize a new curl multi handle", std::move(open_res.error()));
    co_return 0;
  }
  co_return (*open_res).descriptor;
}

inline Optional<array<int64_t>> f$curl_multi_info_read([[maybe_unused]] int64_t /*unused*/,
                                                       [[maybe_unused]] Optional<std::optional<std::reference_wrapper<int64_t>>> /*unused*/ = {}) {
  kphp::log::error("call to unsupported function : curl_multi_info_read");
}
