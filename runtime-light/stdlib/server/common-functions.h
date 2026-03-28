// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/stdlib/system/system-functions.h"

inline auto f$ignore_user_abort(Optional<bool> enable) noexcept -> kphp::coro::task<int64_t> {
  if (InstanceState::get().instance_kind() != instance_kind::http_server) {
    kphp::log::error("called stub f$ignore_user_abort");
  }

  auto& http_server_instance_st{HttpServerInstanceState::get()};
  kphp::log::assertion(http_server_instance_st.connection.has_value());
  if (enable.is_null()) {
    co_return http_server_instance_st.connection->get_ignore_abort_level();
  } else if (enable.val()) {
    http_server_instance_st.connection->increase_ignore_abort_level();
    co_return http_server_instance_st.connection->get_ignore_abort_level();
  } else {
    const auto prev{http_server_instance_st.connection->get_ignore_abort_level()};
    http_server_instance_st.connection->decrease_ignore_abort_level();

    if (http_server_instance_st.connection->get_ignore_abort_level() == 0 && http_server_instance_st.connection->is_aborted()) {
      co_await kphp::forks::id_managed(kphp::system::exit(1));
    }
    co_return prev;
  }
}
