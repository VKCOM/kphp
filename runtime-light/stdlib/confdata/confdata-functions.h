// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/confdata/confdata-constants.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

inline bool f$is_confdata_loaded() noexcept {
  return k2::component_access(kphp::confdata::COMPONENT_NAME) == k2::errno_ok;
}

// SAFETY: `key` is only used before await point
kphp::coro::task<mixed> f$confdata_get_value(const string& key) noexcept;

// SAFETY: `wildcard` is only used before await point
kphp::coro::task<array<mixed>> f$confdata_get_values_by_any_wildcard(const string& wildcard) noexcept;

// SAFETY: `f$confdata_get_values_by_any_wildcard` is coro-pass-by-ref safe
inline kphp::coro::task<array<mixed>> f$confdata_get_values_by_predefined_wildcard(const string& wildcard) noexcept {
  kphp::log::info("k2-confdata doesn't support predefined wildcard optimization. wildcard: {}", wildcard.c_str());
  co_return co_await f$confdata_get_values_by_any_wildcard(wildcard);
}
