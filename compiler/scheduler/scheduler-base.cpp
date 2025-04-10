// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/scheduler/scheduler-base.h"

#include <cassert>

volatile int tasks_before_sync_node;

static SchedulerBase* scheduler;

void set_scheduler(SchedulerBase* new_scheduler) {
  assert(scheduler == nullptr);
  scheduler = new_scheduler;
}

void unset_scheduler(SchedulerBase* old_scheduler) {
  assert(scheduler == old_scheduler);
  scheduler = nullptr;
}

SchedulerBase* get_scheduler() {
  assert(scheduler != nullptr);
  return scheduler;
}

SchedulerBase::SchedulerBase() {
  set_scheduler(this);
}

SchedulerBase::~SchedulerBase() {
  unset_scheduler(this);
}
