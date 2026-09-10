// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdlib>

#include "runtime-common/core/runtime-core.h"

// These standalone tests have no compiled PHP classes implementing ArrayAccess.
// Keep the runtime-core linkage complete, but fail if a test invokes one.
bool f$ArrayAccess$$offsetExists(const class_instance<C$ArrayAccess>&, const mixed&) noexcept {
  std::abort();
}

mixed f$ArrayAccess$$offsetGet(const class_instance<C$ArrayAccess>&, const mixed&) noexcept {
  std::abort();
}

void f$ArrayAccess$$offsetSet(const class_instance<C$ArrayAccess>&, const mixed&, const mixed&) noexcept {
  std::abort();
}

void f$ArrayAccess$$offsetUnset(const class_instance<C$ArrayAccess>&, const mixed&) noexcept {
  std::abort();
}
