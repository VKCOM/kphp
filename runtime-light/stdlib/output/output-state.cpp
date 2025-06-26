// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/output/output-state.h"

#include "runtime-light/state/instance-state.h"

OutputInstanceState& OutputInstanceState::get() noexcept {
  return InstanceState::get().output_instance_state;
}
