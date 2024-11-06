// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/component/image.h"
#include "runtime-light/core/globals/php-init-scripts.h"
#include "runtime-light/utils/context.h"

ImageState *vk_k2_create_image_state(const struct PlatformCtx *pt_ctx) {
  // Note that in vk_k2_create_image_state available only allocator and logs from pt_ctx
  platformCtx = pt_ctx;
  php_debug("create image state on \"%s\"", vk_k2_describe()->image_name);
  char *buffer = static_cast<char *>(platformCtx->allocator.alloc(sizeof(ImageState)));
  if (buffer == nullptr) {
    php_warning("cannot allocate enough memory for ImageState");
    return nullptr;
  }
  mutableImageState = new (buffer) ImageState();
  imageState = mutableImageState;
  init_php_scripts_once_in_master();
  ImageState *mutable_image_state = mutableImageState;
  php_debug("finish image state creation on \"%s\"", vk_k2_describe()->image_name);
  reset_thread_locals();
  return mutable_image_state;
}

InstanceState *vk_k2_create_component_state(const struct ImageState *image_state, const struct PlatformCtx *pt_ctx) {
  // Note that in vk_k2_create_component_state available only allocator and logs from pt_ctx
  imageState = image_state;
  platformCtx = pt_ctx;
  php_debug("create component state on \"%s\"", vk_k2_describe()->image_name);
  char *buffer = static_cast<char *>(platformCtx->allocator.alloc(sizeof(InstanceState)));
  if (buffer == nullptr) {
    php_warning("cannot allocate enough memory for ComponentState");
    return nullptr;
  }
  componentState = new (buffer) InstanceState();
  componentState->init_script_execution();
  InstanceState *component_state = componentState;
  php_debug("finish component state creation on \"%s\"", vk_k2_describe()->image_name);
  reset_thread_locals();
  return component_state;
}

PollStatus vk_k2_poll(const ImageState *image_state, const PlatformCtx *pt_ctx, InstanceState *component_ctx) {
  imageState = image_state;
  platformCtx = pt_ctx;
  componentState = component_ctx;

  php_debug("vk_k2_poll started...");
  componentState->process_platform_updates();
  const auto poll_status = componentState->poll_status;
  php_debug("vk_k2_poll finished with status: %d", poll_status);
  reset_thread_locals();
  return poll_status;
}
