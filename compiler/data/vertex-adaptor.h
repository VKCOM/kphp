// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "auto/compiler/vertex/is-base-of.h"
#include "common/wrappers/fmt_format.h"

#include "compiler/operation.h"
#include "compiler/stage.h"

template<Operation Op>
class vertex_inner;

template<Operation Op>
class VertexAdaptor;

class Location;

using VertexPtr = VertexAdaptor<meta_op_base>;

VertexPtr clone_vertex(VertexPtr);

template<Operation Op>
class VertexAdaptor {
  vertex_inner<Op> *impl;
public:
  struct Hash {
    size_t operator()(const VertexAdaptor<Op> &arg) const noexcept {
      return reinterpret_cast<size_t>(arg.impl);
    }
  };

public:

  VertexAdaptor() :
    impl(nullptr) {
  }

  explicit VertexAdaptor(vertex_inner<Op> *impl) :
    impl(impl) {
  }

  template<Operation FromOp>
  VertexAdaptor(const VertexAdaptor<FromOp> &from) :
    impl(static_cast<vertex_inner<Op> *>(from.impl)) {
    static_assert(op_type_is_base_of(Op, FromOp), "Strange cast to not base vertex");
  }

  template<Operation FromOp>
  VertexAdaptor &operator=(const VertexAdaptor<FromOp> &from) & {
    static_assert(op_type_is_base_of(Op, FromOp), "Strange assignment to not base vertex");
    impl = static_cast<vertex_inner<Op> *> (from.impl);
    return *this;
  }

  VertexAdaptor &operator=(const VertexAdaptor<Op> &) & = default;

  explicit operator bool() const {
    return impl != nullptr;
  }

  vertex_inner<Op> *operator->() {
    assert(impl != nullptr);
    return impl;
  }

  const vertex_inner<Op> *operator->() const {
    assert(impl != nullptr);
    return impl;
  }

  vertex_inner<Op> &operator*() {
    assert(impl != nullptr);
    return *impl;
  }

  const vertex_inner<Op> &operator*() const {
    assert(impl != nullptr);
    return *impl;
  }

  template<Operation to>
  VertexAdaptor<to> as() const {
    if (!impl) {
      return {};
    };
    auto res = try_as<to>();
    kphp_assert_msg(res, fmt_format("Can't cast VertexAdaptor<{}>(real type {}) to VertexAdaptor<{}>",
                                    OpInfo::op_str(Op), impl ? OpInfo::op_str(impl->type()) : "nullptr", OpInfo::op_str(to)));
    return res;
  }

  template<Operation to>
  VertexAdaptor<to> try_as() const {
    if (!impl) {
      return {};
    };
    static_assert(op_type_is_base_of(Op, to), "Strange downcast to not derived vertex");
    static_assert(Op != to || Op == meta_op_base, "Useless cast");
    if (op_type_is_base_of(to, impl->type())) {
      return VertexAdaptor<to>{static_cast<vertex_inner<to> *>(impl)};
    }

    return {};
  }

  static void init_properties(OpProperties *x) {
    vertex_inner<Op>::init_properties(x);
  }

  template<typename... Args>
  static VertexAdaptor<Op> create(Args &&... args) {
    return VertexAdaptor<Op>(vertex_inner<Op>::create(std::forward<Args>(args)...));
  }

  template<typename... Args>
  static VertexAdaptor<Op> create_vararg(Args &&... args) {
    return VertexAdaptor<Op>(vertex_inner<Op>::create_vararg(std::forward<Args>(args)...));
  }

  VertexAdaptor<Op> clone() const {
    if (impl == nullptr) {
      return {};
    }
    return VertexAdaptor<Op>(clone_vertex(*this).template as<Op>());
  }

  template<Operation Op2>
  friend class VertexAdaptor;

  bool operator==(const VertexAdaptor<Op> &other) const noexcept {
    return impl == other.impl;
  }

  bool operator!=(const VertexAdaptor<Op> &other) const noexcept {
    return impl != other.impl;
  }

  VertexAdaptor &set_rl_type(RLValueType valueType) {
    impl->rl_type = valueType;
    return *this;
  }

  VertexAdaptor &set_location(const Location &location) {
    impl->location = location;
    return *this;
  }

  template<Operation Op2>
  VertexAdaptor &set_location(VertexAdaptor<Op2> v) {
    impl->location = v.impl->location;
    return *this;
  }

  VertexAdaptor &set_location_recursively(Location l) {
    set_location(l);
    for (auto child: *impl) {
      child.set_location_recursively(l);
    }
    return *this;
  }

  template<Operation Op2>
  VertexAdaptor &set_location_recursively(VertexAdaptor<Op2> v) {
    return set_location_recursively(v.impl->location);
  }

  // use 'vertex.debugPrint()' anywhere in your code while development
  // (in release it's not used and is not linked to a binary)
  void debugPrint() const {
    std::string debugVertexTree(VertexPtr root);   // implemented in debug.cpp
    printf("%s", debugVertexTree(VertexPtr(*this)).c_str());
  }
};
