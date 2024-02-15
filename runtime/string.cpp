// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/kphp_core.h"

// Don't move this destructor to the headers, it spoils addr2line traces
string::~string() noexcept {
  destroy();
}

static_assert(sizeof(string) == SIZEOF_STRING, "sizeof(string) at runtime doesn't match compile-time");
