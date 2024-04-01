// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/scheduler/one-thread-scheduler.h"

#include <cassert>

#include "compiler/scheduler/task-pull.h"
#include "compiler/scheduler/task.h"

OneThreadScheduler::OneThreadScheduler() {
  task_pull = new TaskPull();
}

void OneThreadScheduler::add_node(Node *node) {
  nodes.push_back(node);
}

void OneThreadScheduler::add_sync_node(Node *node) {
  sync_nodes.push(node);
}

void OneThreadScheduler::add_task(Task *task) {
  assert(task_pull != nullptr);
  task_pull->add_task(task);
}

void OneThreadScheduler::set_threads_count(int new_threads_count __attribute__((unused))) {
  fprintf(stderr, "Using scheduler whithout threads, thread count is ignored");
}

void OneThreadScheduler::execute() {
  task_pull->add_to_scheduler(this);
  bool run_flag = true;
  while (run_flag) {
    run_flag = false;
    for (auto *node : nodes) {
      Task *task;
      while ((task = node->get_task()) != nullptr) {
        run_flag = true;
        task->execute();
        delete task;
      }
    }
    if (!run_flag && !sync_nodes.empty()) {
      sync_nodes.front()->on_finish();
      sync_nodes.pop();
      run_flag = true;
    }
  }
  for (auto *node : nodes) {
    delete node;
  }
}
