// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/allocator-wrapper/libc-alloc-registrator.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/state/image-state.h"
#include "runtime-light/state/instance-state.h"

void AllocationsStorage::register_allocation(void* allocation) noexcept {
  if (!m_spans.empty()) {
    m_spans.top().insert(allocation);
  }
}

void AllocationsStorage::unregister_allocation(void* allocation) noexcept {
  if (!m_spans.empty()) {
    m_spans.top().erase(allocation);
  }
}

void AllocationsStorage::push_span() noexcept {
  m_spans.emplace();
}

void AllocationsStorage::pop_span() {
  if (!m_spans.top().empty()) [[unlikely]] {
    kphp::log::error("some memory was not freed");
  }
  m_spans.pop();
}

const AllocationsStorage& AllocationsStorage::get() noexcept {
  if (const auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->instance_allocations_storage;
  } else if (const auto* component_state_ptr{k2::component_state()}; component_state_ptr != nullptr) {
    return component_state_ptr->component_allocations_storage;
  } else if (const auto* image_state_ptr{k2::image_state()}; image_state_ptr != nullptr) {
    return image_state_ptr->image_allocations_storage;
  }
  kphp::log::error("can't find allocator state");
}

AllocationsStorage& AllocationsStorage::get_mutable() noexcept {
  return const_cast<AllocationsStorage&>(AllocationsStorage::get());
}

AllocationsSpan::AllocationsSpan() {
  AllocationsStorage::get_mutable().push_span();
}

AllocationsSpan::~AllocationsSpan() {
  AllocationsStorage::get_mutable().pop_span();
}
