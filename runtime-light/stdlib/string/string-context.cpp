// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/string-context.h"

#include "runtime-light/component/component.h"

StringComponentContext &StringComponentContext::get() noexcept {
  return get_component_context()->string_component_context;
}
