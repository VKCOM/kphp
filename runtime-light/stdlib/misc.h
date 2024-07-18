// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/superglobals.h"

task_t<void> f$testyield();

void f$check_shutdown();

task_t<void> f$exit(const mixed &v = 0);

void f$die(const mixed &v = 0);

void reset();

task_t<void> parse_input_query(QueryType query_type);
task_t<void> finish(int64_t exit_code, bool from_exit);
