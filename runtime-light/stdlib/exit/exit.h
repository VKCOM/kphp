// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"
#include "runtime-light/coroutine/task.h"

task_t<void> shutdown_script() noexcept;

task_t<void> f$exit(const mixed &v = 0) noexcept;

task_t<void> f$die(const mixed &v = 0) noexcept;
