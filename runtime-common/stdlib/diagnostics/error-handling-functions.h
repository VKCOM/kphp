// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/diagnostics/error-handling-context.h"

inline int64_t f$error_reporting(int64_t level) noexcept {
  auto& error_handling_st{ErrorHandlingContext::get()};
  const int64_t prev{error_handling_st.php_warning_level};
  if ((level & E_ALL) == E_ALL) {
    error_handling_st.php_warning_level = php_warning_levels::PHP_WARNING_MAXIMUM_LEVEL;
  }
  if (php_warning_levels::PHP_WARNING_MINIMUM_LEVEL <= level && level <= php_warning_levels::PHP_WARNING_MAXIMUM_LEVEL) {
    error_handling_st.php_warning_level = std::max(error_handling_st.php_warning_minimum_level, level);
  }
  return prev;
}

inline int64_t f$error_reporting() noexcept {
  return ErrorHandlingContext::get().php_warning_level;
}
