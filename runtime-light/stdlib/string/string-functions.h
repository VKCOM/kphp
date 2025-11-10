// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/core-types/decl/optional.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-header.h"
#include <cstdint>

Optional<string> f$setlocale(int64_t category, const string& locale) noexcept {
  const int32_t i32category = static_cast<int32_t>(category);
  return k2_uselocale(i32category, locale.c_str()) ? false : k2_current_locale_name(i32category);
}
