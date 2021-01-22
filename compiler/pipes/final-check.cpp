// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/final-check.h"

#include "common/termformat/termformat.h"
#include "common/algorithms/string-algorithms.h"

#include "compiler/compiler-core.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/data/vars-collector.h"
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
                 fmt_format("Field {} of immutable class {} should be immutable too, but class {} is mutable",
                            TermStringFormat::paint(std::string{field.local_name()}, TermStringFormat::red),
                            TermStringFormat::paint(klass->name, TermStringFormat::red),
                            TermStringFormat::paint(sub_class->name, TermStringFormat::red)));
    }
  });

  kphp_error(!klass->parent_class || klass->parent_class->is_immutable,
             fmt_format("Immutable class {} has mutable base {}",
                        TermStringFormat::paint(klass->name, TermStringFormat::red),
                        TermStringFormat::paint(klass->parent_class->name, TermStringFormat::red)));
}

void check_instance_cache_fetch_call(VertexAdaptor<op_func_call> call) {
  auto klass = tinf::get_type(call)->class_type();
  kphp_assert(klass);
  klass->deeply_require_instance_cache_visitor();
  kphp_error(klass->is_immutable,
             fmt_format("Can not fetch instance of mutable class {} with instance_cache_fetch call", klass->name));
}

void check_instance_cache_store_call(VertexAdaptor<op_func_call> call) {
  auto type = tinf::get_type(call->args()[1]);
  kphp_error_return(type->ptype() == tp_Class,
                    "Can not store non-instance var with instance_cache_store call");
  auto klass = type->class_type();
  klass->deeply_require_instance_cache_visitor();
  kphp_error(!klass->is_polymorphic_or_has_polymorphic_member(),
             "Can not store instance with interface inside with instance_cache_store call");
  kphp_error(klass->is_immutable,
             fmt_format("Can not store instance of mutable class {} with instance_cache_store call", klass->name));
}

void check_instance_to_array_call(VertexAdaptor<op_func_call> call) {
  auto type = tinf::get_type(call->args()[0]);
  kphp_error_return(type->ptype() == tp_Class, "You may not use instance_to_array with non-instance var");
  type->class_type()->deeply_require_instance_to_array_visitor();
}

void check_estimate_memory_usage_call(VertexAdaptor<op_func_call> call) {
  auto type = tinf::get_type(call->args()[0]);
  std::unordered_set<ClassPtr> classes_inside;
  type->get_all_class_types_inside(classes_inside);
  for (auto klass: classes_inside) {
    klass->deeply_require_instance_memory_estimate_visitor();
  }
}

void check_get_global_vars_memory_stats_call() {
  kphp_error_return(G->settings().enable_global_vars_memory_stats.get(),
                    "function get_global_vars_memory_stats() disabled, use KPHP_ENABLE_GLOBAL_VARS_MEMORY_STATS to enable");
}

void mark_global_vars_for_memory_stats() {
  if (!G->settings().enable_global_vars_memory_stats.get()) {
    return;
  }

  static std::atomic<bool> vars_marked{false};
  if (vars_marked.exchange(true)) {
    return;
  }

  std::unordered_set<ClassPtr> classes_inside;
  VarsCollector vars_collector{0, [&classes_inside](VarPtr variable) {
    tinf::get_type(variable)->get_all_class_types_inside(classes_inside);
    return false;
  }};
  vars_collector.collect_global_and_static_vars_from(G->get_main_file()->main_function);
  for (auto klass: classes_inside) {
    klass->deeply_require_instance_memory_estimate_visitor();
  }
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
    auto func_param = func_params[i].try_as<op_func_param_typed_callback>();
    if (!func_param) {
      kphp_error(call_params[i]->type() != op_func_ptr, "Unexpected function pointer");
      continue;
    }

    auto lambda_class = LambdaClassData::get_from(call_params[i]);
    kphp_error_act(lambda_class || call_params[i]->type() == op_func_ptr, "Callable object expected", continue);

    FunctionPtr func_ptr_of_callable =
      call_params[i]->type() == op_func_ptr ?
      call_params[i].as<op_func_ptr>()->func_id :
      call_params[i].as<op_func_call>()->func_id;

    kphp_error_act(func_ptr_of_callable->root, fmt_format("Unknown callback function [{}]", func_ptr_of_callable->get_human_readable_name()), continue);
    VertexRange cur_params = func_ptr_of_callable->get_params();

    for (auto arg : cur_params) {
      if (auto param_arg = arg.try_as<op_func_param>()) {
        kphp_error_return(!param_arg->var()->ref_flag, "Callback function with reference parameter");
      } else if (auto callback_arg = arg.try_as<op_func_param_typed_callback>()) {
        kphp_error_return(callback_arg->has_default_value(), "Callback function with callback parameter");
      }
    }
    if (func_ptr_of_callable->local_name() == "instance_to_array") {
      auto param_of_callback = func_param->params()->params()[0];
      auto rule_meta = param_of_callback->type_rule->rule();
      if (auto func_type_rule = rule_meta.try_as<op_index>()) {
        auto arg_ref = func_type_rule->array().as<op_type_expr_arg_ref>();
        if (auto arg = GenTree::get_call_arg_ref(arg_ref, call)) {
          auto value_type = tinf::get_type(arg)->lookup_at(Key::any_key());
          auto out_class = value_type->class_type();
          kphp_error_return(out_class, "type of argument for instance_to_array has to be array of Classes");
          out_class->deeply_require_instance_to_array_visitor();
        }
      }
    }

    auto expected_arguments_count = get_function_params(func_param).size();
    if (!FunctionData::check_cnt_params(expected_arguments_count, func_ptr_of_callable)) {
      continue;
    }
  }
}

