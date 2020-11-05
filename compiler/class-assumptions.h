// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>

#include "common/smart_ptrs/intrusive_ptr.h"

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/primitive-type.h"

class TypeData;

enum class AssumptionStatus {
  unknown,
  processing,
  initialized
};

class Assumption : public vk::thread_safe_refcnt<Assumption> {
protected:
  bool or_null_{false};
  bool or_false_{false};

public:
  virtual ~Assumption() = default;

  virtual std::string as_human_readable() const = 0;
  virtual bool is_primitive() const = 0;
  virtual const TypeData *get_type_data() const = 0;
  virtual vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const = 0;

  vk::intrusive_ptr<Assumption> add_flags(bool or_null, bool or_false) {
    or_null_ = or_null;
    or_false_ = or_false;
    return vk::intrusive_ptr<Assumption>(this);
  }
};

class AssumNotInstance final : public Assumption {
  AssumNotInstance() = default;
  PrimitiveType type{tp_Any};

public:

  static auto create(PrimitiveType type = tp_Any) {
    auto self = new AssumNotInstance();
    self->type = type != tp_Unknown ? type : tp_Any;
    return vk::intrusive_ptr<Assumption>(self);
  }

  std::string as_human_readable() const override;
  bool is_primitive() const override;
  const TypeData *get_type_data() const override;
  PrimitiveType get_type() const;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const override;
};

class AssumInstance : public Assumption {
protected:
  AssumInstance() = default;

public:
  ClassPtr klass;

  static auto create(ClassPtr klass) {
    auto self = new AssumInstance();
    self->klass = klass;
    return vk::intrusive_ptr<Assumption>(self);
  }

  std::string as_human_readable() const override;
  bool is_primitive() const override;
  const TypeData *get_type_data() const override;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const override;
};

class AssumArray final : public Assumption {
  AssumArray() = default;

public:
  vk::intrusive_ptr<Assumption> inner;

  static auto create(const vk::intrusive_ptr<Assumption> &inner) {
    auto self = new AssumArray();
    self->inner = inner;
    return vk::intrusive_ptr<Assumption>(self);
  }

  static auto create(ClassPtr klass) {
    return create(AssumInstance::create(klass));
  }

  std::string as_human_readable() const override;
  bool is_primitive() const override;
  const TypeData *get_type_data() const override;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const override;
};

class AssumTuple final : public Assumption {
  AssumTuple(const std::vector<vk::intrusive_ptr<Assumption>> &subkeys_assumptions) :
    subkeys_assumptions(subkeys_assumptions) {}

public:
  std::vector<vk::intrusive_ptr<Assumption>> subkeys_assumptions;

  static auto create(std::vector<vk::intrusive_ptr<Assumption>> &&sub) {
    return vk::intrusive_ptr<Assumption>{new AssumTuple(std::move(sub))};
  }

  std::string as_human_readable() const override;
  bool is_primitive() const override;
  const TypeData *get_type_data() const override;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const override;
};

class AssumShape final : public Assumption {
  AssumShape(const std::map<std::string, vk::intrusive_ptr<Assumption>> &subkeys_assumptions) :
    subkeys_assumptions(subkeys_assumptions) {}

public:
  std::map<std::string, vk::intrusive_ptr<Assumption>> subkeys_assumptions;

  static auto create(std::map<std::string, vk::intrusive_ptr<Assumption>> &&sub) {
    return vk::intrusive_ptr<Assumption>{new AssumShape(std::move(sub))};
  }

  std::string as_human_readable() const override;
  bool is_primitive() const override;
  const TypeData *get_type_data() const override;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const override;
};

class AssumCallable final : public AssumInstance {
  AssumCallable() = default;

public:
  static auto create(InterfacePtr callable_interface) {
    auto self = new AssumCallable();
    self->klass = callable_interface;
    return vk::intrusive_ptr<AssumCallable>(self);
  }

  std::string as_human_readable() const final;
};


void assumption_add_for_var(FunctionPtr f, vk::string_view var_name, const vk::intrusive_ptr<Assumption> &assumption);
vk::intrusive_ptr<Assumption> assumption_get_for_var(FunctionPtr f, vk::string_view var_name);
vk::intrusive_ptr<Assumption> assumption_get_for_var(ClassPtr c, vk::string_view var_name);
vk::intrusive_ptr<Assumption> infer_class_of_expr(FunctionPtr f, VertexPtr root, size_t depth = 0);
vk::intrusive_ptr<Assumption> calc_assumption_for_return(FunctionPtr f, VertexAdaptor<op_func_call> call);
vk::intrusive_ptr<Assumption> calc_assumption_for_var(FunctionPtr f, vk::string_view var_name, size_t depth = 0);
vk::intrusive_ptr<Assumption> calc_assumption_for_argument(FunctionPtr f, vk::string_view var_name);
