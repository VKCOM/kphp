// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <forward_list>
#include <string>

#include "compiler/debug.h"
#include "compiler/inferring/type-data.h"
#include "compiler/location.h"
#include "compiler/threading/locks.h"

namespace tinf {

class Edge;

class TypeInferer;

class Node : public Lockable {
  DEBUG_STRING_METHOD {
    return as_human_readable();
  }

private:
  // this list contains edges from=this to=other
  std::forward_list<const tinf::Edge *> edges_from_this_;
  // this list contains edges to=this from=other
  std::forward_list<const tinf::Edge *> edges_to_this_;
  // using forward_list here is a bit-bit slower than vector, but takes considerably less memory
  // these two lists are separate variables, as joining them into one list causes negative performance impact

protected:
  enum {
    recalc_st_waiting = 0,
    recalc_st_processing = 1,
    recalc_st_need_relaunch = 2,
    // and three more states — the same by meaning, but with bit 16 on (16, 17, 18)
    // that 3 more states — with bit 16 on — mean that recalc has once finished <=> recalc count > 0
    recalc_bit_at_least_once = 16,
  };

  const TypeData *type_{TypeData::get_type(tp_any)};

  // this field is a finite-state automation for multithreading synchronization, see enum above
  // if should be placed here (after TypeData*) to make it join with the next int field in memory
  int recalc_state_{recalc_st_waiting};

public:
  int isset_flags{0}; // ifi bitmask, see get_ifi_id(): for example, when is_integer($a) then node of $a has this flags
  int isset_was{0};   // used to find "result may differ from PHP" error, see RestrictionIsset

  std::string as_human_readable() const;

  bool was_recalc_started_at_least_once() const {
    return recalc_state_ > recalc_st_waiting;
  }

  bool was_recalc_finished_at_least_once() const {
    return recalc_state_ >= recalc_bit_at_least_once;
  }

  void register_edge_from_this(const tinf::Edge *edge);
  void register_edge_to_this(const tinf::Edge *edge);

  const std::forward_list<const Edge *> &get_edges_from_this() const {
    return edges_from_this_;
  }

  const std::forward_list<const Edge *> &get_edges_to_this() const {
    return edges_to_this_;
  }

  bool try_start_recalc();
  void start_recalc();
  bool try_finish_recalc();

  const TypeData *get_type() const {
    return type_;
  }

  void set_type(const TypeData *type) {
    type_ = type;
  }

  virtual void recalc(TypeInferer *inferer) = 0;
  virtual std::string get_description() = 0;
  virtual const Location &get_location() const = 0;
};

} // namespace tinf
