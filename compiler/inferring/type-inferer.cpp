// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/type-inferer.h"

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-file.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/restriction-match-phpdoc.h"
#include "compiler/threading/profiler.h"

namespace tinf {

TypeInferer::TypeInferer()
  : finish_flag(false) {}

void TypeInferer::recalc_node(Node *node) {
  //fprintf (stderr, "tinf::recalc_node %d %p %s\n", get_thread_id(), node, node->get_description().c_str());
  if (node->try_start_recalc()) {
    Q->push(node);
  }
}

void TypeInferer::add_node(Node *node) {
  // fprintf (stderr, "tinf::add_node %d %p %s\n", get_thread_id(), node, node->get_description().c_str());
  if (!node->was_recalc_started_at_least_once()) {
    recalc_node(node);
  }
}

void TypeInferer::add_edge(const Edge *edge) {
  //fprintf (stderr, "add_edge %d [%p %s] -> [%p %s]\n", get_thread_id(), edge->from, edge->from->get_description().c_str(), edge->to, edge->to->get_description().c_str());
  edge->from->register_edge_from_this(edge);
  edge->to->register_edge_to_this(edge);
}

void TypeInferer::add_restriction(RestrictionBase *restriction) {
  restrictions->push_back(restriction);
}

void TypeInferer::check_restrictions() {
  int limit = G->settings().show_all_type_errors.get() ? 200 : 1;
  int shown = 0;

  for (int i = 0; i < restrictions.size(); i++) {
    for (RestrictionBase *r : restrictions.get(i)) {
      if (r->is_restriction_broken()) {
        std::string description = r->get_description();
        stage::set_location(r->get_location()); // after get_description(), as its call could change location
        kphp_error(0, description);

        if (++shown >= limit) {
          return;
        }
      }
    }
  }
}


class TypeInfererTask : public Task {
  static CachedProfiler type_inferer_profiler;
private:
  TypeInferer *inferer_;
  NodeQueue queue_;
public:
  TypeInfererTask(TypeInferer *inferer, NodeQueue &&queue) :
    inferer_(inferer),
    queue_(std::move(queue)) {
  }

  void execute() override {
    AutoProfiler profiler{*type_inferer_profiler};
    stage::set_name("Infer types");
    stage::set_function(FunctionPtr());
    inferer_->run_queue(&queue_);
  }
};

CachedProfiler TypeInfererTask::type_inferer_profiler{"Type Inferring"};

std::vector<Task *> TypeInferer::get_tasks() {
  std::vector<Task *> res;
  for (int i = 0; i < Q.size(); i++) {
    NodeQueue q = std::move(Q.get(i));
    if (q.empty()) {
      continue;
    }

    res.push_back(new TypeInfererTask(this, std::move(q)));
  }
  return res;
}

void TypeInferer::do_run_queue() {
  NodeQueue &q = Q.get();

  while (!q.empty()) {
    Node *node = q.front();

    node->start_recalc();
    node->recalc(this);
    if (node->try_finish_recalc()) {
      q.pop();
    }
  }
}

void TypeInferer::run_queue(NodeQueue *new_q) {
  *Q = std::move(*new_q);
  do_run_queue();
}

void TypeInferer::run_node(Node *node) {
  if (!node->was_recalc_started_at_least_once()) {
    add_node(node);
    do_run_queue();
  }
  while (!node->was_recalc_finished_at_least_once()) {
    usleep(250);
  }
}

void TypeInferer::finish() {
  finish_flag = true;
  kphp_assert(Q->empty());
}



} // namespace tinf

