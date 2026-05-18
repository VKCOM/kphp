// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <utility>

#include "runtime-light/core/globals/php-init-scripts.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/k2-platform/k2-header.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/state/image-state.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

#define VISIBILITY_DEFAULT __attribute__((visibility("default")))

VISIBILITY_DEFAULT ImageState* k2_create_image() {
  return static_cast<ImageState*>(k2::alloc_align(sizeof(ImageState), alignof(ImageState)));
}

VISIBILITY_DEFAULT void k2_init_image() {
  k2::details::image_state_ptr = k2_image_state();
  new (const_cast<ImageState*>(k2::image_state())) ImageState{};
  init_php_scripts_once_in_master();
}

VISIBILITY_DEFAULT ComponentState* k2_create_component() {
  k2::details::image_state_ptr = k2_image_state();
  kphp::log::debug("start component state creation, requested {} bytes", sizeof(ComponentState));
  auto* component_state_ptr{static_cast<ComponentState*>(k2::alloc_align(sizeof(ComponentState), alignof(ComponentState)))};
  kphp::log::debug("finish component state creation");
  return component_state_ptr;
}

VISIBILITY_DEFAULT void k2_init_component() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = k2_component_state();
  kphp::log::debug("start component state init");
  new (const_cast<ComponentState*>(k2::component_state())) ComponentState{};
  kphp::log::debug("finish component state init");
}

VISIBILITY_DEFAULT InstanceState* k2_create_instance() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = k2_component_state();
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
  k2::instance_state()->init_script_execution();
  kphp::log::debug("finish instance state init");
}

VISIBILITY_DEFAULT k2::PollStatus k2_poll() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = k2_component_state();
  k2::details::instance_state_ptr = k2_instance_state();
  kphp::log::debug("k2_poll started");
  const auto poll_status{kphp::coro::io_scheduler::get().process_events()};
  kphp::log::debug("k2_poll finished: {}", std::to_underlying(poll_status));
  return poll_status;
}
