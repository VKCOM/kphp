// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <atomic>

class Node;

class Task;

class SchedulerBase {
public:
  SchedulerBase();
  virtual ~SchedulerBase();
  virtual void add_node(Node *node) = 0;
  virtual void add_sync_node(Node *node) = 0;
  virtual void add_task(Task *task) = 0;
  virtual void execute() = 0;
};

SchedulerBase *get_scheduler();
void set_scheduler(SchedulerBase *new_scheduler);
void unset_scheduler(SchedulerBase *old_scheduler);

extern std::atomic_int tasks_before_sync_node;

inline void register_async_task(Task *task) {
  get_scheduler()->add_task(task);
}
