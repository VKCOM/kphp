// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/string/regexp-context.h"

static RegexpContext regexp_context{};

RegexpContext &RegexpContext::get() noexcept {
  return regexp_context;
}
