// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <array>
#include <string_view>
#include <utility>

#include "runtime-light/core/globals/php-init-scripts.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/k2-platform/k2-header.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/state/image-state.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/diagnostics/metrics.h"

#define VISIBILITY_DEFAULT __attribute__((visibility("default")))

VISIBILITY_DEFAULT ImageState* k2_create_image() {
  k2::details::image_state_ptr = nullptr;
  k2::details::component_state_ptr = nullptr;
  k2::details::instance_state_ptr = nullptr;
  return static_cast<ImageState*>(k2::alloc_align(sizeof(ImageState), alignof(ImageState)));
}

VISIBILITY_DEFAULT void k2_init_image() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = nullptr;
  k2::details::instance_state_ptr = nullptr;
  new (const_cast<ImageState*>(k2::image_state())) ImageState{};
  init_php_scripts_once_in_master();
}

VISIBILITY_DEFAULT ComponentState* k2_create_component() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = nullptr;
  k2::details::instance_state_ptr = nullptr;
  kphp::log::debug("start component state creation, requested {} bytes", sizeof(ComponentState));
  auto* component_state_ptr{static_cast<ComponentState*>(k2::alloc_align(sizeof(ComponentState), alignof(ComponentState)))};
  kphp::log::debug("finish component state creation");
  return component_state_ptr;
}

VISIBILITY_DEFAULT void k2_init_component() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = k2_component_state();
  k2::details::instance_state_ptr = nullptr;
  kphp::log::debug("start component state init");
  new (const_cast<ComponentState*>(k2::component_state())) ComponentState{};
  kphp::log::debug("finish component state init");
}

VISIBILITY_DEFAULT InstanceState* k2_create_instance() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = k2_component_state();
  k2::details::instance_state_ptr = nullptr;
  kphp::log::debug("start instance state creation, requested {} bytes", sizeof(InstanceState));
  auto* instance_state_ptr{static_cast<InstanceState*>(k2::alloc_align(sizeof(InstanceState), alignof(InstanceState)))};
  kphp::log::debug("finish instance state creation");
  return instance_state_ptr;
}

VISIBILITY_DEFAULT void k2_init_instance() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = k2_component_state();
  k2::details::instance_state_ptr = k2_instance_state();
  kphp::log::debug("start instance state init");
  new (k2::instance_state()) InstanceState{};

  {
    auto guard = CpuInfoInstanceState::write_cycles(CpuInfoInstanceState::get().processing_cycles);
    k2::instance_state()->init_script_execution();
  }

  kphp::log::debug("finish instance state init");
}

VISIBILITY_DEFAULT k2::PollStatus k2_poll() {
  auto& cpu_info_instance_state{CpuInfoInstanceState::get()};

  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = k2_component_state();
  k2::details::instance_state_ptr = k2_instance_state();
  kphp::log::debug("k2_poll started");

  k2::PollStatus poll_status{};
  {
    auto guard = CpuInfoInstanceState::write_cycles(CpuInfoInstanceState::get().processing_cycles);
    poll_status = kphp::coro::io_scheduler::get().process_events();
  }

  if (poll_status == k2::PollStatus::PollFinishedOk) {
    constexpr std::string_view metric_name{"instance_cpu_cycles"};
    constexpr std::string_view tag_name{"kind"};

    auto result{
        kphp::diagnostics::metric::empty().send_value(metric_name, std::array{std::pair{tag_name, "total"}}, cpu_info_instance_state.processing_cycles)};
    if (!result.second.has_value()) {
      kphp::log::warning("failed to send metric {} (kind=total): error {}", metric_name, result.second.error());
    }

    auto send = [&result, metric_name, tag_name](std::string_view tag_value, uint64_t value) noexcept {
      result = kphp::diagnostics::metric::with_buffer(std::move(result.first)).send_value(metric_name, std::array{std::pair{tag_name, tag_value}}, value);
      if (!result.second.has_value()) {
        kphp::log::warning("failed to send metric {} (kind={}): error {}", metric_name, tag_value, result.second.error());
      }
    };

    send("coro_alloc", cpu_info_instance_state.coro_alloc_cycles);
    send("coro_free", cpu_info_instance_state.coro_free_cycles);
  }

  kphp::log::debug("k2_poll finished: {}", std::to_underlying(poll_status));
  return poll_status;
}
