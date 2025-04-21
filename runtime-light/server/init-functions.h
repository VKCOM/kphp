// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "runtime-light/server/http/init-functions.h"
#include "runtime-light/server/job-worker/init-functions.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/tl/tl-functions.h"

using ServerQuery = std::variant<tl::K2InvokeHttp, tl::K2InvokeJobWorker>;

inline void init_server(ServerQuery query) noexcept {
  static constexpr std::string_view SERVER_SOFTWARE_VALUE = "K2/KPHP";
  static constexpr std::string_view SERVER_SIGNATURE_VALUE = "K2/KPHP Server v0.0.1";

  // common initialization
  {
    auto& server{InstanceState::get().php_script_mutable_globals_singleton.get_superglobals().v$_SERVER};
    using namespace PhpServerSuperGlobalIndices;
    server.set_value(string{SERVER_SOFTWARE.data(), SERVER_SOFTWARE.size()}, string{SERVER_SOFTWARE_VALUE.data(), SERVER_SOFTWARE_VALUE.size()});
    server.set_value(string{SERVER_SIGNATURE.data(), SERVER_SIGNATURE.size()}, string{SERVER_SIGNATURE_VALUE.data(), SERVER_SIGNATURE_VALUE.size()});
  }
  // specific initialization
  std::visit(
      [](auto&& query) noexcept {
        using query_t = std::remove_cvref_t<decltype(query)>;

        if constexpr (std::same_as<query_t, tl::K2InvokeHttp>) {
          kphp::http::init_server(std::forward<decltype(query)>(query));
        } else if constexpr (std::same_as<query_t, tl::K2InvokeJobWorker>) {
          init_job_server(std::forward<decltype(query)>(query));
        } else {
          static_assert(false, "non-exhaustive visitor!");
        }
      },
      std::move(query));
}
