// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/scheduler/scheduler.h"

#include <vector>

#include "compiler/scheduler/task.h"
#include "compiler/threading/thread-id.h"
#include "compiler/threading/tls.h"

class ThreadContext {
public:
  pthread_t pthread_id;
  int thread_id;

  class Scheduler *scheduler;

  Node *node;
  bool run_flag;
};


void *scheduler_thread_execute(void *arg) {
  auto *tls = (ThreadContext *)arg;
  tls->scheduler->thread_execute(tls);
  return nullptr;
}

Scheduler::Scheduler() :
  threads_count(-1) {
  task_pull = new TaskPull();
}

void Scheduler::add_node(Node *node) {
  if (node->is_parallel()) {
    nodes.push_back(node);
  } else {
    one_thread_nodes.push_back(node);
  }
}

void Scheduler::add_sync_node(Node *node) {
  sync_nodes.push(node);
}

void Scheduler::add_task(Task *task) {
  assert (task_pull != nullptr);
  task_pull->add_task(task);
}

void Scheduler::execute() {
  task_pull->add_to_scheduler(this);
  set_thread_id(0);
  std::vector<ThreadContext> threads(threads_count + 1);

  assert ((int)one_thread_nodes.size() < threads_count);
  for (int i = 1; i <= threads_count; i++) {
    threads[i].thread_id = i;
    threads[i].scheduler = this;
    threads[i].run_flag = true;
    if (i <= (int)one_thread_nodes.size()) {
      threads[i].node = one_thread_nodes[i - 1];
    }
    pthread_create(&threads[i].pthread_id, nullptr, ::scheduler_thread_execute, &threads[i]);
  }

  while (true) {
    if (tasks_before_sync_node.load(std::memory_order_acquire) > 0) {
      usleep(250);
      continue;
    }
    if (sync_nodes.empty()) {
      break;
    }
    sync_nodes.front()->on_finish();
    sync_nodes.pop();
  }

  for (int i = 1; i <= threads_count; i++) {
    threads[i].run_flag = false;
    __sync_synchronize();
    pthread_join(threads[i].pthread_id, nullptr);
  }

  for (auto *node : nodes) {
    delete node;
  }
}

void Scheduler::set_threads_count(int new_threads_count) {
  assert (1 <= new_threads_count && new_threads_count <= MAX_THREADS_COUNT);
  threads_count = new_threads_count;
}

bool Scheduler::thread_process_node(Node *node) {
  Task *task = node->get_task();
  if (task == nullptr) {
    return false;
  }
  task->execute();
  delete task;
  tasks_before_sync_node.fetch_sub(1, std::memory_order_release);
  return true;
}

void Scheduler::thread_execute(ThreadContext *tls) {
  set_thread_id(tls->thread_id);

  auto process_node = [this](Node *node) {
    bool at_least_one_task_executed = false;
    while (thread_process_node(node)) {
      at_least_one_task_executed = true;
    }
    return at_least_one_task_executed;
  };
  while (tls->run_flag) {
    bool at_least_one_task_executed = false;
    if (tls->node != nullptr) {
      at_least_one_task_executed = process_node(tls->node);
    } else {
      at_least_one_task_executed = std::count_if(nodes.begin(), nodes.end(), process_node) > 0;
    }
    if (!at_least_one_task_executed) {
      usleep(250);
    }
  }
}
