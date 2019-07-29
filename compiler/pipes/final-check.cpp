#include "compiler/pipes/final-check.h"

#include "common/termformat/termformat.h"

#include "compiler/compiler-core.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/gentree.h"

namespace {
void check_class_immutableness(ClassPtr klass) {
  if (!klass->is_immutable) {
    return;
  }
  klass->members.for_each([klass](const ClassMemberInstanceField &field) {
    kphp_assert(field.var->marked_as_const);
    std::unordered_set<ClassPtr> sub_classes;
    field.var->tinf_node.get_type()->get_all_class_types_inside(sub_classes);
    for (auto sub_class : sub_classes) {
      kphp_error(sub_class->is_immutable,
                 format("Field %s of immutable class %s should be immutable too, but class %s is mutable",
                        TermStringFormat::paint(field.local_name(), TermStringFormat::red).c_str(),
                        TermStringFormat::paint(klass->name, TermStringFormat::red).c_str(),
                        TermStringFormat::paint(sub_class->name, TermStringFormat::red).c_str()));
    }
  });
}

void check_instance_cache_fetch_immutable_call(VertexAdaptor<op_func_call> call) {
  kphp_assert(call->get_string() == "instance_cache_fetch_immutable");
  auto klass = tinf::get_type(call)->class_type();
  kphp_assert(klass);
  kphp_error(klass->is_immutable,
             format("Can not fetch instance of mutable class %s with instance_cache_fetch_immutable call", klass->name.c_str()));
}

void check_instance_cache_store_call(VertexAdaptor<op_func_call> call) {
  kphp_assert(call->get_string() == "instance_cache_store");
  auto type = tinf::get_type(call->args()[1]);
  kphp_error_return(type->ptype() == tp_Class,
                    "Can not store non-instance var with instance_cache_store call");
  auto klass = type->class_type();
  kphp_error(!klass->is_interface_or_has_interface_member(),
             "Can not store instance with interface inside with instance_cache_store call");
}

void check_instance_to_array_call(VertexAdaptor<op_func_call> call) {
  auto type = tinf::get_type(call->args()[0]);
  kphp_error_return(type->ptype() == tp_Class, "You may not use instance_to_array with non-instance var");

  auto klass = type->class_type();
  kphp_error(!klass->is_interface_or_has_interface_member(), "You may not convert interface to array");
}

void check_func_call_params(VertexAdaptor<op_func_call> call) {
  FunctionPtr f = call->func_id;
  VertexRange func_params = f->get_params();

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

    auto lambda_class = LambdaClassData::get_from(call_params[i]);
    kphp_error_act(lambda_class || call_params[i]->type() == op_func_ptr, "Callable object expected", continue);

    FunctionPtr func_ptr_of_callable =
      call_params[i]->type() == op_func_ptr ?
      call_params[i].as<op_func_ptr>()->func_id :
      call_params[i].as<op_func_call>()->func_id;

    kphp_error_act(func_ptr_of_callable->root, format("Unknown callback function [%s]", func_ptr_of_callable->get_human_readable_name().c_str()), continue);
    VertexRange cur_params = func_ptr_of_callable->get_params();

    for (auto arg : cur_params) {
      auto param_arg = arg.try_as<op_func_param>();
      kphp_error_return(param_arg, "Callback function with callback parameter");
      kphp_error_return(!param_arg->var()->ref_flag, "Callback function with reference parameter");
    }

    auto expected_arguments_count = get_function_params(func_params[i].as<op_func_param_callback>()).size();
    if (!FunctionData::check_cnt_params(expected_arguments_count, func_ptr_of_callable)) {
      continue;
    }
  }
}
} // namespace

bool FinalCheckPass::on_start(FunctionPtr function) {
  if (!FunctionPassBase::on_start(function) || function->is_extern()) {
    return false;
  }

  if (function->type == FunctionData::func_class_holder) {
    check_class_immutableness(function->class_id);
  }

  if (function->is_instance_function() && function->local_name() == "__clone") {
    kphp_error_act(!function->is_resumable, "__clone method has to be not resumable", return false);
    kphp_error_act(!function->can_throw, "__clone method should not throw exception", return false);
  }

  for (auto &static_var : function->static_var_ids) {
    check_static_var_inited(static_var);
  }

  if (function->kphp_lib_export) {
    check_lib_exported_function(function);
  }
  return true;
}

