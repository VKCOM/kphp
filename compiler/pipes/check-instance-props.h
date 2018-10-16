#pragma once

#include "compiler/function-pass.h"

/**
 * Для обращений к свойствам $a->b определяем тип $a, убеждаемся в наличии свойства b у класса,
 * а также заполняем vertex<op_instance_prop>->var (понадобится для type inferring)
 */
class CheckInstanceProps : public FunctionPassBase {
private:
  AUTO_PROF (check_instance_props);

  void init_class_instance_var(VertexPtr v, const ClassMemberInstanceField *field, ClassPtr klass);

public:
  string get_description() {
    return "Check instance properties";
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *);

};
