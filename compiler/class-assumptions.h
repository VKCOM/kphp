// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>

#include "common/smart_ptrs/intrusive_ptr.h"

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/debug.h"


enum class AssumptionStatus {
  unknown,
  processing,
  initialized
};

class Assumption : public vk::thread_safe_refcnt<Assumption> {
  DEBUG_STRING_METHOD { return as_human_readable(); }

public:
  virtual ~Assumption() = default;

  virtual std::string as_human_readable() const = 0;
  virtual bool is_primitive() const = 0;
  virtual vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const = 0;
};

class AssumNotInstance final : public Assumption {
  AssumNotInstance() = default;

public:

  static auto create() {
    return vk::intrusive_ptr<Assumption>(new AssumNotInstance());
  }

  std::string as_human_readable() const final;
  bool is_primitive() const final;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const final;
};

class AssumInstance : public Assumption {
protected:
  AssumInstance() = default;

public:
  ClassPtr klass;

  static auto create(ClassPtr klass) {
    auto *self = new AssumInstance();
    self->klass = klass;
    return vk::intrusive_ptr<Assumption>(self);
  }

  std::string as_human_readable() const final;
  bool is_primitive() const final;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const final;
};

class AssumArray final : public Assumption {
  AssumArray() = default;

public:
  vk::intrusive_ptr<Assumption> inner;

  static auto create(const vk::intrusive_ptr<Assumption> &inner) {
    auto *self = new AssumArray();
    self->inner = inner;
    return vk::intrusive_ptr<Assumption>(self);
  }

  static auto create(ClassPtr klass) {
    return create(AssumInstance::create(klass));
  }

  std::string as_human_readable() const final;
  bool is_primitive() const final;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const final;
};

class AssumTuple final : public Assumption {
  AssumTuple() = default;

public:
  std::vector<vk::intrusive_ptr<Assumption>> subkeys_assumptions;

  static auto create(std::vector<vk::intrusive_ptr<Assumption>> &&sub) {
    auto *self = new AssumTuple();
    self->subkeys_assumptions = std::move(sub);
    return vk::intrusive_ptr<AssumTuple>(self);
  }

  std::string as_human_readable() const final;
  bool is_primitive() const final;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const final;
};

class AssumShape final : public Assumption {
  AssumShape() = default;

public:
  std::map<std::string, vk::intrusive_ptr<Assumption>> subkeys_assumptions;

  static auto create(std::map<std::string, vk::intrusive_ptr<Assumption>> &&sub) {
    auto *self = new AssumShape();
    self->subkeys_assumptions = std::move(sub);
    return vk::intrusive_ptr<AssumShape>(self);
  }

  std::string as_human_readable() const final;
  bool is_primitive() const final;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const final;
};

class AssumTypedCallable final : public Assumption {
  AssumTypedCallable() = default;

public:
  InterfacePtr interface;

  static auto create(InterfacePtr callable_interface) {
    auto *self = new AssumTypedCallable();
    self->interface = callable_interface;
    return vk::intrusive_ptr<AssumTypedCallable>(self);
  }

  std::string as_human_readable() const final;
  bool is_primitive() const final;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const final;
};


void assumption_add_for_var(FunctionPtr f, vk::string_view var_name, const vk::intrusive_ptr<Assumption> &assumption);
vk::intrusive_ptr<Assumption> assumption_get_for_var(FunctionPtr f, vk::string_view var_name);
vk::intrusive_ptr<Assumption> infer_class_of_expr(FunctionPtr f, VertexPtr root, size_t depth = 0);
vk::intrusive_ptr<Assumption> calc_assumption_for_return(FunctionPtr f, VertexAdaptor<op_func_call> call);
vk::intrusive_ptr<Assumption> calc_assumption_for_var(FunctionPtr f, vk::string_view var_name, size_t depth = 0);
vk::intrusive_ptr<Assumption> calc_assumption_for_argument(FunctionPtr f, vk::string_view var_name);
