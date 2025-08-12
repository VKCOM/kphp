//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

struct ErrorHandlingState final : vk::not_copyable {
  int64_t minimum_log_level{E_ALL};

  static constexpr std::string_view INI_LEVEL_KEY = "error_reporting";
  static constexpr int64_t SUPPORTED_ERROR_LEVELS = E_ERROR | E_WARNING | E_NOTICE;

  ErrorHandlingState() noexcept;

  static ErrorHandlingState& get() noexcept;
};
