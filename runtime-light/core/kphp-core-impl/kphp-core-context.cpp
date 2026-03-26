// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace {

constexpr string_size_type initial_minimum_string_buffer_length = 1024;
constexpr string_size_type initial_maximum_string_buffer_length = (1 << 24);

} // namespace

// FIXME codegen for constants initialization should not access RuntimeContext by mutable reference
RuntimeContext& RuntimeContext::get() noexcept {
  if (auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->runtime_context;
  } else if (const auto* component_state_ptr{k2::component_state()}; component_state_ptr != nullptr) [[unlikely]] {
    kphp::log::error("unexpected access to RuntimeContext");
  } else if (const auto* image_state_ptr{k2::image_state()}; image_state_ptr != nullptr) [[likely]] {
    return const_cast<RuntimeContext&>(image_state_ptr->runtime_context);
  }
  kphp::log::error("can't find suitable RuntimeContext");
}

void RuntimeContext::init() noexcept {
  init_string_buffer_lib(initial_minimum_string_buffer_length, initial_maximum_string_buffer_length);
}

void RuntimeContext::free() noexcept {}
