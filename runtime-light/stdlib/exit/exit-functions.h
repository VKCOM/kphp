// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"

kphp::coro::task<> f$exit(mixed v = 0) noexcept;

inline kphp::coro::task<> f$die(mixed v = 0) noexcept {
  co_await f$exit(std::move(v));
}
