// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/diagnostics/exception-state.h"

#include "runtime-light/state/instance-state.h"

ExceptionInstanceState &ExceptionInstanceState::get() noexcept {
  return InstanceState::get().exception_instance_state;
}
