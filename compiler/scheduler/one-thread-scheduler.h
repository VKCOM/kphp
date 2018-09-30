#pragma once

#include <queue>
#include <vector>

#include "compiler/scheduler/scheduler-base.h"

class TaskPull;

class OneThreadScheduler : public SchedulerBase {
private:
  std::vector<Node *> nodes;
  std::queue<Node *> sync_nodes;
  TaskPull *task_pull;
public:
  OneThreadScheduler();

  void add_node(Node *node) override;
  void add_sync_node(Node *node) override;
  void add_task(Task *task) override;
  void execute() override;
  void set_threads_count(int threads_count);
};
