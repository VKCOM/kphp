// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/mixin/not_copyable.h"

namespace php_warning_levels {
constexpr int32_t PHP_WARNING_MINIMUM_LEVEL = 0;
constexpr int32_t DEFAULT_WARNING_LEVEL = 2;
constexpr int32_t PHP_WARNING_MAXIMUM_LEVEL = 3;
} // namespace php_warning_levels

struct ErrorHandlingContext final : private vk::not_copyable {
  int32_t php_warning_level{php_warning_levels::DEFAULT_WARNING_LEVEL};
  int32_t php_warning_minimum_level{};

  static ErrorHandlingContext& get() noexcept;
};
