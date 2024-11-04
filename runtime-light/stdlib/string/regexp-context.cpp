// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/regexp-context.h"

#include "runtime-light/component/image.h"
#include "runtime-light/utils/context.h"

const RegexpImageState &RegexpImageState::get() noexcept {
  return get_image_state()->regexp_image_state;
}

RegexpImageState &RegexpImageState::get_mutable() noexcept {
  return get_mutable_image_state()->regexp_image_state;
}
