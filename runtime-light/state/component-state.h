// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/tagged-logger.h"

struct ComponentState final : private vk::not_copyable {
  AllocatorState component_allocator_state{INIT_COMPONENT_ALLOCATOR_SIZE, 0};
  kphp::log::tagged_logger component_logger;

  const uint32_t argc{k2::args_count()};
  const uint32_t envc{k2::env_count()};
  array<string> ini_opts{array_size{argc, false}};
  array<mixed> env{array_size{envc, false}};
  mixed runtime_config;

  ComponentState() noexcept {
    parse_env();
    parse_args();
  }

  static const ComponentState& get() noexcept {
    return *k2::component_state();
  }

  static ComponentState& get_mutable() noexcept {
    return *const_cast<ComponentState*>(k2::component_state());
  }

private:
  static constexpr std::string_view INI_ARG_PREFIX = "ini ";
  static constexpr std::string_view RUNTIME_CONFIG_ARG = "runtime-config";
  static constexpr auto INIT_COMPONENT_ALLOCATOR_SIZE = static_cast<size_t>(512U * 1024U); // 512KB

  void parse_env() noexcept;

  void parse_args() noexcept;

  void parse_ini_arg(std::string_view, std::string_view) noexcept;

  void parse_runtime_config_arg(std::string_view) noexcept;
};
