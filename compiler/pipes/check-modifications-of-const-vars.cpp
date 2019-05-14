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
      bool const_var_initialization = var_inited->is_const || PhpDocTypeRuleParser::is_tag_in_phpdoc(set_op->phpdoc_str, php_doc_tag::kphp_const);
      if (const_var_initialization) {
        var_inited->get_var_id()->marked_as_const = true;
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
      return check_modifications(v.as<meta_op_unary>()->expr(), true);

    case op_foreach: {
      auto foreach_param = v.as<op_foreach>()->params();
      if (foreach_param->x()->ref_flag) {
        check_modifications(foreach_param->xs(), true);
      }
      break;
    }

    case op_func_call:
    case op_constructor_call: {
      FunctionPtr func = v->get_func_id();
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

    case op_var:
    case op_instance_prop: {
      if (write_flag) {
        auto const_var = v->get_var_id();
        if (const_var && const_var->marked_as_const) {
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

    default:
      return;
  }
}
