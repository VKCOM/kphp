// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "auto/compiler/vertex/is-base-of.h"
#include "common/wrappers/fmt_format.h"

#include "compiler/location.h"
#include "compiler/operation.h"

template<Operation Op>
class vertex_inner;

template<Operation Op>
class VertexAdaptor;

using VertexPtr = VertexAdaptor<meta_op_base>;

VertexPtr clone_vertex(VertexPtr);

template<Operation Op>
class VertexAdaptor {
  vertex_inner<Op> *impl_{nullptr};

  template<Operation Op2>
  friend class VertexAdaptor;

public:
  struct Hash {
    size_t operator()(const VertexAdaptor<Op> &arg) const noexcept {
      return reinterpret_cast<size_t>(arg.impl_);
    }
  };

  VertexAdaptor() = default;

  explicit VertexAdaptor(vertex_inner<Op> *impl) noexcept
    : impl_(impl) {}

  template<Operation FromOp, typename = std::enable_if_t<op_type_is_base_of(Op, FromOp)>>
  VertexAdaptor(const VertexAdaptor<FromOp> &from) noexcept
    : impl_(static_cast<vertex_inner<Op> *>(from.impl_)) {
  }

  template<Operation FromOp, typename = std::enable_if_t<op_type_is_base_of(Op, FromOp)>>
  VertexAdaptor<Op> &operator=(const VertexAdaptor<FromOp> &from) noexcept {
    impl_ = static_cast<vertex_inner<Op> *>(from.impl_);
    return *this;
  }

  explicit operator bool() const noexcept {
    return impl_ != nullptr;
  }

  vertex_inner<Op> *operator->() noexcept {
    assert(impl_ != nullptr);
    return impl_;
  }

  const vertex_inner<Op> *operator->() const noexcept {
    assert(impl_ != nullptr);
    return impl_;
  }

  vertex_inner<Op> &operator*() noexcept {
    assert(impl_ != nullptr);
    return *impl_;
  }

  const vertex_inner<Op> &operator*() const noexcept {
    assert(impl_ != nullptr);
    return *impl_;
  }

  template<Operation to>
  VertexAdaptor<to> as() const noexcept {
    auto res = try_as<to>();
    kphp_assert_msg(res, fmt_format("Can't cast VertexAdaptor<{}>(real type {}) to VertexAdaptor<{}>", OpInfo::op_str(Op),
                                    impl_ ? OpInfo::op_str(impl_->type()) : "nullptr", OpInfo::op_str(to)));
    return res;
  }

  template<Operation to>
  VertexAdaptor<to> try_as() const noexcept {
    if (!impl_) {
      return {};
    };
    static_assert(op_type_is_base_of(Op, to), "Strange downcast to not derived vertex");
    static_assert(Op != to || Op == meta_op_base, "Useless cast");
    if (op_type_is_base_of(to, impl_->type())) {
      return VertexAdaptor<to>{static_cast<vertex_inner<to> *>(impl_)};
    }

    return {};
  }

  static void init_properties(OpProperties *prop) {
    vertex_inner<Op>::init_properties(prop);
  }

  template<typename... Args>
  static VertexAdaptor<Op> create(Args &&...args) noexcept {
    return VertexAdaptor<Op>{vertex_inner<Op>::create(std::forward<Args>(args)...)};
  }

  template<typename... Args>
  static VertexAdaptor<Op> create_vararg(Args &&...args) noexcept {
    return VertexAdaptor<Op>{vertex_inner<Op>::create_vararg(std::forward<Args>(args)...)};
  }

  VertexAdaptor<Op> clone() const noexcept {
    return impl_ ? VertexAdaptor<Op>(clone_vertex(*this).template as<Op>()) : VertexAdaptor<Op>{};
  }

  bool operator==(const VertexAdaptor<Op> &other) const noexcept {
    return impl_ == other.impl_;
  }

  bool operator!=(const VertexAdaptor<Op> &other) const noexcept {
    return impl_ != other.impl_;
  }

  VertexAdaptor<Op> &set_rl_type(RLValueType type) noexcept {
    impl_->rl_type = type;
    return *this;
  }

  VertexAdaptor<Op> &set_location(const Location &location) noexcept {
    impl_->location = location;
    return *this;
  }

  template<Operation Op2>
  VertexAdaptor<Op> &set_location(VertexAdaptor<Op2> other) noexcept {
    impl_->location = other.impl_->location;
    return *this;
  }

  VertexAdaptor<Op> &set_location_recursively(const Location &location) noexcept {
    set_location(location);
    for (auto child : *impl_) {
      child.set_location_recursively(location);
    }
    return *this;
  }

  template<Operation Op2>
  VertexAdaptor<Op> &set_location_recursively(VertexAdaptor<Op2> other) noexcept {
    return set_location_recursively(other.impl_->location);
  }

  // use 'vertex.debugPrint()' anywhere in your code while development
  // (in release it's not used and is not linked to a binary)
  void debugPrint() const noexcept {
    std::string debugVertexTree(VertexPtr root); // implemented in debug.cpp
    printf("%s", debugVertexTree(VertexPtr(*this)).c_str());
  }
};
