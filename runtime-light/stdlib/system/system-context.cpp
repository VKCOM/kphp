// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/system/system-context.h"

#include "runtime-light/component/component.h"

SystemInstanceState &SystemInstanceState::get() noexcept {
  return InstanceState::get().system_instance_state;
}
