#pragma once


#include "compiler/inferring/node.h"
#include "compiler/inferring/restriction-base.h"
#include "compiler/scheduler/task.h"

namespace tinf {

typedef queue<Node *> NodeQueue;

class TypeInferer {
private:
  TLS<vector<RestrictionBase *>> restrictions;
  bool finish_flag;

public:
  TLS<NodeQueue> Q;

public:
  TypeInferer();

  void add_edge(Edge *edge);

  void recalc_node(Node *node);
  bool add_node(Node *node);

  void add_restriction(RestrictionBase *restriction);
  void check_restrictions();

  int run_queue(NodeQueue *q);
  vector<Task *> get_tasks();

  void run_node(Node *node);

  void finish();
  bool is_finished();

private:
  int do_run_queue();
};

}
