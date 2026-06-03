// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/node.h"

#include "compiler/inferring/type-data.h"
#include <atomic>

namespace tinf {

std::string Node::as_human_readable() const {
  return type_.load(std::memory_order_relaxed)->as_human_readable(false);
}

void Node::register_edge_from_this(const tinf::Edge *edge) {
  AutoLocker<Node *> locker(this);
  edges_from_this_.emplace_front(edge);
}

void Node::register_edge_to_this(const tinf::Edge *edge) {
  AutoLocker<Node *> locker(this);
  edges_to_this_.emplace_front(edge);
}

bool Node::try_start_recalc() {
  while (true) {
    int recalc_state_copy = recalc_state_;
    int once_finished_flag = recalc_state_copy & recalc_bit_at_least_once;
    switch (recalc_state_copy & 15) {   // preserve bit 16 in transformations
      case recalc_st_waiting: {
        int old = recalc_st_waiting | once_finished_flag;
        if (recalc_state_.compare_exchange_strong(old, recalc_st_need_relaunch | once_finished_flag)) {
          return true;
        }
        break;
      }
      case recalc_st_processing: {
        int old = recalc_st_processing | once_finished_flag;
        if (recalc_state_.compare_exchange_strong(old, recalc_st_need_relaunch | once_finished_flag)) {
          return false;
        }
        break;
      }
      case recalc_st_need_relaunch:
        return false;
      default:
        assert (0);
    }
  }
  return false;
}

void Node::start_recalc() {
  int once_finished_flag = recalc_state_ & recalc_bit_at_least_once;  // preserve bit 16 in transformation
  int old = recalc_st_need_relaunch | once_finished_flag;
  bool swapped = recalc_state_.compare_exchange_strong(old, recalc_st_processing | once_finished_flag);
  kphp_assert(swapped);
}

bool Node::try_finish_recalc() {
  while (true) {
    int recalc_state_copy = recalc_state_;
    switch (recalc_state_copy) {  // always set bit 16 in transformations
      case recalc_st_processing: {
        int old = recalc_st_processing;
        if (recalc_state_.compare_exchange_strong(old, recalc_st_waiting | recalc_bit_at_least_once)) {
          return true;
        }
        break;
      }
      case recalc_st_processing | recalc_bit_at_least_once: {
        int old = recalc_st_processing | recalc_bit_at_least_once;
        if (recalc_state_.compare_exchange_strong(old, recalc_st_waiting | recalc_bit_at_least_once)) {
          return true;
        }
        break;
      }
      case recalc_st_need_relaunch: {
        int old = recalc_st_need_relaunch;
        if (recalc_state_.compare_exchange_strong(old, recalc_st_need_relaunch | recalc_bit_at_least_once)) {
          return false; // false here, unlike above, but like below
        }
        break;
      }
      case recalc_st_need_relaunch | recalc_bit_at_least_once:
        return false;
      default:
        assert (0);
    }
  }
  return false;
}

} // namespace tinf
