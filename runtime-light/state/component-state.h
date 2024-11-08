// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"

struct ComponentState final : private vk::not_copyable {
  RuntimeAllocator allocator;

  const uint32_t argc;
  mixed runtime_config;
  array<string> ini_opts;

  ComponentState() noexcept
    : allocator(INIT_COMPONENT_ALLOCATOR_SIZE, 0)
    , argc(k2::args_count())
    , ini_opts(array_size{argc, false}) /* overapproximation */ {
    parse_args();
  }

  static const ComponentState &get() noexcept {
    return *k2::component_state();
  }

  static ComponentState &get_mutable() noexcept {
    return *const_cast<ComponentState *>(k2::component_state());
  }

private:
  static constexpr std::string_view INI_ARG_PREFIX_VIEW = "D ";
  static constexpr std::string_view RUNTIME_CONFIG_ARG_VIEW = "runtime-config";
  static constexpr auto INIT_COMPONENT_ALLOCATOR_SIZE = static_cast<size_t>(512U * 1024U); // 512KB

  void parse_args() noexcept;

  void parse_ini_arg(std::string_view, std::string_view) noexcept;

  void parse_runtime_config_arg(std::string_view) noexcept;
};
