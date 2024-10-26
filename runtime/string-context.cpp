// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/string/string-context.h"

StringLibContext &StringLibContext::get() noexcept {
  static StringLibContext string_lib_context;
  return string_lib_context;
}

StringLibConstants &StringLibConstants::get() noexcept {
  static StringLibConstants string_lib_constants;
  return string_lib_constants;
}
