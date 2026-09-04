// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

struct ComponentState final : private vk::not_copyable {
  // === MEMBERS ==================================================================================
private:
  static constexpr std::string_view CONFDATA_MEMORY_LIMIT_ARG{"confdata-memory-limit"};
  static constexpr std::string_view CONFDATA_OOM_HANDLING_SIZE_ARG{"confdata-oom-handling-size"};
  static constexpr std::string_view CONFDATA_PROXY_ACTOR_NAME_ARG{"confdata-proxy-actor-name"};
  static constexpr std::string_view PREDEFINED_WILDCARDS_ARG{"predefined-wildcards"};
  static constexpr std::string_view INITIAL_INSTANCE_MEMORY_SIZE_ARG{"initial-instance-memory-size"};
  static constexpr std::string_view MIN_INSTANCE_EXTRA_MEMORY_SIZE_ARG{"min-instance-extra-memory-size"};
  static constexpr auto INIT_COMPONENT_ALLOCATOR_SIZE{static_cast<size_t>(1024U * 1024U)};              // 1MiB
  static constexpr auto DEFAULT_INIT_INSTANCE_ALLOCATOR_SIZE{static_cast<size_t>(64U * 1024U * 1024U)}; // 64MiB
  static constexpr auto DEFAULT_MIN_INSTANCE_EXTRA_MEMORY_SIZE{64U * 1024U * 1024U};                    // 64MiB
  /** Leaves the same five-percent runway between legacy KPHP's hard OOM threshold and its memory limit. */
  static constexpr size_t DEFAULT_OOM_HANDLING_SIZE_DIVISOR{20};

public:
  AllocatorState m_allocator_state{INIT_COMPONENT_ALLOCATOR_SIZE, DEFAULT_MIN_EXTRA_MEMORY_POOL_SIZE, 0};

private:
  const uint32_t m_argc{k2::args_count()};
  // Owns the immutable multiline argument referenced by m_predefined_wildcards.
  kphp::stl::string<kphp::memory::script_allocator> m_predefined_wildcards_storage;

public:
  /** Total allocator payload available in each shared-memory piece. */
  size_t m_confdata_memory_limit{};
  /** Final allocatable part of the payload reserved for completing one in-flight operation safely. */
  size_t m_confdata_oom_handling_size{};
  kphp::stl::string<kphp::memory::script_allocator> m_confdata_proxy_actor_name;
  kphp::stl::vector<std::string_view, kphp::memory::script_allocator> m_predefined_wildcards;
  size_t m_initial_instance_memory_size{DEFAULT_INIT_INSTANCE_ALLOCATOR_SIZE};
  size_t m_min_instance_extra_memory_size{DEFAULT_MIN_INSTANCE_EXTRA_MEMORY_SIZE};

  // === METHODS ==================================================================================
  ComponentState() noexcept;
  static auto get() noexcept -> const ComponentState&;
  static auto get_mutable() noexcept -> ComponentState&;

private:
  auto parse_confdata_memory_limit_arg(std::string_view) noexcept -> void;
  auto parse_confdata_oom_handling_size_arg(std::string_view) noexcept -> void;
  auto parse_confdata_proxy_actor_name_arg(std::string_view) noexcept -> void;
  auto parse_predefined_wildcards_arg(std::string_view) noexcept -> void;
  auto parse_initial_instance_memory_size_arg(std::string_view) noexcept -> void;
  auto parse_min_instance_extra_memory_size_arg(std::string_view) noexcept -> void;
  auto parse_args() noexcept -> void;
};

inline ComponentState::ComponentState() noexcept {
  parse_args();

  if (m_confdata_memory_limit == 0) {
    kphp::log::error("{} argument is required and must be a positive number", CONFDATA_MEMORY_LIMIT_ARG);
  }
  if (m_confdata_oom_handling_size == 0) {
    m_confdata_oom_handling_size = m_confdata_memory_limit / DEFAULT_OOM_HANDLING_SIZE_DIVISOR;
    if (m_confdata_oom_handling_size == 0) {
      m_confdata_oom_handling_size = 1;
    }
  }
  if (m_confdata_oom_handling_size >= m_confdata_memory_limit) {
    kphp::log::error("{} must be smaller than {}", CONFDATA_OOM_HANDLING_SIZE_ARG, CONFDATA_MEMORY_LIMIT_ARG);
  }
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
