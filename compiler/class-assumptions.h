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
public:
  AssumType assum_type;
  std::string var_name;
  ClassPtr klass;

  Assumption() :
    assum_type(assum_unknown),
    var_name(),
    klass() {}

  Assumption(AssumType type, std::string var_name, ClassPtr klass) :
    assum_type(type),
    var_name(std::move(var_name)),
    klass(std::move(klass)) {}
};


void assumption_add_for_var(FunctionPtr f, AssumType assum, const std::string &var_name, ClassPtr klass);
AssumType assumption_get_for_var(FunctionPtr f, const std::string &var_name, ClassPtr &out_class);
AssumType assumption_get_for_var(ClassPtr c, const std::string &var_name, ClassPtr &out_class);
AssumType infer_class_of_expr(FunctionPtr f, VertexPtr root, ClassPtr &out_class, size_t depth = 0);
AssumType calc_assumption_for_return(FunctionPtr f, VertexAdaptor<op_func_call> call, ClassPtr &out_class);
AssumType calc_assumption_for_var(FunctionPtr f, const std::string &var_name, ClassPtr &out_class, size_t depth = 0);

#endif //PHP_CLASS_ASSUMPTIONS_H
