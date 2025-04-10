// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/string-state.h"

#include "runtime-light/state/image-state.h"
#include "runtime-light/state/instance-state.h"

StringInstanceState& StringInstanceState::get() noexcept {
  return InstanceState::get().string_instance_state;
}

const StringImageState& StringImageState::get() noexcept {
  return ImageState::get().string_image_state;
}
