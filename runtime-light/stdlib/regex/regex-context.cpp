// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/regex/regex-functions.h"

#include "runtime-light/component/component.h"

RegexComponentContext &RegexComponentContext::get() noexcept {
  return get_component_context()->regex_component_context;
}
