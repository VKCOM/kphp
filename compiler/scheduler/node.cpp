// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/scheduler/node.h"

#include <cassert>

#include "compiler/scheduler/scheduler-base.h"

void Node::add_to_scheduler(SchedulerBase *scheduler) {
  if (in_scheduler == scheduler) {
    return;
  }
  assert(in_scheduler == nullptr);
  in_scheduler = scheduler;
  in_scheduler->add_node(this);
}

void Node::add_to_scheduler_as_sync_node() {
  assert(in_scheduler);
  in_scheduler->add_sync_node(this);
}

bool Node::is_parallel() {
  return parallel;
}

Node::Node(bool parallel) :
  in_scheduler(nullptr),
  parallel(parallel) {}
