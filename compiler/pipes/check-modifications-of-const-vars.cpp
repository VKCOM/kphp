#include "compiler/pipes/check-modifications-of-const-vars.h"

#include "common/termformat/termformat.h"

#include "compiler/data/class-data.h"
#include "compiler/data/var-data.h"
#include "compiler/phpdoc.h"

VertexPtr CheckModificationsOfConstVars::on_enter_vertex(VertexPtr v, LocalT *) {
  check_modifications(v);
  return v;
}

void CheckModificationsOfConstVars::check_modifications(VertexPtr v, bool write_flag) const {
  if (auto set_op = v.try_as<op_set>()) {
    auto lvalue = set_op->lhs();
    if (auto var_inited = lvalue.try_as<op_var>()) {
      // todo подумать, куда писать и как сохранять @kphp-const для обычных переменных
      bool const_var_initialization = var_inited->is_const;
      if (const_var_initialization) {
        var_inited->var_id->marked_as_const = true;
        var_inited->var_id->is_read_only = false;
        return;
      }
    }
  }

  if (OpInfo::rl(v->type()) == rl_set) {
    return check_modifications(v.as<meta_op_binary>()->lhs(), true);
  }

  switch (v->type()) {
    case op_prefix_inc:
    case op_postfix_inc:
    case op_prefix_dec:
    case op_postfix_dec:
    case op_conv_array_l:
    case op_conv_int_l:
    case op_conv_string_l:
      return check_modifications(v.as<meta_op_unary>()->expr(), true);

    case op_foreach: {
      auto foreach_param = v.as<op_foreach>()->params();
      if (foreach_param->x()->ref_flag) {
        check_modifications(foreach_param->xs(), true);
      }
      check_modifications(foreach_param->x(), true);

      if (foreach_param->has_key()) {
        check_modifications(foreach_param->key(), true);
      }
      break;
    }

    case op_func_call:
    case op_constructor_call: {
      FunctionPtr func = v.as<op_func_call>()->func_id;
      if (!func) {
        return;
      }

      auto func_call = v.as<op_func_call>();
      auto cnt_params = func_call->args().size();
      for (int i = 0; i < cnt_params; ++i) {
        auto param = func->get_params()[i].try_as<op_func_param>();
        if (param && param->var()->ref_flag) {
          check_modifications(func_call->args()[i], true);
        }
      }
      break;
    }

    case op_index:
      return check_modifications(v.as<op_index>()->array(), write_flag);

    case op_list: {
      for (auto lhs_var_in_list : v.as<op_list>()->list()) {
        check_modifications(lhs_var_in_list, true);
      }
      break;
    }

    case op_var:
    case op_instance_prop: {
      if (write_flag) {
        if (auto const_var = v->type() == op_var ? v.as<op_var>()->var_id : v.as<op_instance_prop>()->var_id) {
          const_var->is_read_only = false;
          if (!const_var->marked_as_const) {
            return;
          }
          const bool modification_allowed = const_var->class_id &&
                                            const_var->class_id->construct_function == current_function &&
                                            v->type() == op_instance_prop &&
                                            v.as<op_instance_prop>()->instance()->get_string() == "this";
          auto err_msg = "Modification of const variable: " + TermStringFormat::paint(const_var->name, TermStringFormat::red);
          kphp_error(modification_allowed, err_msg.c_str());
        }
      }
      break;
    }

    case op_unset: {
      return check_modifications(v.as<op_unset>()->expr(), true);
    }

    default:
      return;
  }
}