void check_null_usage_in_binary_operations(VertexAdaptor<meta_op_binary> binary_vertex) {
  auto lhs_type = tinf::get_type(binary_vertex->lhs());
  auto rhs_type = tinf::get_type(binary_vertex->rhs());

  switch (binary_vertex->type()) {
    case op_add:
    case op_set_add:
      if (vk::any_of_equal(tp_array, lhs_type->get_real_ptype(), rhs_type->get_real_ptype())) {
        kphp_error(vk::none_of_equal(tp_any, lhs_type->ptype(), rhs_type->ptype()),
                   fmt_format("Can't use '{}' operation between {} and {} types",
                              OpInfo::str(binary_vertex->type()), colored_type_out(lhs_type), colored_type_out(rhs_type)));
        return;
      }

      // fall through
    case op_mul:
    case op_sub:
    case op_div:
    case op_mod:
    case op_pow:
    case op_and:
    case op_or:
    case op_xor:
    case op_shl:
    case op_shr:

    case op_set_mul:
    case op_set_sub:
    case op_set_div:
    case op_set_mod:
    case op_set_pow:
    case op_set_and:
    case op_set_or:
    case op_set_xor:
    case op_set_shl:
    case op_set_shr: {
      kphp_error((lhs_type->ptype() != tp_any || lhs_type->or_false_flag()) &&
                 (rhs_type->ptype() != tp_any || rhs_type->or_false_flag()),
                 fmt_format("Got '{}' operation between {} and {} types",
                            OpInfo::str(binary_vertex->type()), colored_type_out(lhs_type), colored_type_out(rhs_type)));
      return;
    }

    default:
      return;
  }
}

void check_function_throws(FunctionPtr f) {
  std::unordered_set<std::string> throws_expected(f->check_throws.begin(), f->check_throws.end());
  std::unordered_set<std::string> throws_actual;
  for (const auto &e : f->exceptions_thrown) {
    throws_actual.insert(e->name);
  }
  kphp_error(throws_expected == throws_actual,
             fmt_format("kphp-throws mismatch: have <{}>, want <{}>",
                        vk::join(throws_actual, ", "),
                        vk::join(throws_expected, ", ")));
}
} // namespace

void FinalCheckPass::on_start() {
  mark_global_vars_for_memory_stats();

  if (current_function->type == FunctionData::func_class_holder) {
    check_class_immutableness(current_function->class_id);
  }

  if (current_function->modifiers.is_instance() && current_function->local_name() == ClassData::NAME_OF_CLONE) {
    kphp_error(!current_function->is_resumable, fmt_format("{} method has to be not resumable", ClassData::NAME_OF_CLONE));
    kphp_error(!current_function->can_throw(), fmt_format("{} method should not throw exception", ClassData::NAME_OF_CLONE));
  }

  if (current_function->should_not_throw && current_function->can_throw()) {
    kphp_error(0, fmt_format("Function {} marked as @kphp-should-not-throw, but really can throw an exception:\n{}",
                             current_function->get_human_readable_name(), current_function->get_throws_call_chain()));
  }

  for (auto &static_var : current_function->static_var_ids) {
    check_static_var_inited(static_var);
  }

  if (current_function->kphp_lib_export) {
    check_lib_exported_function(current_function);
  }

  if (!current_function->check_throws.empty()) {
    check_function_throws(current_function);
  }
}

