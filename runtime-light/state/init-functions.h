// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/state/instance-state.h"

// Returns a stream descriptor that is supposed to be a stream to stdout
inline task_t<uint64_t> init_kphp_cli_component() noexcept {
  { // TODO
    auto &superglobals{InstanceState::get().php_script_mutable_globals_singleton.get_superglobals()};
    using namespace PhpServerSuperGlobalIndices;
    superglobals.v$argc = static_cast<int64_t>(0);
    superglobals.v$argv = array<mixed>{};
    superglobals.v$_SERVER.set_value(string{ARGC.data(), ARGC.size()}, superglobals.v$argc);
    superglobals.v$_SERVER.set_value(string{ARGV.data(), ARGV.size()}, superglobals.v$argv);
    superglobals.v$_SERVER.set_value(string{PHP_SELF.data(), PHP_SELF.size()}, string{});
    superglobals.v$_SERVER.set_value(string{SCRIPT_NAME.data(), SCRIPT_NAME.size()}, string{});
  }
  co_return co_await wait_for_incoming_stream_t{};
}

// Performs some initialization and returns a stream descriptor we need to write server response into
task_t<uint64_t> init_kphp_server_component() noexcept;
