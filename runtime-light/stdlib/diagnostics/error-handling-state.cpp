//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/diagnostics/error-handling-state.h"

#include "runtime-light/state/instance-state.h"

ErrorHandlingInstanceState& ErrorHandlingInstanceState::get() noexcept {
  return InstanceState::get().error_handling_instance_state;
}
