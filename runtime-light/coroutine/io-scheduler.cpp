// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/coroutine/io-scheduler.h"

#include "runtime-light/state/instance-state.h"

namespace kphp::coro {

io_scheduler& io_scheduler::get() noexcept {
  return InstanceState::get().io_scheduler;
}

} // namespace kphp::coro
