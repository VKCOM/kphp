// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/regex/regex-functions.h"

#include "runtime-light/component/component.h"

RegexInstanceState &RegexInstanceState::get() noexcept {
  return InstanceState::get().regex_instance_state;
}
