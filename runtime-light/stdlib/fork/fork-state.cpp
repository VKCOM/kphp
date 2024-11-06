// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/fork/fork-state.h"

#include "runtime-light/state/instance-state.h"

ForkInstanceState &ForkInstanceState::get() noexcept {
  return InstanceState::get().fork_instance_state;
}
