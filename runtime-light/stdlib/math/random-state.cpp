// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/math/random-state.h"

#include "runtime-light/state/instance-state.h"

RandomInstanceState& RandomInstanceState::get() noexcept {
  return InstanceState::get().random_instance_state;
}
