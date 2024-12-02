// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <limits>

inline int64_t f$mt_getrandmax() noexcept {
  // PHP uses this value, but it doesn't forbid the users to pass
  // a number that exceeds this limit into mt_rand()
  return std::numeric_limits<int32_t>::max();
}