VertexPtr FinalCheckPass::on_enter_vertex(VertexPtr vertex) {
  if (vertex->type() == op_func_name) {
    kphp_error (0, fmt_format("Unexpected {} (maybe, it should be a define?)", vertex->get_string()));
  }
  if (vertex->type() == op_addr) {
    kphp_error (0, "Getting references is unsupported");
  }
  if (vk::any_of_equal(vertex->type(), op_eq3, op_neq3)) {
    check_eq3_neq3(vertex.as<meta_op_binary>()->lhs(), vertex.as<meta_op_binary>()->rhs(), vertex->type());
  }
  if (vk::any_of_equal(vertex->type(), op_lt, op_le, op_gt, op_ge, op_spaceship, op_eq2, op_neq2)) {
    check_comparisons(vertex.as<meta_op_binary>()->lhs(), vertex.as<meta_op_binary>()->rhs(), vertex->type());
  }
  if (vertex->type() == op_add) {
    const TypeData *type_left = tinf::get_type(vertex.as<meta_op_binary>()->lhs());
    const TypeData *type_right = tinf::get_type(vertex.as<meta_op_binary>()->rhs());
    if ((type_left->ptype() == tp_array) ^ (type_right->ptype() == tp_array)) {
      if (type_left->ptype() != tp_mixed && type_right->ptype() != tp_mixed) {
        kphp_warning (fmt_format("{} + {} is strange operation", type_out(type_left), type_out(type_right)));
      }
    }
  }
  if (vk::any_of_equal(vertex->type(), op_sub, op_mul, op_div, op_mod, op_pow)) {
    const TypeData *type_left = tinf::get_type(vertex.as<meta_op_binary>()->lhs());
    const TypeData *type_right = tinf::get_type(vertex.as<meta_op_binary>()->rhs());
    if ((type_left->ptype() == tp_array) || (type_right->ptype() == tp_array)) {
      kphp_warning(fmt_format("{} {} {} is strange operation",
                              OpInfo::str(vertex->type()),
                              type_out(type_left),
                              type_out(type_right)));
    }
  }

  if (vertex->type() == op_foreach) {
    VertexPtr arr = vertex.as<op_foreach>()->params()->xs();
    const TypeData *arrayType = tinf::get_type(arr);
    if (arrayType->ptype() == tp_array) {
      const TypeData *valueType = arrayType->lookup_at(Key::any_key());
      if (valueType->get_real_ptype() == tp_any) {
        kphp_error (0, "Can not compile foreach on array of Unknown type");
      }
    }
  }
  if (vertex->type() == op_list) {
    const auto list = vertex.as<op_list>();
    VertexPtr arr = list->array();
    const TypeData *arrayType = tinf::get_type(arr);
    if (arrayType->ptype() == tp_array) {
      const TypeData *valueType = arrayType->lookup_at(Key::any_key());
      kphp_error (valueType->get_real_ptype() != tp_any, "Can not compile list with array of Unknown type");
    } else if (arrayType->ptype() == tp_tuple) {
      size_t list_size = vertex.as<op_list>()->list().size();
      size_t tuple_size = arrayType->get_tuple_max_index();
      kphp_error (list_size <= tuple_size, fmt_format("Can't assign tuple of length {} to list of length {}", tuple_size, list_size));
      for (auto cur : list->list()) {
        const auto kv = cur.as<op_list_keyval>();
        if (GenTree::get_actual_value(kv->key())->type() != op_int_const) {
          const TypeData *key_type = tinf::get_type(kv->key());
          kphp_error(0, fmt_format("Only int const keys can be used, got '{}'", ptype_name(key_type->ptype())));
        }
      }
    } else if (arrayType->ptype() == tp_shape) {
      for (auto cur : list->list()) {
        const auto kv = cur.as<op_list_keyval>();
        if (GenTree::get_actual_value(kv->key())->type() != op_string) {
          const TypeData *key_type = tinf::get_type(kv->key());
          kphp_error(0, fmt_format("Only string const keys can be used, got '{}'", ptype_name(key_type->ptype())));
        }
      }
    } else {
      kphp_error (arrayType->ptype() == tp_mixed, fmt_format("Can not compile list with '{}'", ptype_name(arrayType->ptype())));
    }
  }
  if (vertex->type() == op_index && vertex.as<op_index>()->has_key()) {
    VertexPtr arr = vertex.as<op_index>()->array();
    VertexPtr key = vertex.as<op_index>()->key();
    const TypeData *array_type = tinf::get_type(arr);
    // TODO: do we need this?
    if (array_type->ptype() == tp_tuple) {
      long index = parse_int_from_string(GenTree::get_actual_value(key).as<op_int_const>());
      size_t tuple_size = array_type->get_tuple_max_index();
      kphp_error (0 <= index && index < tuple_size, fmt_format("Can't get element {} of tuple of length {}", index, tuple_size));
    }
    const TypeData *key_type = tinf::get_type(key);
    kphp_error(key_type->ptype() != tp_any || key_type->or_false_flag(),
               fmt_format("Can't get array element by key with {} type", colored_type_out(key_type)));
  }
  if (auto xset = vertex.try_as<meta_op_xset>()) {
    auto v = xset->expr();
    if (auto var_vertex = v.try_as<op_var>()) {    // isset($var), unset($var)
      VarPtr var = var_vertex->var_id;
      if (vertex->type() == op_unset) {
        kphp_error(!var->is_reference, "Unset of reference variables is not supported");
        if (var->is_in_global_scope()) {
          FunctionPtr f = stage::get_function();
          if (!f->is_main_function() && f->type != FunctionData::func_switch) {
            kphp_error(0, "Unset of global variables in functions is not supported");
          }
        }
      } else {
        kphp_error(!var->is_constant(), "Can't use isset on const variable");
        const TypeData *type_info = tinf::get_type(var);
        kphp_error(type_info->can_store_null(),
                   fmt_format("isset({}) will be always true for {}", var->get_human_readable_name(), colored_type_out(type_info)));
      }
    } else if (v->type() == op_index) {   // isset($arr[index]), unset($arr[index])
      const TypeData *arrayType = tinf::get_type(v.as<op_index>()->array());
      PrimitiveType ptype = arrayType->get_real_ptype();
      kphp_error(vk::any_of_equal(ptype, tp_tuple, tp_shape, tp_array, tp_mixed), "Can't use isset/unset by[idx] for not an array");
    }
  }
  if (vertex->type() == op_func_call) {
    check_op_func_call(vertex.as<op_func_call>());
  }
  if (vertex->type() == op_return && current_function->is_no_return) {
    kphp_error(false, "Return is done from no return function");
  }
  if (current_function->can_throw() && current_function->is_no_return) {
    kphp_error(false, "Exception is thrown from no return function");
  }
  if (vertex->type() == op_instance_prop) {
    const TypeData *lhs_type = tinf::get_type(vertex.as<op_instance_prop>()->instance());
    kphp_error(lhs_type->ptype() == tp_Class,
               fmt_format("Accessing ->property of non-instance {}", colored_type_out(lhs_type)));
  }

  if (vertex->type() == op_throw) {
    const TypeData *thrown_type = tinf::get_type(vertex.as<op_throw>()->exception());
    kphp_error(thrown_type->ptype() == tp_Class && G->get_class("Throwable")->is_parent_of(thrown_type->class_type()),
               fmt_format("Throw not Throwable, but {}", colored_type_out(thrown_type)));
  }

  if (vertex->type() == op_fork) {
    const VertexAdaptor<op_func_call> &func_call = vertex.as<op_fork>()->func_call();
    kphp_error(!func_call->func_id->is_extern(), "fork of builtin function is forbidden");
  }

  if (G->settings().warnings_level.get() >= 2 && vertex->type() == op_func_call) {
    FunctionPtr function_where_require = stage::get_function();

    if (function_where_require && function_where_require->type == FunctionData::func_local) {
      FunctionPtr function_which_required = vertex.as<op_func_call>()->func_id;
      if (function_which_required->is_main_function()) {
        for (VarPtr global_var : function_which_required->global_var_ids) {
          if (!global_var->marked_as_global) {
            kphp_warning(fmt_format("require file with global variable not marked as global: {}", global_var->name));
          }
        }
      }
    }
  }

  if (auto binary_vertex = vertex.try_as<meta_op_binary>()) {
    check_null_usage_in_binary_operations(binary_vertex);
  }

  //TODO: may be this should be moved to tinf_check
  return vertex;
}

