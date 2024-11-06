// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/string-context.h"

#include "runtime-light/component/component.h"
#include "runtime-light/component/image.h"
#include "runtime-light/utils/context.h"

StringComponentContext &StringComponentContext::get() noexcept {
  return get_component_context()->string_instance_state;
}

const StringImageState &StringImageState::get() noexcept {
  return get_image_state()->string_image_state;
}
