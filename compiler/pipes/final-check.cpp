#include "compiler/pipes/final-check.h"

#include "compiler/compiler-core.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/gentree.h"

bool FinalCheckPass::on_start(FunctionPtr function) {
  if (!FunctionPassBase::on_start(function) || function->type() == FunctionData::func_extern) {
    return false;
  }

  for (auto &static_var : function->static_var_ids) {
    check_static_var_inited(static_var);
  }

  return true;
}

void FinalCheckPass::init() {
  from_return = 0;
}

VertexPtr FinalCheckPass::on_enter_vertex(VertexPtr vertex, LocalT *) {
  if (vertex->type() == op_return) {
    from_return++;
  }
  if (vertex->type() == op_func_name) {
    kphp_error (0, format("Unexpected %s (maybe, it should be a define?)", vertex->get_c_string()));
  }
  if (vertex->type() == op_addr) {
    kphp_error (0, "Getting references is unsupported");
  }
  if (vertex->type() == op_eq3) {
    const TypeData *type_left = tinf::get_type(VertexAdaptor<meta_op_binary>(vertex)->lhs());
    const TypeData *type_right = tinf::get_type(VertexAdaptor<meta_op_binary>(vertex)->rhs());
    if ((type_left->ptype() == tp_float && !type_left->or_false_flag()) ||
        (type_right->ptype() == tp_float && !type_right->or_false_flag())) {
      kphp_warning(format("Using === with float operand"));
    }
    if (!can_be_same_type(type_left, type_right)) {
      kphp_warning(format("=== with %s and %s as operands will be always false",
                           type_out(type_left).c_str(),
                           type_out(type_right).c_str()));

    }
  }
  if (vertex->type() == op_add) {
    const TypeData *type_left = tinf::get_type(VertexAdaptor<meta_op_binary>(vertex)->lhs());
    const TypeData *type_right = tinf::get_type(VertexAdaptor<meta_op_binary>(vertex)->rhs());
    if ((type_left->ptype() == tp_array) ^ (type_right->ptype() == tp_array)) {
      if (type_left->ptype() != tp_var && type_right->ptype() != tp_var) {
        kphp_warning (format("%s + %s is strange operation",
                              type_out(type_left).c_str(),
                              type_out(type_right).c_str()));
      }
    }
  }
  if (vk::any_of_equal(vertex->type(), op_sub, op_mul, op_div, op_mod, op_pow)) {
    const TypeData *type_left = tinf::get_type(VertexAdaptor<meta_op_binary>(vertex)->lhs());
    const TypeData *type_right = tinf::get_type(VertexAdaptor<meta_op_binary>(vertex)->rhs());
    if ((type_left->ptype() == tp_array) || (type_right->ptype() == tp_array)) {
      kphp_warning (format("%s %s %s is strange operation",
                            OpInfo::str(vertex->type()).c_str(),
                            type_out(type_left).c_str(),
                            type_out(type_right).c_str()));
    }
  }

  if (vertex->type() == op_foreach) {
    VertexPtr arr = vertex.as<op_foreach>()->params().as<op_foreach_param>()->xs();
    const TypeData *arrayType = tinf::get_type(arr);
    if (arrayType->ptype() == tp_array) {
      const TypeData *valueType = arrayType->lookup_at(Key::any_key());
      if (valueType->get_real_ptype() == tp_Unknown) {
        kphp_error (0, "Can not compile foreach on array of Unknown type");
      }
    }
  }
  if (vertex->type() == op_list) {
    VertexPtr arr = vertex.as<op_list>()->array();
    const TypeData *arrayType = tinf::get_type(arr);
    if (arrayType->ptype() == tp_array) {
      const TypeData *valueType = arrayType->lookup_at(Key::any_key());
      kphp_error (valueType->get_real_ptype() != tp_Unknown, "Can not compile list with array of Unknown type");
    } else if (arrayType->ptype() == tp_tuple) {
      size_t list_size = vertex.as<op_list>()->list().size();
      size_t tuple_size = arrayType->get_tuple_max_index();
      kphp_error (list_size <= tuple_size, format("Can't assign tuple of length %zd to list of length %zd", tuple_size, list_size));
    }
  }
  if (vertex->type() == op_index && vertex.as<op_index>()->has_key()) {
    VertexPtr arr = vertex.as<op_index>()->array();
    VertexPtr key = vertex.as<op_index>()->key();
    const TypeData *arrayType = tinf::get_type(arr);
    if (arrayType->ptype() == tp_tuple) {
      long index = parse_int_from_string(GenTree::get_actual_value(key).as<op_int_const>());
      size_t tuple_size = arrayType->get_tuple_max_index();
      kphp_error (0 <= index && index < tuple_size, format("Can't get element %ld of tuple of length %zd", index, tuple_size));
    }
  }
  if (vertex->type() == op_unset || vertex->type() == op_isset) {
    for (auto v : vertex.as<meta_op_xset>()->args()) {
      if (v->type() == op_var) {    // isset($var), unset($var)
        VarPtr var = v.as<op_var>()->get_var_id();
        if (vertex->type() == op_unset) {
          kphp_error(!var->is_reference, "Unset of reference variables is not supported");
          if (var->is_in_global_scope()) {
            FunctionPtr f = stage::get_function();
            if (f->type() != FunctionData::func_global && f->type() != FunctionData::func_switch) {
              kphp_error(0, "Unset of global variables in functions is not supported");
            }
          }
        } else {
          kphp_error(!var->is_constant(), "Can't use isset on const variable");
        }
      } else if (v->type() == op_index) {   // isset($arr[index]), unset($arr[index])
        const TypeData *arrayType = tinf::get_type(v.as<op_index>()->array());
        PrimitiveType ptype = arrayType->get_real_ptype();
        kphp_error(ptype == tp_array || ptype == tp_var, "Can't use isset/unset by[idx] for not an array");
      }
    }
  }
  if (vertex->type() == op_func_call) {
    FunctionPtr fun = vertex.as<op_func_call>()->get_func_id();
    if (fun->root->void_flag && vertex->rl_type == val_r && from_return == 0) {
      if (fun->type() != FunctionData::func_switch && fun->file_id->main_func_name != fun->name) {
        kphp_warning(format("Using result of void function %s\n", fun->get_human_readable_name().c_str()));
      }
    }
    check_op_func_call(vertex.as<op_func_call>());
  }

  if (G->env().get_warnings_level() >= 2 && vk::any_of_equal(vertex->type(), op_require, op_require_once)) {
    FunctionPtr function_where_require = stage::get_function();

    if (function_where_require && function_where_require->type() == FunctionData::func_local) {
      for (auto v : *vertex) {
        FunctionPtr function_which_required = v->get_func_id();

        kphp_assert(function_which_required);
        kphp_assert(function_which_required->type() == FunctionData::func_global);

        for (VarPtr global_var : function_which_required->global_var_ids) {
          if (!global_var->marked_as_global) {
            kphp_warning(("require file with global variable not marked as global: " + global_var->name).c_str());
          }
        }
      }
    }
  }
  //TODO: may be this should be moved to tinf_check
  return vertex;
}