VertexPtr FinalCheckPass::on_exit_vertex(VertexPtr vertex) {
  return vertex;
}

void FinalCheckPass::check_op_func_call(VertexAdaptor<op_func_call> call) {
  if (call->func_id->is_extern()) {
    auto &function_name = call->get_string();
    if (function_name == "instance_cache_fetch") {
      check_instance_cache_fetch_call(call);
    } else if (function_name == "instance_cache_store") {
      check_instance_cache_store_call(call);
    } else if (function_name == "instance_to_array") {
      check_instance_to_array_call(call);
    } else if (function_name == "estimate_memory_usage") {
      check_estimate_memory_usage_call(call);
    } else if (function_name == "get_global_vars_memory_stats") {
      check_get_global_vars_memory_stats_call();
    } else if (function_name == "is_null") {
      const TypeData *arg_type = tinf::get_type(call->args()[0]);
      kphp_error(arg_type->can_store_null(),
                 fmt_format("is_null() will be always false for {}", colored_type_out(arg_type)));
    } else if (vk::string_view{function_name}.starts_with("rpc_tl_query")) {
      G->set_untyped_rpc_tl_used();
    }

    // TODO: express the array<Comparable> requirement in functions.txt and remove these adhoc checks?
    bool is_value_sort_function = vk::any_of_equal(function_name, "sort", "rsort", "asort", "arsort");
    if (is_value_sort_function) {
      // Forbid arrays with elements that would be rejected by check_comparisons().
      const TypeData *array_type = tinf::get_type(call->args()[0]);
      auto *elem_type = array_type->lookup_at(Key::any_key());
      kphp_error(vk::none_of_equal(elem_type->ptype(), tp_Class, tp_tuple, tp_shape),
                 fmt_format("{} is not comparable and cannot be sorted", colored_type_out(elem_type)));
    }
  }

  check_func_call_params(call);
}

