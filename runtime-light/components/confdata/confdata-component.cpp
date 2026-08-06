// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <array>
#include <memory>

#include "runtime-light/components/confdata/state/component-state.h"
#include "runtime-light/components/confdata/state/image-state.h"
#include "runtime-light/components/confdata/state/instance-state.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/k2-platform/k2-header.h"
#include "runtime-light/stdlib/confdata/confdata-constants.h"

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
}

VISIBILITY_DEFAULT ComponentState* k2_create_component() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = nullptr;
  k2::details::instance_state_ptr = nullptr;
  return static_cast<ComponentState*>(k2::alloc_align(sizeof(ComponentState), alignof(ComponentState)));
}

VISIBILITY_DEFAULT void k2_init_component() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = k2_component_state();
  k2::details::instance_state_ptr = nullptr;
  new (const_cast<ComponentState*>(k2::component_state())) ComponentState{};
}

VISIBILITY_DEFAULT InstanceState* k2_create_instance() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = k2_component_state();
  k2::details::instance_state_ptr = nullptr;
  return static_cast<InstanceState*>(k2::alloc_align(sizeof(InstanceState), alignof(InstanceState)));
}

VISIBILITY_DEFAULT void k2_init_instance() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = k2_component_state();
  k2::details::instance_state_ptr = k2_instance_state();
  new (k2::instance_state()) InstanceState{};
  k2::instance_state()->init();
}

VISIBILITY_DEFAULT k2::PollStatus k2_warmup() {
  return k2::PollStatus::PollFinishedOk;
}

VISIBILITY_DEFAULT k2::PollStatus k2_poll() {
  k2::details::image_state_ptr = k2_image_state();
  k2::details::component_state_ptr = k2_component_state();
  k2::details::instance_state_ptr = k2_instance_state();
  return kphp::coro::io_scheduler::get().process_events();
}

VISIBILITY_DEFAULT const ImageInfo* k2_describe() {
  static constexpr std::array extra_info{ImageInfo::KeyValuePair{.key = "compiler_version", .value = K2_CONFDATA_COMPILER_VERSION}};
  static constexpr ImageInfo image_info{.image_name = kphp::confdata::COMPONENT_NAME.data(),
                                        .is_oneshot = 0,
                                        .build_timestamp = K2_CONFDATA_BUILD_TIMESTAMP,
                                        .header_h_version = K2_PLATFORM_HEADER_H_VERSION,
                                        .version = "0.0.1",
                                        .extra_info_size = extra_info.size(),
                                        .extra_info = extra_info.data()};
  return std::addressof(image_info);
}
