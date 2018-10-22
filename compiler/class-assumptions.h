#ifndef PHP_CLASS_ASSUMPTIONS_H
#define PHP_CLASS_ASSUMPTIONS_H

#pragma once

#include "compiler/data_ptr.h"

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

  Assumption(AssumType type, const std::string &var_name, ClassPtr klass) :
    assum_type(type),
    var_name(var_name),
    klass(klass) {}
};


AssumType assumption_get(FunctionPtr f, const std::string &var_name, ClassPtr &out_class);
AssumType assumption_get(ClassPtr c, const std::string &var_name, ClassPtr &out_class);
AssumType infer_class_of_expr(FunctionPtr f, VertexPtr root, ClassPtr &out_class, size_t depth = 0);
AssumType calc_assumption_for_return(FunctionPtr f, ClassPtr &out_class);

#endif //PHP_CLASS_ASSUMPTIONS_H
