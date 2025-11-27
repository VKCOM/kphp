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

//  Optional<int64_t> f$curl_multi_add_handle(kphp::web::curl::multi_type multi_id, kphp::web::curl::easy_type easy_id) noexcept {
//    if (auto* multi_context = get_context<MultiContext>(multi_id)) {
//      if (auto* easy_context = get_context<EasyContext>(easy_id)) {
//        if (kphp_tracing::is_turned_on()) {
//          string url = easy_context->get_info(CURLINFO_EFFECTIVE_URL).as_string();
//          kphp_tracing::on_curl_multi_add_handle(multi_context->uniq_id, easy_context->uniq_id, url);
//        }
//        easy_context->cleanup_for_next_request();
//        multi_context->error_num = dl::critical_section_call(curl_multi_add_handle, multi_context->multi_handle, easy_context->easy_handle);
//        return multi_context->error_num;
//      }
//    }
//    return false;
//  }

inline Optional<array<int64_t>> f$curl_multi_info_read([[maybe_unused]] int64_t /*unused*/,
                                                       [[maybe_unused]] Optional<std::optional<std::reference_wrapper<int64_t>>> /*unused*/ = {}) {
  kphp::log::error("call to unsupported function : curl_multi_info_read");
}
