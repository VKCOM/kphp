// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <queue>
#include <vector>

#include "compiler/scheduler/scheduler-base.h"
#include "compiler/scheduler/task-pull.h"

class ThreadContext;

class Scheduler : public SchedulerBase {
private:
  std::vector<Node *> nodes;
  std::vector<Node *> one_thread_nodes;
  std::queue<Node *> sync_nodes;
  int threads_count;
  TaskPull *task_pull;

  bool thread_process_node(Node *node);
  void thread_execute(ThreadContext *tls);
  friend void *scheduler_thread_execute(void *arg);

public:
  Scheduler();

  void add_node(Node *node) override;
  void add_sync_node(Node *node) override;
  void add_task(Task *task) override;
  void execute() override;

  void set_threads_count(int new_threads_count);
};
