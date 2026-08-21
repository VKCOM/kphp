// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

struct ComponentState final : private vk::not_copyable {
  AllocatorState m_allocator_state{INIT_COMPONENT_ALLOCATOR_SIZE, DEFAULT_MIN_EXTRA_MEMORY_POOL_SIZE, 0};
  kphp::stl::string<kphp::memory::script_allocator> m_confdata_proxy_actor_name;

private:
  const uint32_t m_argc{k2::args_count()};

public:
  ComponentState() noexcept;
  static auto get() noexcept -> const ComponentState&;
  static auto get_mutable() noexcept -> ComponentState&;

private:
  auto parse_confdata_proxy_actor_name_arg(std::string_view) noexcept -> void;
  auto parse_args() noexcept -> void;

  static constexpr std::string_view CONFDATA_PROXY_ACTOR_NAME_ARG{"confdata-proxy-actor-name"};
  static constexpr auto INIT_COMPONENT_ALLOCATOR_SIZE{static_cast<size_t>(1024U * 1024U)}; // 1MiB
};

inline ComponentState::ComponentState() noexcept {
  parse_args();

  if (m_confdata_proxy_actor_name.empty()) {
    kphp::log::error("{} argument is required", CONFDATA_PROXY_ACTOR_NAME_ARG);
  }
}

inline auto ComponentState::get() noexcept -> const ComponentState& {
  return *k2::component_state();
}

inline auto ComponentState::get_mutable() noexcept -> ComponentState& {
  return const_cast<ComponentState&>(ComponentState::get());
}
