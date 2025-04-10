// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <queue>

#include "compiler/inferring/node.h"
#include "compiler/inferring/restriction-base.h"
#include "compiler/scheduler/task.h"
#include "compiler/threading/tls.h"

namespace tinf {

using NodeQueue = std::queue<Node*>;

class TypeInferer {
private:
  TLS<std::vector<RestrictionBase*>> restrictions;
  bool finish_flag;

public:
  TLS<NodeQueue> Q;

public:
  TypeInferer();

  void add_edge(const Edge* edge);

  void recalc_node(Node* node);
  void add_node(Node* node);

  void add_restriction(RestrictionBase* restriction);
  void check_restrictions();

  void run_queue(NodeQueue* q);
  std::vector<Task*> get_tasks();

  void run_node(Node* node);

  void finish();
  bool is_finished() const {
    return finish_flag;
  }

private:
  void do_run_queue();
};

} // namespace tinf
