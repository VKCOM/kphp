// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/system/system-context.h"

#include "runtime-light/component/component.h"

SystemComponentContext &SystemComponentContext::get() noexcept {
  return get_component_context()->system_instance_state;
}
