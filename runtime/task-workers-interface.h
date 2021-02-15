// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime/kphp_core.h"

void init_task_workers_lib();
void free_task_workers_lib();
void global_init_task_workers_lib();

void process_task_worker_answer_event(int ready_task_id, void *task_result_script_memory_ptr);

int64_t f$async_x2(const array<int64_t> &arr);
array<int64_t> f$await_x2(int64_t task_id);
bool f$is_task_workers_enabled();
