#include "compiler/pipes/check-modifications-of-const-fields.h"

#include "common/termformat/termformat.h"

#include "compiler/data/class-data.h"
#include "compiler/data/var-data.h"

VertexPtr CheckModificationsOfConstFields::on_enter_vertex(VertexPtr v, LocalT *) {
  check_modification_of_const_class_field(v);
  return v;
}

void CheckModificationsOfConstFields::check_modification_of_const_class_field(VertexPtr v, bool write_flag) const {
  if (OpInfo::rl(v->type()) == rl_set) {
    return check_modification_of_const_class_field(v.as<meta_op_binary>()->lhs(), true);
  }

  switch (v->type()) {
    case op_prefix_inc:
    case op_postfix_inc:
    case op_prefix_dec:
    case op_postfix_dec:
    case op_conv_array_l:
    case op_conv_int_l:
      return check_modification_of_const_class_field(v.as<meta_op_unary>()->expr(), true);

    case op_foreach: {
      auto foreach_param = v.as<op_foreach>()->params();
      if (foreach_param->x()->ref_flag) {
        check_modification_of_const_class_field(foreach_param->xs(), true);
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
          check_modification_of_const_class_field(func_call->args()[i], true);
        }
      }
      break;
    }

    case op_index:
      return check_modification_of_const_class_field(v.as<op_index>()->array(), write_flag);

    case op_instance_prop: {
      if (write_flag) {
        auto var_of_instance_prop = v->get_var_id();
        if (var_of_instance_prop && var_of_instance_prop->marked_as_const) {
          const bool modification_allowed = var_of_instance_prop->class_id &&
                                            var_of_instance_prop->class_id->construct_function == current_function &&
                                            v.as<op_instance_prop>()->instance()->get_string() == "this";
          auto err_msg = "Modification of const field: " + TermStringFormat::paint(var_of_instance_prop->name, TermStringFormat::red);
          kphp_error(modification_allowed, err_msg.c_str());
        }
      }
      break;
    }

    default:
      return;
  }
}
