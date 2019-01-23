#include "compiler/pipes/calc-rl.h"

#include "compiler/data/function-data.h"
#include "compiler/vertex.h"

void rl_calc(VertexPtr root, RLValueType expected_rl_type);

template <RLValueType type, RLValueType except_type>
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

template <RLValueType type>
void rl_calc_all(VertexPtr root) {
  for (auto v: *root) {
    rl_calc(v, type);
  }
}

void rl_func_call_calc(VertexPtr root, RLValueType expected_rl_type) {
  kphp_error (expected_rl_type != val_l, "Function result cannot be used as lvalue");
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
    case op_array: //TODO: in fact it is wrong
    case op_tuple:
    case op_print:
    case op_min:
    case op_max:
    case op_defined:
    case op_require:
      rl_calc_all<val_r>(root);
      return;
    case op_func_call:
    case op_constructor_call:
      break;
    default:
    kphp_fail();
      break;
  }
  FunctionPtr f = root->get_func_id();
  if (f->varg_flag) {
    rl_calc_all<val_r>(root);
    return;
  }
  VertexRange params = f->root.as<meta_op_function>()->params().
    as<op_func_param_list>()->params();
  auto params_it = params.begin();

  assert (root->size() <= (int)params.size());

  for (auto i : *root) {
    if ((*params_it)->type() == op_func_param_callback) {
    } else {
      VertexAdaptor<op_func_param> param = *params_it;
      RLValueType tp = param->var()->ref_flag ? val_l : val_r;
      rl_calc(i, tp);
    }

    ++params_it;
  }
}

void rl_other_calc(VertexPtr root, RLValueType expected_rl_type) {
  switch (root->type()) {
    case op_conv_array_l:
    case op_conv_int_l:
      rl_calc_all<val_l>(root);
      break;
    case op_noerr:
      rl_calc(root.as<op_noerr>()->expr(), expected_rl_type);
      break;
    default:
      assert ("Unknown operation in rl_other_calc" && 0);
      break;
  }
}

void rl_common_calc(VertexPtr root, RLValueType expected_rl_type) {
  kphp_error (expected_rl_type == val_none, "Invalid lvalue/rvalue operation");
  switch (root->type()) {
    case op_if:
    case op_do:
    case op_while:
    case op_switch:
    case op_case:
      rl_calc_all<val_none, val_r>(root, 0);
      break;
    case op_require:
    case op_require_once:
    case op_return:
    case op_break:
    case op_continue:
    case op_dbg_echo:
    case op_echo:
    case op_throw:
    case op_var_dump:
      rl_calc_all<val_r>(root);
      break;
    case op_unset: //TODO: fix it (???)
      rl_calc_all<val_l>(root);
      break;
    case op_try:
    case op_seq:
    case op_foreach:
    case op_default:
      rl_calc_all<val_none>(root);
      break;
    case op_for:
      //TODO: it may be untrue
      rl_calc_all<val_none, val_r>(root, 1);
      break;
    case op_global:
    case op_static:
    case op_empty:
    case op_func_decl:
    case op_extern_func:
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

  //fprintf (stderr, "rl_calc (%p = %s)\n", root, OpInfo::str (tp).c_str());

  switch (OpInfo::rl(tp)) {
    case rl_set:
      switch (expected_rl_type) {
        case val_r:
        case val_none: {
          VertexAdaptor<meta_op_binary> set_op = root;
          if (set_op->lhs()->extra_type != op_ex_var_superlocal_inplace) {
            rl_calc(set_op->lhs(), val_l);
          }
          rl_calc(set_op->rhs(), val_r);
          break;
        }
        case val_l:
          kphp_error (0, format("trying to use result of [%s] as lvalue", OpInfo::str(tp).c_str()));
          break;
        default:
        kphp_fail();
          break;
      }
      break;
    case rl_index: {
      VertexAdaptor<op_index> index = root;
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
          kphp_error (array->type() == op_var || array->type() == op_index ||
                      array->type() == op_func_call || array->type() == op_instance_prop,
                      "op_index has to be used on lvalue");
          rl_calc(array, val_r);

          if (index->has_key()) {
            rl_calc(index->key(), val_r);
          }
          break;
        default:
          assert (0);
          break;
      }
      break;
    }
    case rl_instance_prop: {
      VertexPtr lhs = root.as<op_instance_prop>()->instance();
      switch (expected_rl_type) {
        case val_l:
          break;
        case val_r:
        case val_none:
          kphp_error (lhs->type() == op_var || lhs->type() == op_index
                      || lhs->type() == op_func_call || lhs->type() == op_constructor_call
                      || lhs->type() == op_instance_prop,
                      "op_instance_prop has to be used on lvalue");
          rl_calc(lhs, val_r);
          break;
        default:
          assert (0);
          break;
      }
      break;
    }
    case rl_op:
      switch (expected_rl_type) {
        case val_l:
          kphp_error (0, "Can't make result of operation to be lvalue");
          break;
        case val_r:
        case val_none:
          rl_calc_all<val_r>(root);
          break;
        default:
          assert (0);
          break;
      }
      break;

    case rl_op_l:
      switch (expected_rl_type) {
        case val_l:
          kphp_error (0, "Can't make result of operation to be lvalue");
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
          kphp_error (0, "Can't make const to be lvalue");
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
          kphp_error (root->extra_type != op_ex_var_const, "Can't make const to be lvalue");
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

void CalcRLF::execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
  stage::set_name("Calc RL");
  stage::set_function(function);

  rl_calc(function->root, val_none);

  if (stage::has_error()) {
    return;
  }

  os << function;
}
