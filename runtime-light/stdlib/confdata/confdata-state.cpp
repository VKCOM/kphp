// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/confdata/confdata-state.h"

#include "runtime-light/state/instance-state.h"

ConfdataInstanceState& ConfdataInstanceState::get() noexcept {
  return InstanceState::get().confdata_instance_state;
}
