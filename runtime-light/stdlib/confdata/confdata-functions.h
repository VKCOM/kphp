// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/task.h"

bool f$is_confdata_loaded() noexcept;

kphp::coro::task<mixed> f$confdata_get_value(string key) noexcept;

kphp::coro::task<array<mixed>> f$confdata_get_values_by_any_wildcard(string wildcard) noexcept;

inline kphp::coro::task<array<mixed>> f$confdata_get_values_by_predefined_wildcard(string wildcard) noexcept {
  php_notice("K2 confdata does not support predefined wildcard optimization");
  co_return(co_await f$confdata_get_values_by_any_wildcard(std::move(wildcard)));
}
