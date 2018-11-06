#include "compiler/inferring/node.h"

#include "compiler/stage.h"

namespace tinf {

Node::Node() :
  recalc_state_(empty_st),
  holder_id_(-1),
  type_(TypeData::get_type(tp_Unknown)),
  recalc_cnt_(-1),
  isset_flags(0),
  isset_was(0) {
}

void Node::add_edge(Edge *edge) {
  AutoLocker<Node *> locker(this);
  next_.push_back(edge);
}

void Node::add_rev_edge(Edge *edge) {
  AutoLocker<Node *> locker(this);
  rev_next_.push_back(edge);
}

bool Node::try_start_recalc() {
  while (true) {
    int recalc_state_copy = recalc_state_;
    switch (recalc_state_copy) {
      case empty_st:
        if (__sync_bool_compare_and_swap(&recalc_state_, empty_st, own_recalc_st)) {
          recalc_cnt_++;
          holder_id_ = get_thread_id();
          return true;
        }
        break;
      case own_st:
        if (__sync_bool_compare_and_swap(&recalc_state_, own_st, own_recalc_st)) {
          return false;
        }
        break;
      case own_recalc_st:
        return false;
      default:
        assert (0);
    }
  }
  return false;
}

void Node::start_recalc() {
  bool swapped = __sync_bool_compare_and_swap(&recalc_state_, own_recalc_st, own_st);
  kphp_assert(swapped);
}

bool Node::try_finish_recalc() {
  recalc_cnt_++;
  while (true) {
    int recalc_state_copy = recalc_state_;
    switch (recalc_state_copy) {
      case own_st:
        if (__sync_bool_compare_and_swap(&recalc_state_, own_st, empty_st)) {
          holder_id_ = -1;
          return true;
        }
        break;
      case own_recalc_st:
        return false;
      default:
        assert (0);
    }
  }
  return false;
}

}
