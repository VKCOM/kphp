#include "compiler/pipes/preprocess-eq3.h"

#include "common/algorithms/contains.h"
#include "common/wrappers/likely.h"

VertexPtr PreprocessEq3Pass::on_exit_vertex(VertexPtr root) {
  if (root->type() == op_eq3 || root->type() == op_neq3) {
    auto eq_op = root.as<meta_op_binary>();
    VertexPtr a = eq_op->lhs();
    VertexPtr b = eq_op->rhs();

    if (a->type() == op_null || b->type() == op_null) {
      root = convert_eq3_null_to_isset(root, a->type() == op_null ? b : a);
    }
  }

  return root;
}

/*
 * $a === null это !isset($a); $a !== null это isset($a)
 * аналогично для массивов: $a[$i] !== null это isset($a[$i]) (скомпилится в нативный isset массива)
 */
inline VertexPtr PreprocessEq3Pass::convert_eq3_null_to_isset(VertexPtr eq_op, VertexPtr not_null) {
  VertexPtr v = not_null;
  while (v->type() == op_index) {
    v = v.as<op_index>()->array();
  }
  if (auto var_v = v.try_as<op_var>()) {
  // увы, это пока нужно для компиляции реального php кода, но от этого хочется избавиться потом
    if (var_v->str_val == "connection" || vk::contains(var_v->str_val, "MC")) {
      return eq_op;
    }

    auto isset = VertexAdaptor<op_isset>::create(not_null).set_location(not_null->location);
    if (eq_op->type() != op_neq3) {
      return VertexAdaptor<op_log_not>::create(isset);
    }
    return isset;
  }

  return eq_op;
}
