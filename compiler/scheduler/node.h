#pragma once

class SchedulerBase;
class Task;

class Node {
  SchedulerBase *in_scheduler;
  bool parallel;

public:
  explicit Node(bool parallel = true);
  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;
  Node(const Node&&) = delete;
  Node& operator=(const Node&&) = delete;

  virtual ~Node() = default;

  void add_to_scheduler(SchedulerBase *scheduler);

  void add_to_scheduler_as_sync_node();

  virtual bool is_parallel();

  virtual Task *get_task() = 0;
  virtual void on_finish() = 0;
};
