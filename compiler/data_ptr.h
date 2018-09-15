#pragma once

#include "compiler/graph.h"
#include "compiler/operation.h"

/** used pointers to **/
class VarData;

class ClassData;

class DefineData;

class FunctionData;

class SrcFile;

class Token;

class FunctionSet;

class Location;

typedef Id<FunctionData> FunctionPtr;
typedef Id<VarData> VarPtr;
typedef Id<ClassData> ClassPtr;
typedef Id<DefineData> DefinePtr;
typedef Id<SrcFile> SrcFilePtr;
typedef Id<FunctionSet> FunctionSetPtr;

template<Operation Op>
class vertex_inner;

template<Operation Op>
class VertexAdaptor;

typedef VertexAdaptor<meta_op_base> VertexPtr;

VertexPtr clone_vertex(VertexPtr);

//TODO: use private and protected when necessary
template<Operation Op>
class VertexAdaptor {
public:
  vertex_inner<Op> *impl;

  VertexAdaptor() :
    impl(nullptr) {
  }

  explicit VertexAdaptor(vertex_inner<Op> *impl) :
    impl(impl) {
  }

  template<Operation FromOp>
  VertexAdaptor(const VertexAdaptor<FromOp> &from) :
    impl(dynamic_cast <vertex_inner<Op> *> (from.impl)) {
    dl_assert (impl != nullptr, dl_pstr("Can't cast VertexAdaptor<%d>(real type %d) to VertexAdaptor<%d>", FromOp, from.is_null() ? -1 : from->type(), Op));
  }

  template<Operation FromOp>
  VertexAdaptor &operator=(const VertexAdaptor<FromOp> &from) {
    impl = dynamic_cast <vertex_inner<Op> *> (from.impl);
    dl_assert (from.impl == nullptr || impl != nullptr, dl_pstr("Can't cast VertexAdaptor<%d> (real type %d) to VertexAdaptor<%d>", FromOp, from.is_null() ? -1 : from->type(), Op));
    return *this;
  }

  inline bool is_null() const {
    return impl == nullptr;
  }

  inline bool not_null() const {
    return !is_null();
  }

  vertex_inner<Op> *operator->() {
    assert (impl != nullptr);
    return impl;
  }

  const vertex_inner<Op> *operator->() const {
    assert (impl != nullptr);
    return impl;
  }

  vertex_inner<Op> &operator*() {
    assert (impl != nullptr);
    return *impl;
  }

  const vertex_inner<Op> &operator*() const {
    assert (impl != nullptr);
    return *impl;
  }

  template<Operation to>
  VertexAdaptor<to> as() {
    return *this;
  }

  template<Operation to>
  const VertexAdaptor<to> as() const {
    return *this;
  }

  static void init_properties(OpProperties *x) {
    vertex_inner<Op>::init_properties(x);
  }

  template <typename... Args>
  static VertexAdaptor<Op> create(Args... args) {
    return VertexAdaptor<Op>(vertex_inner<Op>::create(args...));
  }

  template <typename... Args>
  static VertexAdaptor<Op> copy_create(VertexAdaptor<meta_op_base> from, Args... args) {
    return VertexAdaptor<Op>(vertex_inner<Op>::copy_create(from, args...));
  }

  VertexAdaptor<Op> clone() const {
    return VertexAdaptor<Op>(clone_vertex(*this));
  }

};


