// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/wrappers/iterator_range.h"

#include "compiler/common.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/expr-node.h"
#include "compiler/inferring/primitive-type.h"

template<Operation Op>
vertex_inner<Op> *raw_create_vertex_inner(int args_n);

class TypeHint;

template<>
class vertex_inner<meta_op_base> {
public:
  using value_type = VertexPtr;
  using xiterator = value_type *;
  using const_xiterator = const value_type *;
  using iterator = std::reverse_iterator<xiterator>;
  using const_iterator = std::reverse_iterator<const_xiterator>;

private:
  Operation type_;
  int n;
public:
  int id;
  tinf::ExprNode tinf_node;
  Location location;

  OperationExtra extra_type : 4;
  RLValueType rl_type : 2;
  RLValueType val_ref_flag : 2;
  ConstValueType const_type : 2;
  bool ref_flag : 1;
  bool throw_flag : 1;
  bool used_flag : 1;

private:
  VertexPtr *arr() const {
    return (VertexPtr *)this - 1;
  }

protected:

  bool check_range(int i) const { return 0 <= i && i < size(); }

  VertexPtr &ith(int i) {
    kphp_assert(check_range(i));
    return arr()[-i];
  }

  const VertexPtr &ith(int i) const {
    kphp_assert(check_range(i));
    return arr()[-i];
  }

  template<class... Args>
  void set_children(int shift, VertexPtr arg, Args &&... args) {
    ith(shift) = arg;
    set_children(shift + 1, std::forward<Args>(args)...);
  }

  template<Operation op, class... Args>
  void set_children(int shift, const std::vector<VertexAdaptor<op>> &arg, Args &&... args) {
    for (int i = 0, ni = (int)arg.size(); i < ni; i++) {
      ith(shift + i) = arg[i];
    }
    set_children(shift + (int)arg.size(), std::forward<Args>(args)...);
  }

  template<class... Args>
  void set_children(int shift, const vk::iterator_range<vertex_inner<meta_op_base>::iterator> &arg, Args &&... args) {
    for (int i = 0, ni = (int)arg.size(); i < ni; i++) {
      ith(shift + i) = arg[i];
    }
    set_children(shift + (int)arg.size(), std::forward<Args>(args)...);
  }

  void set_children(int shift) {
    kphp_assert_msg(shift == n, "???");
  }

  template<class... Args>
  static int get_children_size(const VertexPtr &, Args &&... args) {
    return 1 + get_children_size(std::forward<Args>(args)...);
  }

  template<Operation op, class... Args>
  static int get_children_size(const std::vector<VertexAdaptor<op>> &arg, Args &&... args) {
    return (int)arg.size() + get_children_size(std::forward<Args>(args)...);
  }

  template<class... Args>
  static int get_children_size(const vk::iterator_range<vertex_inner<meta_op_base>::iterator> &arg, Args &&... args) {
    return (int)arg.size() + get_children_size(std::forward<Args>(args)...);
  }

  static int get_children_size() {
    return 0;
  }


public:
  vertex_inner() :
    type_(op_none),
    n(-1),
    id(0),
    tinf_node(VertexPtr(this)),
    location(),
    extra_type(op_ex_none),
    rl_type(val_error),
    val_ref_flag(val_none),
    const_type(cnst_error_),
    ref_flag(false),
    throw_flag(false),
    used_flag(false) {
  }

  vertex_inner(const vertex_inner<meta_op_base> &from) :
    type_(from.type_),
    n(-1),
    id(from.id),
    tinf_node(VertexPtr(this)),
    location(from.location),
    extra_type(from.extra_type),
    rl_type(from.rl_type),
    val_ref_flag(from.val_ref_flag),
    const_type(from.const_type),
    ref_flag(from.ref_flag),
    throw_flag(from.throw_flag),
    used_flag(from.used_flag) {
  }

  virtual ~vertex_inner() {}

  void copy_location_and_flags(const vertex_inner<meta_op_base> &from) {
    location = from.location;
    val_ref_flag = from.val_ref_flag;
    const_type = from.const_type;
    ref_flag = from.ref_flag;
    throw_flag = from.throw_flag;
    used_flag = from.used_flag;
  }

  void raw_init(int real_n) {
    kphp_assert(n == -1);
    n = real_n;
    for (int i = 0; i < n; i++) {
      new(&ith(i)) VertexPtr();
    }
  }

  void raw_copy(const vertex_inner<meta_op_base> &from) {
    kphp_assert(n == -1);
    n = from.size();
    for (int i = 0; i < n; i++) {
      new(&ith(i)) VertexPtr(from.ith(i).clone());
    }
  }

  int size() const { return n; }

  VertexPtr &back() { return ith(size() - 1); }

  std::vector<VertexPtr> get_next() { return std::vector<VertexPtr>(begin(), end()); }

  bool empty() { return size() == 0; }

  iterator begin() { return iterator(arr() + 1); }

  iterator end() { return iterator(arr() - size() + 1); }

  const_iterator begin() const { return const_iterator(arr() + 1); }

  const_iterator end() const { return const_iterator(arr() - size() + 1); }

  const Location &get_location() const { return location; }

  void init() {}

  static void init_properties(OpProperties *p __attribute__((unused))) {
    p->op_str = "meta_op_base";
  }

  const Operation &type() const { return type_; }

  virtual const std::string &get_string() const { kphp_fail_msg (fmt_format("not supported [{}:{}]", type_, OpInfo::str(type_))); }

  virtual void set_string(std::string) { kphp_fail_msg (fmt_format("not supported [{}:{}]", type_, OpInfo::str(type_))); }

  virtual bool has_get_string() const { return false; }

  template<Operation Op>
  friend vertex_inner<Op> *raw_create_vertex_inner(int args_n);

  template<typename... Args>
  static vertex_inner<meta_op_base> *create_vararg(Args &&... args) {
    vertex_inner<meta_op_base> *v = raw_create_vertex_inner<meta_op_base>(get_children_size(std::forward<Args>(args)...));
    v->set_children(0, std::forward<Args>(args)...);
    return v;
  }

    static bool deep_equal(VertexPtr lhs, VertexPtr rhs) {
      if (lhs == rhs) {
        return true;
      }
      if (!lhs || !rhs) {
        return false;
      }

      auto get_string_def = [](VertexPtr v) {
        return v->has_get_string() ? v->get_string() : "";
      };

      return lhs->type() == rhs->type() &&
             get_string_def(lhs) == get_string_def(rhs) &&
             std::equal(lhs->begin(), lhs->end(), rhs->begin(), rhs->end(), deep_equal);
    }
};

using Vertex = vertex_inner<meta_op_base>;
using VertexRange = vk::iterator_range<Vertex::iterator>;
using VertexConstRange = vk::iterator_range<Vertex::const_iterator>;

