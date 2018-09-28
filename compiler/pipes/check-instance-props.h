#pragma once

#include "compiler/function-pass.h"

/**
 * Для обращений к свойствам $a->b определяем тип $a, убеждаемся в наличии свойства b у класса,
 * а также заполняем vertex<op_instance_prop>->var (понадобится для type inferring)
 */
class CheckInstanceProps : public FunctionPassBase {
private:
  AUTO_PROF (check_instance_props);

  /**
   * Если при объявлении поля класса написано / ** @var int|false * / к примеру, делаем type_rule из phpdoc.
   * Это заставит type inferring принимать это во внимание, и если где-то выведется по-другому, будет ошибка.
   * С инстансами это тоже работает, т.е. / ** @var \AnotherClass * / будет тоже проверяться при выводе типов.
   */
  void init_class_instance_var(VertexPtr v, VarPtr var, ClassPtr klass);

public:
  string get_description() {
    return "Check instance properties";
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *);

};
