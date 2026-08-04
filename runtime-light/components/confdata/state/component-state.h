// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-light/k2-platform/k2-api.h"

struct ComponentState final : private vk::not_copyable {
  ComponentState() noexcept = default;
  static auto get() noexcept -> const ComponentState&;
  static auto get_mutable() noexcept -> ComponentState&;
};

inline auto ComponentState::get() noexcept -> const ComponentState& {
  return *k2::component_state();
}

inline auto ComponentState::get_mutable() noexcept -> ComponentState& {
  return const_cast<ComponentState&>(ComponentState::get());
}
