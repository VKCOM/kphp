// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

void process_task_worker_answer_event(int ready_task_id, int task_result);

int64_t f$async_x2(int64_t x);
int64_t f$await_x2(int64_t task_id);
