// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-core/runtime-core.h"
#include "runtime-light/utils/context.h"
#include "runtime-light/component/component.h"

KphpCoreContext &KphpCoreContext::current() noexcept {
  return get_component_context()->kphp_core_context;
}

void KphpCoreContext::init() {
  init_string_buffer_lib(1024, (1 << 24));
}

void KphpCoreContext::free() {
}
