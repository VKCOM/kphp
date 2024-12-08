// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"

task_t<void> f$exit(const mixed &v = 0) noexcept;

inline task_t<void> f$die(const mixed &v = 0) noexcept {
  co_await f$exit(v);
}
