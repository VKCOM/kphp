// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/instance-cache/instance-cache-state.h"

#include "runtime-light/state/instance-state.h"

InstanceCacheState& InstanceCacheState::get() noexcept {
  return InstanceState::get().instance_cache_state;
}
