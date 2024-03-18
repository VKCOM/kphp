// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/lvalue.h"

#include "common/algorithms/find.h"

#include "compiler/vertex.h"

LValue as_lvalue(VertexPtr v) {
  if (auto prop = v.try_as<op_instance_prop>()) {
    return as_lvalue(prop->var_id);
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
  if (auto var = v.try_as<op_var>()) {
    value = tinf::get_tinf_node(var->var_id);
  } else if (vk::any_of_equal(v->type(), op_conv_array_l, op_conv_int_l, op_conv_string_l, op_ffi_cdata_value_ref)) {
    kphp_assert(depth == 0);
    return as_lvalue(v.as<meta_op_unary>()->expr());
  } else if (v->type() == op_array) {
    kphp_fail();
  } else if (auto call = v.try_as<op_func_call>()) {
    value = tinf::get_tinf_node(call->func_id, -1);
  } else if (auto prop = v.try_as<op_instance_prop>()) { // for '$a->arr[] = 1;'
    value = tinf::get_tinf_node(prop->var_id);
  } else {
    kphp_error(0, fmt_format("Bug in compiler: Trying to use [{}] as lvalue", OpInfo::str(v->type())));
    kphp_fail();
  }

  kphp_assert(value != nullptr);
  return {value, &MultiKey::any_key(depth)};
}
