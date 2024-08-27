// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdint>

#include "runtime-core/runtime-core.h"

inline int64_t f$strlen(const string &s) noexcept {
  return s.size();
}