VertexPtr FinalCheckPass::on_exit_vertex(VertexPtr vertex, LocalT *) {
  if (vertex->type() == op_return) {
    from_return--;
  }
  return vertex;
}

void FinalCheckPass::check_op_func_call(VertexAdaptor<op_func_call> call) {
  FunctionPtr f = call->get_func_id();
  VertexRange func_params = f->root.as<meta_op_function>()->params().as<op_func_param_list>()->params();

  VertexRange call_params = call->args();
  int call_params_n = static_cast<int>(call_params.size());
  if (call_params_n != func_params.size()) {
    return;
  }

  for (int i = 0; i < call_params_n; i++) {
    if (func_params[i]->type() != op_func_param_callback) {
      kphp_error(call_params[i]->type() != op_func_ptr, "Unexpected function pointer");
      continue;
    }

    ClassPtr lambda_class = FunctionData::is_lambda(call_params[i]);
    kphp_error_act(call_params[i]->type() == op_func_ptr || lambda_class, "Callable object expected", continue);

    FunctionPtr func_ptr_of_callable = call_params[i]->get_func_id();

    kphp_error_act(func_ptr_of_callable->root, format("Unknown callback function [%s]", func_ptr_of_callable->get_human_readable_name().c_str()), continue);
    VertexRange cur_params = func_ptr_of_callable->root.as<meta_op_function>()->params().as<op_func_param_list>()->params();

    int given_arguments_count = static_cast<int>(cur_params.size()) - static_cast<bool>(lambda_class);
    int expected_arguments_count = func_params[i].as<op_func_param_callback>()->params().as<op_func_param_list>()->params().size();
    kphp_error_act(given_arguments_count == expected_arguments_count, "Wrong callback arguments count", continue);

    for (auto cur_param : cur_params) {
      kphp_error_return(cur_param->type() == op_func_param, "Callback function with callback parameter");
      kphp_error_return(cur_param.as<op_func_param>()->var()->ref_flag == 0, "Callback function with reference parameter");
    }
  }
}

/*
 * Inspection: static-переменная должна быть проинициализирована при объявлении (только если это не var)
 */
inline void FinalCheckPass::check_static_var_inited(VarPtr static_var) {
  kphp_error(static_var->init_val || tinf::get_type(static_var)->ptype() == tp_var,
             format("static $%s is not inited at declaration (inferred %s)",
                     static_var->name.c_str(), colored_type_out(tinf::get_type(static_var)).c_str()));
}
