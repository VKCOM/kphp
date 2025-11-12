// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"

inline Optional<string> f$setlocale(int64_t category, const string& locale) noexcept {
  const int32_t i32category{static_cast<int32_t>(category)};
  if (k2::uselocale(i32category, {locale.c_str(), locale.size()}) != k2::errno_ok) {
    return false;
  }
  const auto opt_locale_name{k2::current_locale_name(i32category)};
  if (!opt_locale_name.has_value()) [[unlikely]] {
    return false;
  }
  return opt_locale_name->data();
}