// Inspection: static-var should be initialized at the declaration (with the exception of tp_mixed).
inline void FinalCheckPass::check_static_var_inited(VarPtr static_var) {
  kphp_error(static_var->init_val || tinf::get_type(static_var)->ptype() == tp_mixed,
             fmt_format("static ${} is not inited at declaration (inferred {})", static_var->name, colored_type_out(tinf::get_type(static_var))));
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

void FinalCheckPass::check_eq3_neq3(VertexPtr lhs, VertexPtr rhs, Operation op) {
  auto lhs_type = tinf::get_type(lhs);
  auto rhs_type = tinf::get_type(rhs);

  // we only check the ===, but !== would also make sense;
  // but it results in a lot of warnings, so we disable it for now
  if (op == op_eq3) {
    if ((lhs_type->ptype() == tp_float && !lhs_type->or_false_flag() && !lhs_type->or_null_flag()) ||
        (rhs_type->ptype() == tp_float && !rhs_type->or_false_flag() && !rhs_type->or_null_flag())) {
      kphp_warning("Using === with float operand");
    }
    if (!can_be_same_type(lhs_type, rhs_type)) {
      kphp_warning(fmt_format("{} === {} is always false", type_out(lhs_type), type_out(rhs_type)));
    }
  }

  // instance can be compared with other instances (reference comparison) or nulls
  if (vk::any_of_equal(tp_Class, lhs_type->ptype(), rhs_type->ptype())) {
    auto cmp_type = lhs_type->ptype() == tp_Class ? rhs_type : lhs_type;
    bool cmp_type_is_null = cmp_type->ptype() == tp_any && (cmp_type->or_false_flag() || cmp_type->or_null_flag());
    kphp_error(cmp_type->ptype() == tp_Class || cmp_type_is_null, fmt_format("instance {} {} is a strange operation", OpInfo::desc(op), colored_type_out(cmp_type)));
  }
}

void FinalCheckPass::check_comparisons(VertexPtr lhs, VertexPtr rhs, Operation op) {
  if (vk::none_of_equal(tinf::get_type(lhs)->ptype(), tp_Class, tp_array, tp_tuple, tp_shape)) {
    std::swap(lhs, rhs);
  }

  auto lhs_t = tinf::get_type(lhs);
  auto rhs_t = tinf::get_type(rhs);

  if (lhs_t->ptype() == tp_Class) {
    if (vk::any_of_equal(op, op_eq2, op_neq2)) {
      kphp_error(false, fmt_format("instance {} {} is unsupported", OpInfo::desc(op), colored_type_out(rhs_t)));
    } else {
      kphp_error(false, fmt_format("comparison instance with {} is prohibited (operation: {})", colored_type_out(rhs_t), OpInfo::desc(op)));
    }
  } else if (lhs_t->ptype() == tp_array) {
    kphp_error(vk::any_of_equal(rhs_t->get_real_ptype(), tp_array, tp_bool, tp_mixed),
               fmt_format("{} is always > than {} used operator {}", colored_type_out(lhs_t), colored_type_out(rhs_t), OpInfo::desc(op)));
  } else if (lhs_t->ptype() == tp_tuple) {
    bool can_compare_with_tuple = vk::any_of_equal(op, op_eq2, op_neq2) &&
                                  rhs_t->ptype() == tp_tuple && lhs_t->get_tuple_max_index() == rhs_t->get_tuple_max_index();
    kphp_error(can_compare_with_tuple,
               fmt_format("You may not compare {} with {} used operator {}", colored_type_out(lhs_t), colored_type_out(rhs_t), OpInfo::desc(op)));
  } else if (lhs_t->ptype() == tp_shape) {
    // shape can't be compared with anything using ==, it's meaningless
    kphp_error(0,
               fmt_format("You may not compare {} with {} used operator {}", colored_type_out(lhs_t), colored_type_out(rhs_t), OpInfo::desc(op)));
  }

}

bool FinalCheckPass::user_recursion(VertexPtr v) {
  if (v->type() == op_function) {
    run_function_pass(v.as<op_function>()->cmd_ref(), this);
    return true;
  }

  if (vk::any_of_equal(v->type(), op_func_call, op_var, op_index, op_conv_drop_false, op_conv_drop_null)) {
    if (v->rl_type == val_r) {
      const TypeData *type = tinf::get_type(v);
      if (type->get_real_ptype() == tp_any) {
        raise_error_using_Unknown_type(v);
        return true;
      }
    }
  }

  return false;
}

void FinalCheckPass::raise_error_using_Unknown_type(VertexPtr v) {
  std::string index_depth;
  while (auto v_index = v.try_as<op_index>()) {
    v = v_index->array();
    index_depth += "[*]";
  }

  while (vk::any_of_equal(v->type(), op_conv_drop_false, op_conv_drop_null)) {
    v = v.try_as<meta_op_unary>()->expr();
  }

  if (auto var_vertex = v.try_as<op_var>()) {
    VarPtr var = var_vertex->var_id;
    if (index_depth.empty()) {              // Unknown type single var access
      kphp_error(0, fmt_format("Variable ${} has Unknown type", var->name));
    } else if (index_depth.size() == 3) {   // 1-depth array[*] access (most common case)

      if (vk::any_of_equal(tinf::get_type(var)->get_real_ptype(), tp_array, tp_mixed)) {
        kphp_error(0, fmt_format("Array ${} is always empty, getting its element has Unknown type", var->name));
      } else if (tinf::get_type(var)->get_real_ptype() == tp_shape) {
        kphp_error(0, fmt_format("Accessing unexisting element of shape ${}", var->name));
      } else {
        kphp_error(0, fmt_format("${} is {}, can not get element", var->name, colored_type_out(tinf::get_type(var))));
      }

    } else {                                // multidimentional array[*]...[*] access
      kphp_error(0, fmt_format("${}{} has Unknown type", var->name, index_depth));
    }
  } else if (auto call = v.try_as<op_func_call>()) {
    if (index_depth.empty()) {
      kphp_error(0, fmt_format("Function {}() returns Unknown type", call->func_id->name));
    } else {
      kphp_error(0, fmt_format("{}(){} has Unknown type", call->func_id->name, index_depth));
    }
  } else {
    kphp_error(0, "Using Unknown type");
  }
}