VertexPtr FinalCheckPass::on_enter_vertex(VertexPtr vertex, LocalT *) {
  if (vertex->type() == op_func_name) {
    kphp_error (0, format("Unexpected %s (maybe, it should be a define?)", vertex->get_c_string()));
  }
  if (vertex->type() == op_addr) {
    kphp_error (0, "Getting references is unsupported");
  }
  if (vertex->type() == op_eq3) {
    const TypeData *type_left = tinf::get_type(vertex.as<meta_op_binary>()->lhs());
    const TypeData *type_right = tinf::get_type(vertex.as<meta_op_binary>()->rhs());
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
    const TypeData *type_left = tinf::get_type(vertex.as<meta_op_binary>()->lhs());
    const TypeData *type_right = tinf::get_type(vertex.as<meta_op_binary>()->rhs());
    if ((type_left->ptype() == tp_array) ^ (type_right->ptype() == tp_array)) {
      if (type_left->ptype() != tp_var && type_right->ptype() != tp_var) {
        kphp_warning (format("%s + %s is strange operation",
                              type_out(type_left).c_str(),
                              type_out(type_right).c_str()));
      }
    }
  }
  if (vk::any_of_equal(vertex->type(), op_sub, op_mul, op_div, op_mod, op_pow)) {
    const TypeData *type_left = tinf::get_type(vertex.as<meta_op_binary>()->lhs());
    const TypeData *type_right = tinf::get_type(vertex.as<meta_op_binary>()->rhs());
    if ((type_left->ptype() == tp_array) || (type_right->ptype() == tp_array)) {
      kphp_warning (format("%s %s %s is strange operation",
                            OpInfo::str(vertex->type()).c_str(),
                            type_out(type_left).c_str(),
                            type_out(type_right).c_str()));
    }
  }

  if (vertex->type() == op_foreach) {
    VertexPtr arr = vertex.as<op_foreach>()->params()->xs();
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
    } else {
      kphp_error (arrayType->ptype() == tp_var, format("Can not compile list with '%s'", ptype_name(arrayType->ptype())));
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
      if (auto var_vertex = v.try_as<op_var>()) {    // isset($var), unset($var)
        VarPtr var = var_vertex->var_id;
        if (vertex->type() == op_unset) {
          kphp_error(!var->is_reference, "Unset of reference variables is not supported");
          if (var->is_in_global_scope()) {
            FunctionPtr f = stage::get_function();
            if (f->type != FunctionData::func_global && f->type != FunctionData::func_switch) {
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
    check_op_func_call(vertex.as<op_func_call>());
  }
  if (vertex->type() == op_instance_prop) {
    const TypeData *lhs_type = tinf::get_type(vertex.as<op_instance_prop>()->instance());
    kphp_error(lhs_type->ptype() == tp_Class,
               format("Accessing ->property of non-instance %s", colored_type_out(lhs_type).c_str()));
  }

  if (G->env().get_warnings_level() >= 2 && vertex->type() == op_func_call) {
    FunctionPtr function_where_require = stage::get_function();

    if (function_where_require && function_where_require->type == FunctionData::func_local) {
      FunctionPtr function_which_required = vertex.as<op_func_call>()->func_id;
      if (function_which_required == function_which_required->file_id->main_function) {
        kphp_assert(function_which_required->type == FunctionData::func_global);

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
  return vertex;
}

void FinalCheckPass::check_op_func_call(VertexAdaptor<op_func_call> call) {
  if (call->func_id->is_extern()) {
    auto &function_name = call->get_string();
    if (function_name == "instance_cache_fetch_immutable") {
      check_instance_cache_fetch_immutable_call(call);
    } else if (function_name == "instance_cache_store") {
      check_instance_cache_store_call(call);
    } else if (function_name == "instance_to_array") {
      check_instance_to_array_call(call);
    }
  }

  check_func_call_params(call);
}

/*
 * Inspection: static-переменная должна быть проинициализирована при объявлении (только если это не var)
 */
inline void FinalCheckPass::check_static_var_inited(VarPtr static_var) {
  kphp_error(static_var->init_val || tinf::get_type(static_var)->ptype() == tp_var,
             format("static $%s is not inited at declaration (inferred %s)",
                     static_var->name.c_str(), colored_type_out(tinf::get_type(static_var)).c_str()));
}

void FinalCheckPass::check_lib_exported_function(FunctionPtr function) {
  const TypeData *ret_type = tinf::get_type(function, -1);
  kphp_error(!ret_type->has_class_type_inside(),
             "Can not use class instance in return of @kphp-lib-export function");

  for (auto p: function->get_params()) {
    auto param = p.as<op_func_param>();
    if (param->has_default_value() && param->default_value()) {
      VertexPtr default_value = GenTree::get_actual_value(param->default_value());
      kphp_error_act(vk::any_of_equal(default_value->type(), op_int_const, op_float_const),
                     "Only const int, const float are allowed as default param for @kphp-lib-export function",
                     continue);
    }
    kphp_error(!tinf::get_type(p)->has_class_type_inside(),
               "Can not use class instance in param of @kphp-lib-export function");
  }
}

bool FinalCheckPass::user_recursion(VertexPtr v, LocalT *, VisitVertex<FinalCheckPass> &visit) {
  if (v->type() == op_function) {
    visit(v.as<op_function>()->cmd_ref());
    return true;
  }

  if (vk::any_of_equal(v->type(), op_func_call, op_var, op_index, op_constructor_call)) {
    if (v->rl_type == val_r) {
      const TypeData *type = tinf::get_type(v);
      if (type->get_real_ptype() == tp_Unknown) {
        string index_depth;
        while (v->type() == op_index) {
          v = v.as<op_index>()->array();
          index_depth += "[.]";
        }
        string desc = "Using Unknown type : ";
        if (auto var_vertex = v.try_as<op_var>()) {
          VarPtr var = var_vertex->var_id;
          desc += "variable [$" + var->name + "]" + index_depth;
        } else if (auto cons = v.try_as<op_constructor_call>()) {
          desc += "constructor [" + cons->func_id->name + "]" + index_depth;
        } else if (auto call = v.try_as<op_func_call>()) {
          desc += "function [" + call->func_id->name + "]" + index_depth;
        } else {
          desc += "...";
        }

        kphp_error (0, desc.c_str());
        return true;
      }
    }
  }

  return false;
}
