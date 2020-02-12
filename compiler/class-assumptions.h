#ifndef PHP_CLASS_ASSUMPTIONS_H
#define PHP_CLASS_ASSUMPTIONS_H

#pragma once

#include "common/smart_ptrs/intrusive_ptr.h"

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

class Assumption : public vk::thread_safe_refcnt<Assumption> {
public:
  virtual ~Assumption() = default;

  virtual std::string as_human_readable() const = 0;

  template<class Derived>
  const Derived* try_as() const {
    return dynamic_cast<Derived*>(this);
  }

  template<class Derived>
  Derived* try_as() {
    return dynamic_cast<Derived*>(this);
  }
};

class AssumUnknown : public Assumption {
  AssumUnknown() = default;

public:

  static vk::intrusive_ptr<Assumption> create() {
    auto self = new AssumUnknown();
    return vk::intrusive_ptr<Assumption>(self);
  }

  std::string as_human_readable() const override;
};

class AssumNotInstance : public Assumption {
  AssumNotInstance() = default;

public:

  static vk::intrusive_ptr<Assumption> create() {
    auto self = new AssumNotInstance();
    return vk::intrusive_ptr<Assumption>(self);
  }

  std::string as_human_readable() const override;
};

class AssumInstance : public Assumption {
  AssumInstance() = default;

public:
  ClassPtr klass;

  static vk::intrusive_ptr<Assumption> create(ClassPtr klass) {
    auto self = new AssumInstance();
    self->klass = klass;
    return vk::intrusive_ptr<Assumption>(self);
  }

  std::string as_human_readable() const override;
};

class AssumInstanceArray : public Assumption {
  AssumInstanceArray() = default;

public:
  ClassPtr klass;

  static vk::intrusive_ptr<Assumption> create(ClassPtr klass) {
    auto self = new AssumInstanceArray();
    self->klass = klass;
    return vk::intrusive_ptr<Assumption>(self);
  }

  std::string as_human_readable() const override;
};


void assumption_add_for_var(FunctionPtr f, vk::string_view var_name, const vk::intrusive_ptr<Assumption> &assumption);
vk::intrusive_ptr<Assumption> assumption_get_for_var(FunctionPtr f, vk::string_view var_name);
vk::intrusive_ptr<Assumption> assumption_get_for_var(ClassPtr c, vk::string_view var_name);
vk::intrusive_ptr<Assumption> infer_class_of_expr(FunctionPtr f, VertexPtr root, size_t depth = 0);
vk::intrusive_ptr<Assumption> calc_assumption_for_return(FunctionPtr f, VertexAdaptor<op_func_call> call);
vk::intrusive_ptr<Assumption> calc_assumption_for_var(FunctionPtr f, vk::string_view var_name, size_t depth = 0);

#endif //PHP_CLASS_ASSUMPTIONS_H
