// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <type_traits>
#include <utility>
#include <variant>

#include "runtime-light/state/instance-state.h"
#include "runtime-light/server/http/init-functions.h"
#include "runtime-light/server/job-worker/init-functions.h"
#include "runtime-light/tl/tl-functions.h"

using ServerQuery = std::variant<tl::K2InvokeHttp, tl::K2InvokeJobWorker>;

inline void init_server(ServerQuery &&query) noexcept {
  static constexpr auto *SERVER_SOFTWARE_VALUE = "K2/KPHP";
  static constexpr auto *SERVER_SIGNATURE_VALUE = "K2/KPHP Server v0.0.0";

  // common initialization
  {
    using namespace PhpServerSuperGlobalIndices;
    auto &server{InstanceState::get().php_script_mutable_globals_singleton.get_superglobals().v$_SERVER};
    server.set_value(string{SERVER_SOFTWARE, std::char_traits<char>::length(SERVER_SOFTWARE)},
                     string{SERVER_SOFTWARE_VALUE, std::char_traits<char>::length(SERVER_SOFTWARE_VALUE)});
    server.set_value(string{SERVER_SIGNATURE, std::char_traits<char>::length(SERVER_SIGNATURE)},
                     string{SERVER_SIGNATURE_VALUE, std::char_traits<char>::length(SERVER_SIGNATURE_VALUE)});
  }
  // specific initialization
  std::visit(
    [](auto &&query) noexcept {
      using query_t = std::remove_cvref_t<decltype(query)>;

      if constexpr (std::same_as<query_t, tl::K2InvokeHttp>) {
        init_http_server(std::forward<decltype(query)>(query));
      } else if constexpr (std::same_as<query_t, tl::K2InvokeJobWorker>) {
        init_job_server(std::forward<decltype(query)>(query));
      } else {
        static_assert(false, "non-exhaustive visitor!");
      }
    },
    std::move(query));
}
