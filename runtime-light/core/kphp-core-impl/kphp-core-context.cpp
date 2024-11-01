// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/utils/context.h"

constexpr string_size_type initial_minimum_string_buffer_length = 1024;
constexpr string_size_type initial_maximum_string_buffer_length = (1 << 24);

RuntimeContext &RuntimeContext::get() noexcept {
  return get_component_context()->runtime_component_context;
}

void RuntimeContext::init() noexcept {
  init_string_buffer_lib(initial_minimum_string_buffer_length, initial_maximum_string_buffer_length);
}

void RuntimeContext::free() noexcept {}
