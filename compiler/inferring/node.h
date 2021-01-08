// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "compiler/location.h"
#include "compiler/threading/locks.h"

class TypeData;

namespace tinf {

class Edge;

class TypeInferer;

class Node : public Lockable {
private:
  std::vector<Edge *> next_;
  std::vector<Edge *> rev_next_;
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

  int get_recalc_cnt() const {
    return recalc_cnt_;
  }

  void add_edge(Edge *edge);
  void add_rev_edge(Edge *edge);

  inline const std::vector<Edge *> &get_next() const {
    return next_;
  }

  inline const std::vector<Edge *> &get_rev_next() const {
    return rev_next_;
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
