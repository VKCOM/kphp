// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/time-state.h"

#include "runtime-light/state/image-state.h"
#include "runtime-light/state/instance-state.h"

TimeInstanceState& TimeInstanceState::get() noexcept {
  return InstanceState::get().time_instance_state;
}

const TimeImageState& TimeImageState::get() noexcept {
  return ImageState::get().time_image_state;
}

TimeImageState& TimeImageState::get_mutable() noexcept {
  return ImageState::get_mutable().time_image_state;
}
