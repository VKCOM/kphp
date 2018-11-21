#include "compiler/inferring/lvalue.h"

#include "compiler/vertex.h"

LValue as_lvalue(VertexPtr v) {
  if (v->type() == op_instance_prop) {
    return as_lvalue(v->get_var_id());
  }

  int depth = 0;
  if (v->type() == op_foreach_param) {
    depth++;
    v = v.as<op_foreach_param>()->xs();
  }
  while (v->type() == op_index) {
    depth++;
    v = v.as<op_index>()->array();
  }

  tinf::Node *value = nullptr;
  if (v->type() == op_var) {
    value = tinf::get_tinf_node(v->get_var_id());
  } else if (v->type() == op_conv_array_l || v->type() == op_conv_int_l) {
    kphp_assert (depth == 0);
    return as_lvalue(v.as<meta_op_unary>()->expr());
  } else if (v->type() == op_array) {
    kphp_fail();
  } else if (v->type() == op_func_call) {
    value = tinf::get_tinf_node(v.as<op_func_call>()->get_func_id(), -1);
  } else if (v->type() == op_instance_prop) {       // при $a->arr[] = 1; когда не работает верхнее условие
    value = tinf::get_tinf_node(v->get_var_id());
  } else {
    kphp_error (0, format("Bug in compiler: Trying to use [%s] as lvalue", OpInfo::str(v->type()).c_str()));
    kphp_fail();
  }

  kphp_assert (value != 0);
  return LValue(value, &MultiKey::any_key(depth));
}
