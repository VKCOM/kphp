// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdint>
#include <random>

inline int64_t f$rand() noexcept {
  return static_cast<int64_t>(std::random_device{}());
}
