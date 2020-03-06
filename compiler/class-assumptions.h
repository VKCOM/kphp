#pragma once

#include "common/smart_ptrs/intrusive_ptr.h"

#include <map>

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

class TypeData;

class Assumption : public vk::thread_safe_refcnt<Assumption> {
public:
  virtual ~Assumption() = default;

  virtual std::string as_human_readable() const = 0;
  virtual bool is_primitive() const = 0;
  virtual const TypeData *get_type_data() const = 0;
  virtual vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const = 0;
};

class AssumNotInstance : public Assumption {
  AssumNotInstance() = default;

public:

  static auto create() {
    auto self = new AssumNotInstance();
    return vk::intrusive_ptr<Assumption>(self);
  }

  std::string as_human_readable() const override;
  bool is_primitive() const override;
  const TypeData *get_type_data() const override;
  vk::intrusive_ptr<Assumption> get_subkey_by_index(VertexPtr index_key) const override;
};

class AssumInstance : public Assumption {
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

class AssumArray : public Assumption {
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

class AssumTuple : public Assumption {
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

class AssumShape : public Assumption {
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


void assumption_add_for_var(FunctionPtr f, vk::string_view var_name, const vk::intrusive_ptr<Assumption> &assumption);
vk::intrusive_ptr<Assumption> assumption_get_for_var(FunctionPtr f, vk::string_view var_name);
vk::intrusive_ptr<Assumption> assumption_get_for_var(ClassPtr c, vk::string_view var_name);
vk::intrusive_ptr<Assumption> infer_class_of_expr(FunctionPtr f, VertexPtr root, size_t depth = 0);
vk::intrusive_ptr<Assumption> calc_assumption_for_return(FunctionPtr f, VertexAdaptor<op_func_call> call);
vk::intrusive_ptr<Assumption> calc_assumption_for_var(FunctionPtr f, vk::string_view var_name, size_t depth = 0);
