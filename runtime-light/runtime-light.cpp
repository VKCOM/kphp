// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/core/globals/php-init-scripts.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/image-state.h"
#include "runtime-light/state/instance-state.h"

ImageState *k2_create_image() {
  // Note that in k2_create_image most of K2 functionality is not yet available
  php_debug("create image state of \"%s\"", k2::describe()->image_name);
  auto *buffer{static_cast<char *>(k2::alloc(sizeof(ImageState)))};
  if (buffer == nullptr) {
    php_warning("can't allocate enough memory for ImageState");
    return nullptr;
  }
  auto *image_state{new (buffer) ImageState{}};
  php_debug("finish image state creation of \"%s\"", k2::describe()->image_name);
  return image_state;
}

void k2_init_image() {
  php_debug("start image state init");
  init_php_scripts_once_in_master();
  php_debug("end image state init");
}

ComponentState *k2_create_component() {
  return nullptr;
}

void k2_init_component() {}

InstanceState *k2_create_instance() {
  // Note that in k2_create_image most of K2 functionality is not yet available
  php_debug("create instance state of \"%s\"", k2::describe()->image_name);
  auto *buffer{static_cast<char *>(k2::alloc(sizeof(InstanceState)))};
  if (buffer == nullptr) {
    php_warning("cannot allocate enough memory for ComponentState");
    return nullptr;
  }
  auto *instance_state{new (buffer) InstanceState{}};
  php_debug("finish component state creation of \"%s\"", k2::describe()->image_name);
  return instance_state;
}

void k2_init_instance() {
  php_debug("start instance state init");
  k2::instance_state()->init_script_execution();
  php_debug("end instance state init");
}

k2::PollStatus k2_poll() {
  php_debug("k2_poll started...");
  auto &instance_state{InstanceState::get()};
  instance_state.process_platform_updates();
  const auto poll_status{instance_state.poll_status};
  php_debug("k2_poll finished with status: %d", poll_status);
  return poll_status;
}
