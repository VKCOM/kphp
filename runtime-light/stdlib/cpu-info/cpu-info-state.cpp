// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/cpu-info/cpu-info-state.h"

#include "runtime-light/state/instance-state.h"

CpuInfoInstanceState& CpuInfoInstanceState::get() noexcept {
  return InstanceState::get().cpu_info_instance_state;
}
