// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/string-functions.h"

#include "runtime-light/component/component.h"

StringComponentState &StringComponentState::get() noexcept {
  return get_component_context()->string_component_state;
}
