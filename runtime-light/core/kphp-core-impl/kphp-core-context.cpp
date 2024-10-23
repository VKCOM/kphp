// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/runtime-core/runtime-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/utils/context.h"

constexpr string_size_type initial_minimum_string_buffer_length = 1024;
constexpr string_size_type initial_maximum_string_buffer_length = (1 << 24);

KphpCoreContext &KphpCoreContext::current() noexcept {
  return get_component_context()->kphp_core_context;
}

void KphpCoreContext::init() {
  init_string_buffer_lib(initial_minimum_string_buffer_length, initial_maximum_string_buffer_length);
}

void KphpCoreContext::free() {}
