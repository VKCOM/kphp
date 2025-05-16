// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/allocator/allocator-state.h"

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/state/image-state.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/utils/logs.h"

const AllocatorState& AllocatorState::get() noexcept {
  if (const auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->instance_allocator_state;
  } else if (const auto* component_state_ptr{k2::component_state()}; component_state_ptr != nullptr) {
    return component_state_ptr->component_allocator_state;
  } else if (const auto* image_state_ptr{k2::image_state()}; image_state_ptr != nullptr) {
    return image_state_ptr->image_allocator_state;
  }
  kphp::log::error("can't find allocator state");
}

AllocatorState& AllocatorState::get_mutable() noexcept {
  return const_cast<AllocatorState&>(AllocatorState::get());
}
