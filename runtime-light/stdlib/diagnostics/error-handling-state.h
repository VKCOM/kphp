//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

struct ErrorHandlingState final : vk::not_copyable {
  int64_t minimum_log_level{E_ALL};
  const RuntimeContext& runtime_context{RuntimeContext::get()};

  static constexpr std::string_view INI_ERROR_REPORTING_KEY = "error_reporting";
  static constexpr int64_t SUPPORTED_ERROR_LEVELS = E_ERROR | E_WARNING | E_NOTICE;

  ErrorHandlingState() noexcept;

  bool log_level_enabled(int64_t level) const noexcept {
    if (runtime_context.php_disable_warnings > 0 && (level & E_ERROR) == 0) {
      // log level disabled by error control operator @
      return false;
    }
    return (minimum_log_level & level) != 0;
  }

  static ErrorHandlingState& get() noexcept;

  static std::optional<std::reference_wrapper<ErrorHandlingState>> try_get() noexcept;
};
