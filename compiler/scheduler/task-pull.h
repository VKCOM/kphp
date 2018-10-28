#pragma once

#include "compiler/scheduler/node.h"
#include "compiler/threading/data-stream.h"

class Task;

class TaskPull : public Node {
private:
  DataStream<Task *> stream;
public:
  inline void add_task(Task *task) {
    stream << task;
  }

  Task *get_task() override {
    Maybe<Task *> x = stream.get();
    if (x.empty()) {
      return nullptr;
    }
    return x;
  }

  void on_finish() override {}
};
