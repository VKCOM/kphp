#ifndef PHP_CLASS_ASSUMPTIONS_H
#define PHP_CLASS_ASSUMPTIONS_H

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

enum AssumType {
  assum_unknown,
  assum_not_instance,
  assum_instance,
  assum_instance_array,
};

class Assumption {
  Assumption(AssumType type, ClassPtr klass) :
    assum_type(type),
    klass(klass) {}

public:
  Assumption() = default;

  AssumType assum_type = assum_unknown;
  ClassPtr klass;

  static Assumption not_instance() {
    return {assum_not_instance, ClassPtr{}};
  }

  static Assumption unknown() {
    return {assum_unknown, {}};
  }

  static Assumption instance(ClassPtr klass) {
    return {assum_instance, klass};
  }

  static Assumption array(ClassPtr klass) {
    return {assum_instance_array, klass};
  }
};


void assumption_add_for_var(FunctionPtr f, vk::string_view var_name, const Assumption &assumption);
const Assumption *assumption_get_for_var(FunctionPtr f, vk::string_view var_name);
const Assumption *assumption_get_for_var(ClassPtr c, vk::string_view var_name);
Assumption infer_class_of_expr(FunctionPtr f, VertexPtr root, size_t depth = 0);
Assumption calc_assumption_for_return(FunctionPtr f, VertexAdaptor<op_func_call> call);
Assumption calc_assumption_for_var(FunctionPtr f, vk::string_view var_name, size_t depth = 0);

#endif //PHP_CLASS_ASSUMPTIONS_H
