// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <forward_list>

#include "compiler/debug.h"
#include "compiler/location.h"
#include "compiler/threading/locks.h"

class TypeData;

namespace tinf {

class Edge;

class TypeInferer;

class Node : public Lockable {
  DEBUG_STRING_METHOD { return as_human_readable(); }

private:
  // this list contains edges from=this to=other
  std::forward_list<const tinf::Edge *> edges_from_this_;
  // this list contains edges to=this from=other
  std::forward_list<const tinf::Edge *> edges_to_this_;
  // using forward_list here is a bit-bit slower than vector, but takes considerably less memory
  // these two lists are separate variables, as joining them into one list causes negative performance impact

  volatile int recalc_state_;
public:
  const TypeData *type_;
  volatile int recalc_cnt_;
  int isset_flags;
  int isset_was;

  enum {
    empty_st,
    own_st,
    own_recalc_st
  };

  Node();

  std::string as_human_readable() const;

  int get_recalc_cnt() const {
    return recalc_cnt_;
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
