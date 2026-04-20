// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "common/php-functions.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/core/reference-counter/reference-counter-functions.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/kml/kml-state.h"

struct ComponentState final : private vk::not_copyable {
  AllocatorState component_allocator_state{INIT_COMPONENT_ALLOCATOR_SIZE, DEFAULT_MIN_EXTRA_MEMORY_POOL_SIZE, 0};
  KmlComponentState kml_component_state; // This member does not hold any KPHP types, so setting a reference counter is unnecessary.

  const uint32_t argc{k2::args_count()};
  const uint32_t envc{k2::env_count()};
  array<string> ini_opts{array_size{argc, false}};
  array<mixed> env{array_size{envc, false}};
  mixed runtime_config;
  string cluster_name{DEFAULT_CLUSTER_NAME.data(), DEFAULT_CLUSTER_NAME.size()};
  bool exit_after_response{};
  uint64_t initial_instance_memory_size{INIT_INSTANCE_ALLOCATOR_SIZE};
  uint64_t min_instance_extra_memory_size{DEFAULT_MIN_EXTRA_MEMORY_POOL_SIZE};

  ComponentState() noexcept {
    parse_env();
    parse_args();

    kphp::log::assertion((kphp::core::set_reference_counter_recursive(ini_opts, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(ini_opts, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(env, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(env, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(runtime_config, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(runtime_config, ExtraRefCnt::for_global_const)));
    kphp::log::assertion((kphp::core::set_reference_counter_recursive(cluster_name, ExtraRefCnt::for_global_const),
                          kphp::core::is_reference_counter_recursive(cluster_name, ExtraRefCnt::for_global_const)));
  }

  static const ComponentState& get() noexcept {
    return *k2::component_state();
  }

  static ComponentState& get_mutable() noexcept {
    return *const_cast<ComponentState*>(k2::component_state());
  }

private:
  static constexpr std::string_view INI_ARG_PREFIX = "ini ";
  static constexpr std::string_view KML_DIR_ARG = "kml-dir";
  static constexpr std::string_view RUNTIME_CONFIG_ARG = "runtime-config";
  static constexpr std::string_view CLUSTER_NAME_ARG = "cluster-name";
  static constexpr std::string_view DEFAULT_CLUSTER_NAME = "default";
  static constexpr std::string_view EXIT_AFTER_RESPONSE_ARG = "exit-after-response";
  static constexpr std::string_view INITIAL_INSTANCE_MEMORY_SIZE_ARG = "initial-instance-memory-size";
  static constexpr std::string_view MIN_INSTANCE_EXTRA_MEMORY_SIZE_ARG = "min-instance-extra-memory-size";
  static constexpr auto INIT_COMPONENT_ALLOCATOR_SIZE = static_cast<size_t>(1024U * 1024U);      // 1MB
  static constexpr auto INIT_INSTANCE_ALLOCATOR_SIZE = static_cast<size_t>(64U * 1024U * 1024U); // 64MB

  void parse_env() noexcept;

  void parse_args() noexcept;

  void parse_ini_arg(std::string_view, std::string_view) noexcept;

  void parse_kml_arg(std::string_view) noexcept;

  void parse_runtime_config_arg(std::string_view) noexcept;

  void parse_cluster_name_arg(std::string_view) noexcept;

  void parse_exit_after_response_arg(std::string_view) noexcept;

  void parse_initial_instance_memory_size_arg(std::string_view) noexcept;

  void parse_min_instance_extra_memory_size_arg(std::string_view) noexcept;
};
