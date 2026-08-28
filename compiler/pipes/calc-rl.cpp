// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-rl.h"

#include "compiler/data/function-data.h"
#include "compiler/type-hint.h"
#include "compiler/vertex.h"

void rl_calc(VertexPtr root, RLValueType expected_rl_type);

template<RLValueType type, RLValueType except_type>
void rl_calc_all(VertexPtr root, int except) {
  int ii = 0;
  for (auto v : *root) {
    if (ii == except) {
      rl_calc(v, except_type);
    } else {
      rl_calc(v, type);
    }
    ii++;
  }
}

template<RLValueType type>
void rl_calc_all(VertexPtr root) {
  for (auto v : *root) {
    rl_calc(v, type);
  }
}

void rl_func_call_calc(VertexPtr root, RLValueType expected_rl_type) {
  kphp_error(expected_rl_type != val_l, "Function result cannot be used as lvalue");
  switch (root->type()) {
  case op_list:
    rl_calc_all<val_l, val_r>(root, root->size() - 1);
    return;
  case op_seq_comma:
  case op_seq_rval:
    rl_calc_all<val_none, val_r>(root, root->size() - 1);
    return;
  case op_fork:
    rl_calc_all<val_none>(root);
    return;
  case op_array: // TODO: in fact it is wrong
  case op_tuple:
  case op_shape:
  case op_defined:
    rl_calc_all<val_r>(root);
    return;
  case op_func_call:
    break;
  default:
    kphp_fail();
    break;
  }

  auto call = root.as<op_func_call>();

  VertexRange func_params = call->func_id->get_params();
  auto func_param_it = func_params.begin();

  for (auto call_arg : call->args()) {
    auto param = (*func_param_it).as<op_func_param>();
    RLValueType tp = param->var()->ref_flag ? val_l : val_r;
    rl_calc(call_arg, tp);
    ++func_param_it;
  }
}

void rl_other_calc(VertexPtr root, RLValueType expected_rl_type) {
  switch (root->type()) {
  case op_ffi_new: {
    auto v = root.try_as<op_ffi_new>();
    if (v->has_array_size_expr()) {
      rl_calc(v->array_size_expr(), expected_rl_type);
    }
    break;
  }
  case op_ffi_cdata_value_ref:
  case op_conv_array_l:
  case op_conv_int_l:
  case op_conv_string_l:
    rl_calc_all<val_l>(root);
    break;
  case op_noerr:
    rl_calc(root.as<op_noerr>()->expr(), expected_rl_type);
    break;
  case op_phpdoc_var:
    break;
  case op_list_keyval: {
    const auto kv = root.as<op_list_keyval>();
    rl_calc(kv->key(), val_r);
    rl_calc(kv->var(), val_l);
    break;
  }
  default:
    assert("Unknown operation in rl_other_calc" && 0);
    break;
  }
}

void rl_common_calc(VertexPtr root, RLValueType expected_rl_type) {
  kphp_error(expected_rl_type == val_none, "Invalid lvalue/rvalue operation");
  switch (root->type()) {
  case op_if:
  case op_while:
  case op_switch:
  case op_case:
    rl_calc_all<val_none, val_r>(root, 0);
    break;
  case op_do:
    rl_calc_all<val_none, val_r>(root, 1);
    break;
  case op_ffi_new:
    break;
  case op_ffi_array_set:
  case op_return:
  case op_break:
  case op_continue:
  case op_throw:
    rl_calc_all<val_r>(root);
    break;
  case op_unset: // TODO: fix it (???)
    rl_calc_all<val_l>(root);
    break;
  case op_try: {
    auto v = root.as<op_try>();
    rl_calc(v->try_cmd(), val_none);
    for (auto c : v->catch_list()) {
      auto catch_op = c.as<op_catch>();
      rl_calc(catch_op->var(), val_none);
      rl_calc(catch_op->cmd(), val_none);
    }
    break;
  }
  case op_seq:
  case op_foreach:
  case op_default:
    rl_calc_all<val_none>(root);
    break;
  case op_for:
    // TODO: it may be untrue
    rl_calc_all<val_none, val_r>(root, 1);
    break;
  case op_global:
  case op_static:
  case op_empty:
    break;
  case op_foreach_param:
    if (root.as<op_foreach_param>()->x()->ref_flag) {
      rl_calc_all<val_l, val_none>(root, 2);
    } else {
      rl_calc_all<val_l, val_r>(root, 0);
    }
    break;
  case op_function:
    rl_calc(root.as<op_function>()->cmd(), val_none);
    break;

  default:
    kphp_fail();
    break;
  }
  return;
}

