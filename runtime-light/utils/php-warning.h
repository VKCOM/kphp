// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/utils/kphp-assert-core.h"

inline int64_t f$error_reporting([[maybe_unused]] int64_t level) noexcept {
  php_warning("called stub error_reporting");
  return 0;
}

inline int64_t f$error_reporting() noexcept {
  php_warning("called stub error_reporting");
  return 0;
}
