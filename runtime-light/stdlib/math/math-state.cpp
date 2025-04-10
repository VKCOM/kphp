// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/math/math-state.h"

#include "runtime-light/state/image-state.h"
#include "runtime-light/state/instance-state.h"

MathInstanceState& MathInstanceState::get() noexcept {
  return InstanceState::get().math_instance_state;
}

const MathImageState& MathImageState::get() noexcept {
  return ImageState::get().math_image_state;
}
