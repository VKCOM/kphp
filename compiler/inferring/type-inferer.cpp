#include "compiler/inferring/type-inferer.h"

#include "compiler/threading/profiler.h"

namespace tinf {

TypeInferer::TypeInferer() :
  finish_flag(false) {
}

void TypeInferer::recalc_node(Node *node) {
  //fprintf (stderr, "tinf::recalc_node %d %p %s\n", get_thread_id(), node, node->get_description().c_str());
  if (node->try_start_recalc()) {
    Q->push(node);
  }
}

bool TypeInferer::add_node(Node *node) {
  //fprintf (stderr, "tinf::add_node %d %p %s\n", get_thread_id(), node, node->get_description().c_str());
  if (node->get_recalc_cnt() < 0) {
    recalc_node(node);
    return true;
  }
  return false;
}

void TypeInferer::add_edge(Edge *edge) {
  assert(edge != nullptr);
  assert(edge->from != nullptr);
  assert(edge->to != nullptr);
  //fprintf (stderr, "add_edge %d [%p %s] -> [%p %s]\n", get_thread_id(), edge->from, edge->from->get_description().c_str(), edge->to, edge->to->get_description().c_str());
  edge->from->add_edge(edge);
  edge->to->add_rev_edge(edge);
}

void TypeInferer::add_restriction(RestrictionBase *restriction) {
  restrictions->push_back(restriction);
}

void TypeInferer::check_restrictions() {
  for (int i = 0; i < restrictions.size(); i++) {
    for (RestrictionBase *r : *restrictions.get(i)) {
      if (r->check_broken_restriction()) {
        return;
      }
    }
  }
}


class TypeInfererTask : public Task {
  static CachedProfiler type_inferer_profiler;
private:
  TypeInferer *inferer_;
  NodeQueue *queue_;
public:
  TypeInfererTask(TypeInferer *inferer, NodeQueue *queue) :
    inferer_(inferer),
    queue_(queue) {
  }

  void execute() {
    AutoProfiler profiler{*type_inferer_profiler};
    stage::set_name("Infer types");
    stage::set_function(FunctionPtr());
    inferer_->run_queue(queue_);
    delete queue_;
  }
};

CachedProfiler TypeInfererTask::type_inferer_profiler{"type_inferring"};

vector<Task *> TypeInferer::get_tasks() {
  vector<Task *> res;
  for (int i = 0; i < Q.size(); i++) {
    NodeQueue *q = Q.get(i);
    if (q->empty()) {
      continue;
    }
    NodeQueue *new_q = new NodeQueue();
    swap(*new_q, *q);

    res.push_back(new TypeInfererTask(this, new_q));
  }
  return res;
}

int TypeInferer::do_run_queue() {
  NodeQueue &q = *Q;

  int cnt = 0;
  while (!q.empty()) {
    cnt++;
    Node *node = q.front();

    node->start_recalc();
    node->recalc(this);

    if (node->try_finish_recalc()) {
      q.pop();
    }
  }

  return cnt;
}

int TypeInferer::run_queue(NodeQueue *new_q) {
  swap(*Q, *new_q);
  return do_run_queue();
}

void TypeInferer::run_node(Node *node) {
  if (add_node(node)) {
    do_run_queue();
  }
  while (node->get_recalc_cnt() == 0) {
    usleep(250);
  }
}

void TypeInferer::finish() {
  finish_flag = true;
}

bool TypeInferer::is_finished() {
  return finish_flag;
}



}

