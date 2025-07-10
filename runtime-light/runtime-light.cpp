// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <utility>

#include "runtime-light/core/globals/php-init-scripts.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/state/image-state.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/utils/logs.h"

ImageState* k2_create_image() {
  kphp::log::debug("start image state creation");
  auto* image_state_ptr{static_cast<ImageState*>(k2::alloc(sizeof(ImageState)))};
  if (image_state_ptr == nullptr) [[unlikely]] {
    kphp::log::error("can't allocate enough memory for image state");
  }
  kphp::log::debug("finish image state creation");
  return image_state_ptr;
}

void k2_init_image() {
  kphp::log::debug("start image state init");
  new (const_cast<ImageState*>(k2::image_state())) ImageState{};
  init_php_scripts_once_in_master();
  kphp::log::debug("finish image state init");
}

ComponentState* k2_create_component() {
  kphp::log::debug("start component state creation");
  auto* component_state_ptr{static_cast<ComponentState*>(k2::alloc(sizeof(ComponentState)))};
  if (component_state_ptr == nullptr) [[unlikely]] {
    kphp::log::error("can't allocate enough memory for component state");
  }
  kphp::log::debug("finish component state creation");
  return component_state_ptr;
}

void k2_init_component() {
  kphp::log::debug("start component state init");
  new (const_cast<ComponentState*>(k2::component_state())) ComponentState{};
  kphp::log::debug("finish component state init");
}

InstanceState* k2_create_instance() {
  kphp::log::debug("start instance state creation");
  auto* instance_state_ptr{static_cast<InstanceState*>(k2::alloc(sizeof(InstanceState)))};
  if (instance_state_ptr == nullptr) [[unlikely]] {
    kphp::log::error("can't allocate enough memory for instance state");
  }
  kphp::log::debug("finish instance state creation");
  return instance_state_ptr;
}

void k2_init_instance() {
  kphp::log::debug("start instance state init");
  new (k2::instance_state()) InstanceState{};
  k2::instance_state()->init_script_execution();
  kphp::log::debug("finish instance state init");
}

k2::PollStatus k2_poll() {
  kphp::log::debug("k2_poll started");
  const auto poll_status{kphp::coro::io_scheduler::get().process_events()};
  kphp::log::debug("k2_poll finished: {}", std::to_underlying(poll_status));
  return poll_status;
}