void rl_calc(VertexPtr root, RLValueType expected_rl_type) {
  stage::set_location(root->get_location());

  root->rl_type = expected_rl_type;

  Operation tp = root->type();

  // fprintf (stderr, "rl_calc (%p = %s)\n", root, OpInfo::str (tp).c_str());

  switch (OpInfo::rl(tp)) {
  case rl_set:
    switch (expected_rl_type) {
    case val_r:
    case val_none: {
      auto set_op = root.as<meta_op_binary>();
      if (set_op->lhs()->extra_type != op_ex_var_superlocal_inplace) {
        rl_calc(set_op->lhs(), val_l);
      }
      rl_calc(set_op->rhs(), val_r);
      break;
    }
    case val_l:
      kphp_error(0, fmt_format("trying to use result of [{}] as lvalue", OpInfo::str(tp)));
      break;
    default:
      kphp_fail();
      break;
    }
    break;
  case rl_index: {
    auto index = root.as<op_index>();
    VertexPtr array = index->array();
    switch (expected_rl_type) {
    case val_l:
      rl_calc(array, val_l);
      if (index->has_key()) {
        rl_calc(index->key(), val_r);
      }
      break;
    case val_r:
    case val_none:
      kphp_error(vk::any_of_equal(array->type(), op_var, op_index, op_func_call, op_instance_prop, op_array), "op_index has to be used on lvalue");
      rl_calc(array, val_r);

      if (index->has_key()) {
        rl_calc(index->key(), val_r);
      }
      break;
    default:
      assert(0);
      break;
    }
    break;
  }
  case rl_instance_prop: {
    VertexPtr lhs = root.as<op_instance_prop>()->instance();
    switch (expected_rl_type) {
    case val_l:
      rl_calc(lhs, val_r); // ..a..->b as a whole is lvalue, but ..a.. is rvalue
      break;
    case val_r:
    case val_none:
      if (lhs->type() != op_var) { // "$a->prop" is most common usage, but others like "f()->prop" are also possible
        kphp_error(vk::any_of_equal(lhs->type(), op_index, op_func_call, op_instance_prop, op_clone, op_seq_rval, op_ffi_new, op_ffi_addr, op_ffi_array_get),
                   "op_instance_prop has to be used on lvalue");
      }
      rl_calc(lhs, val_r);
      break;
    default:
      assert(0);
      break;
    }
    break;
  }
  case rl_op:
    switch (expected_rl_type) {
    case val_l:
      kphp_error(0, "Can't make result of operation to be lvalue");
      break;
    case val_r:
    case val_none:
      rl_calc_all<val_r>(root);
      break;
    default:
      assert(0);
      break;
    }
    break;

  case rl_op_l:
    switch (expected_rl_type) {
    case val_l:
      kphp_error(0, "Can't make result of operation to be lvalue");
      break;
    case val_r:
    case val_none:
      rl_calc(root.as<meta_op_unary>()->expr(), val_l);
      break;
    default:
      kphp_fail();
      break;
    }
    break;

  case rl_const:
    switch (expected_rl_type) {
    case val_l:
      kphp_error(0, "Can't make const to be lvalue");
      break;
    case val_r:
    case val_none:
      break;
    default:
      kphp_fail();
      break;
    }
    break;
  case rl_var:
    switch (expected_rl_type) {
    case val_l:
      kphp_error(root->extra_type != op_ex_var_const, "Can't make const to be lvalue");
      break;
    case val_r:
    case val_none:
      break;
    default:
      kphp_fail();
      break;
    }
    break;
  case rl_other:
    rl_other_calc(root, expected_rl_type);
    break;
  case rl_func:
    rl_func_call_calc(root, expected_rl_type);
    break;
  case rl_mem_func:
    rl_calc_all<val_r>(root);
    break;
  case rl_common:
    rl_common_calc(root, expected_rl_type);
    break;

  default:
    kphp_fail();
    break;
  }
}

void CalcRLF::execute(FunctionPtr function, DataStream<FunctionPtr>& os) {
  stage::set_name("Calc RL");
  stage::set_function(function);

  rl_calc(function->root, val_none);

  if (stage::has_error()) {
    return;
  }

  os << function;
}
