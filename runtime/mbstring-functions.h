// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <assert.h>
#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/mbstring-functions.h"
#include "runtime-common/stdlib/string/string-context.h"

inline string f$mb_strtolower(const string& str, const string& encoding = StringLibConstants::get().CP1251_STR) noexcept {
  return mb_strtolower_impl(
      str, [](bool condition) noexcept { assert(condition); }, encoding);
}

inline string f$mb_strtoupper(const string& str, const string& encoding = StringLibConstants::get().CP1251_STR) noexcept {
  return mb_strtoupper_impl(
      str, [](bool condition) noexcept { assert(condition); }, encoding);
}

inline Optional<int64_t> f$mb_stripos(const string& haystack, const string& needle, int64_t offset = 0,
                                      const string& encoding = StringLibConstants::get().CP1251_STR) noexcept {
  return mb_stripos_impl(
      haystack, needle, [](bool condition) noexcept { assert(condition); }, offset, encoding);
}
