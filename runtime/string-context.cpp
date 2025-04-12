// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/string/string-context.h"

static StringLibContext string_lib_context{};

StringLibContext& StringLibContext::get() noexcept {
  return string_lib_context;
}

const static StringLibConstants string_lib_constants{};

const StringLibConstants& StringLibConstants::get() noexcept {
  return string_lib_constants;
}
