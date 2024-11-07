// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-light/k2-platform/k2-api.h"

struct ComponentState final : private vk::not_copyable {
  RuntimeAllocator allocator;

  ComponentState() noexcept
    : allocator(INIT_COMPONENT_ALLOCATOR_SIZE, 0) {}

  static const ComponentState &get() noexcept {
    return *k2::component_state();
  }

  static ComponentState &get_mutable() noexcept {
    return *const_cast<ComponentState *>(k2::component_state());
  }

private:
  static constexpr auto INIT_COMPONENT_ALLOCATOR_SIZE = static_cast<size_t>(512U * 1024U); // 512KB
};
