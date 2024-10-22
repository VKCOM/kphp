// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/optimization.h"

#include "auto/compiler/vertex/vertex-types.h"
#include "compiler/inferring/primitive-type.h"
#include "compiler/inferring/type-data.h"
#include "compiler/kphp_assert.h"
#include "compiler/operation.h"
#include <sstream>

#include "common/algorithms/hashes.h"

#include "compiler/data/class-data.h"
#include "compiler/vertex-util.h"
#include "compiler/inferring/public.h"

namespace {

bool can_init_value_be_removed(VertexPtr init_value, const VarPtr &variable) {
  const auto *variable_type = tinf::get_type(variable);
  if (variable_type->use_optional() ||
      vk::any_of_equal(variable_type->ptype(), tp_Class, tp_mixed)) {
    return init_value->type() == op_null;
  }

  const auto *init_type = tinf::get_type(init_value);
  if (init_value->extra_type != op_ex_var_const ||
      init_type->use_optional() ||
      init_type->ptype() != variable_type->ptype()) {
    return false;
  }

  switch (init_type->ptype()) {
    case tp_string: {
      const auto *init_string = VertexUtil::get_constexpr_string(init_value);
      return init_string && init_string->empty();
    }
    case tp_array:
      return init_type->lookup_at_any_key()->get_real_ptype() == tp_any;
    default:
      return false;
  }
}

VarPtr cast_const_array_type(VertexPtr &type_acceptor, const TypeData *required_type) noexcept {
  std::stringstream ss;
  ss << type_acceptor->get_string() << "$" << std::hex << vk::std_hash(type_out(required_type));
  std::string name = ss.str();
  bool is_new = true;
  VarPtr var_id = G->get_constant_var(name, type_acceptor, &is_new);
  var_id->tinf_node.copy_type_from(required_type);  // not inside if(is_new) to avoid race conditions when one thread creates and another uses faster
  if (is_new) {
    var_id->dependency_level = type_acceptor.as<op_var>()->var_id->dependency_level + 1;
  }

  auto casted_var = VertexAdaptor<op_var>::create();
  casted_var->str_val = std::move(name);
  casted_var->extra_type = op_ex_var_const;
  casted_var->location = type_acceptor->location;

  casted_var->var_id = var_id;
  type_acceptor = casted_var;
  return var_id;
}

void cast_array_creation_type(VertexAdaptor<op_array> op_array_vertex, const TypeData *required_type) noexcept {
  if (required_type->get_real_ptype() == tp_mixed) {
    required_type = TypeData::get_type(tp_array, tp_mixed);
  } else if (required_type->use_optional()) {
    required_type = TypeData::create_array_of(required_type->lookup_at_any_key());
  }
  op_array_vertex->tinf_node.set_type(required_type);
}

void explicit_cast_array_type(VertexPtr &type_acceptor, const TypeData *required_type, std::set<VarPtr> *new_var_out = nullptr) noexcept {
  const auto *existed_type = tinf::get_type(type_acceptor);
  if (existed_type->get_real_ptype() != tp_array ||
      !is_implicit_array_conversion(existed_type, required_type)) {
    return;
  }
  if (!vk::any_of_equal(required_type->get_real_ptype(), tp_array, tp_mixed)) {
    return;
  }
  if (type_acceptor->extra_type == op_ex_var_const) {
    auto var_id = cast_const_array_type(type_acceptor, required_type);
    if (new_var_out) {
      new_var_out->emplace(var_id);
    }
  }
  if (auto op_array_vertex = type_acceptor.try_as<op_array>()) {
    cast_array_creation_type(op_array_vertex, required_type);
  }
}

} // namespace

VertexPtr OptimizationPass::optimize_set_push_back(VertexAdaptor<op_set> set_op) {
  // TODO embed here
  if (set_op->lhs()->type() != op_index) {
    return set_op;
  }
  VertexAdaptor<op_index> index = set_op->lhs().as<op_index>();
  if (index->has_key() && set_op->rl_type != val_none) {
    return set_op;
  }

  VertexPtr a, b, c;
  a = index->array();
  if (index->has_key()) {
    b = index->key();
  }
  c = set_op->rhs();

  VertexPtr result;

  if (!b) {
    // '$s[] = ...' is forbidden for non-array types;
    // for arrays it's converted to push_back
    PrimitiveType a_ptype = tinf::get_type(a)->get_real_ptype();
    kphp_error (a_ptype == tp_array || a_ptype == tp_mixed,
                fmt_format("Can not use [] for {}", type_out(tinf::get_type(a))));

    if (set_op->rl_type == val_none) {
      result = VertexAdaptor<op_push_back>::create(a, c);
    } else {
      result = VertexAdaptor<op_push_back_return>::create(a, c);
    }
  } else {
    PrimitiveType a_ptype = tinf::get_type(a)->get_real_ptype();
    if (a_ptype == tp_Class) {
      auto klass = tinf::get_type(a)->class_type();
      kphp_assert_msg(klass, "bad klass");

      const auto *method = klass->get_instance_method("offsetSet");

      kphp_assert_msg(klass, "bad method");


      // TODO assume here that key is present
      auto new_call = VertexAdaptor<op_func_call>::create(a, b, c).set_location(set_op->get_location());
      
      new_call->str_val = method->global_name();
      new_call->func_id = method->function;
      new_call->extra_type = op_ex_func_call_arrow; // Is that right?
      new_call->auto_inserted = true;
      new_call->rl_type = set_op->rl_type;
      
      result = new_call;
      // As I see it's reduntant, but I do not understand why
      current_function->dep.emplace_back(method->function);

      return result;
    } else {
      result = VertexAdaptor<op_set_value>::create(a, b, c);
    }

  }
  result->location = set_op->get_location();
  result->extra_type = op_ex_internal_func;
  result->rl_type = set_op->rl_type;
  return result;
}
void OptimizationPass::collect_concat(VertexPtr root, std::vector<VertexPtr> *collected) {
  if (root->type() == op_string_build || root->type() == op_concat) {
    for (auto i : *root) {
      collect_concat(i, collected);
    }
  } else {
    if (const auto call = try_convert_expr_to_call_to_string_method(root)) {
      root = call;
    }
    collected->push_back(root);
  }
}
VertexPtr OptimizationPass::optimize_string_building(VertexPtr root) {
  std::vector<VertexPtr> collected;
  collect_concat(root, &collected);
  auto new_root = VertexAdaptor<op_string_build>::create(collected);
  new_root->location = root->get_location();
  new_root->rl_type = root->rl_type;

  return new_root;
}
VertexPtr OptimizationPass::optimize_postfix_inc(VertexPtr root) {
  if (root->rl_type == val_none) {
    auto new_root = VertexAdaptor<op_prefix_inc>::create(root.as<op_postfix_inc>()->expr());
    new_root->rl_type = root->rl_type;
    new_root->location = root->get_location();
    root = new_root;
  }
  return root;
}
VertexPtr OptimizationPass::optimize_postfix_dec(VertexPtr root) {
  if (root->rl_type == val_none) {
    auto new_root = VertexAdaptor<op_prefix_dec>::create(root.as<op_postfix_dec>()->expr());
    new_root->rl_type = root->rl_type;
    new_root->location = root->get_location();
    root = new_root;
  }
  return root;
}
[[clang::optnone]]
VertexPtr OptimizationPass::optimize_index(VertexAdaptor<op_index> index) {
  if (!index->has_key()) {
    if (index->rl_type == val_l) {
      kphp_error (0, "Unsupported []");
    } else {
      kphp_error (0, "Cannot use [] for reading");
    }
  }

  auto &lhs = index->array();
  const auto *tpe = tinf::get_type(index->array()); // funny
  if (tpe->get_real_ptype() == tp_Class) {
    auto klass = tpe->class_type();
    kphp_assert_msg(klass, "bad klass");

    const auto *method = klass->get_instance_method("offsetGet");
    if (!method) {
      kphp_assert_msg(method, "bad method");
    }
    // TODO assume here that key is present
    auto new_call = VertexAdaptor<op_func_call>::create(lhs, index->key()).set_location(lhs);
    new_call->str_val = method->global_name();
    new_call->func_id = method->function;
    new_call->extra_type = op_ex_func_call_arrow; // Is that right?
    new_call->auto_inserted = true;
    new_call->rl_type = index->rl_type;

    current_function->dep.emplace_back(method->function);

    // For interfaces, I should construct type node here?
    auto &node = new_call->tinf_node;
    auto * tdata = new TypeData(*method->function->tinf_node.get_type());
    auto xxx = method->function->tinf_node;
    node.set_type(tdata);
    tinf::get_type(new_call); // why OK for LikeArray, but bad for array access

    return new_call;
  }

  return index;
}
VertexPtr OptimizationPass::remove_extra_conversions(VertexPtr root) {
  if (auto c2php = root.try_as<op_ffi_c2php_conv>()) {
    if (c2php->rl_type == val_none) {
      return c2php->expr();
    }
  }

  while (OpInfo::type(root->type()) == conv_op || vk::any_of_equal(root->type(), op_conv_array_l, op_conv_int_l, op_conv_string_l)) {
    VertexPtr expr = root.as<meta_op_unary>()->expr();
    const TypeData *tp = tinf::get_type(expr);
    VertexPtr res;
    if (!tp->use_optional()) {
      if (vk::any_of_equal(root->type(), op_conv_int, op_conv_int_l) && tp->ptype() == tp_int) {
        res = expr;
      } else if (root->type() == op_conv_bool && tp->ptype() == tp_bool) {
        res = expr;
      } else if (root->type() == op_conv_float && tp->ptype() == tp_float) {
        res = expr;
      } else if (vk::any_of_equal(root->type(), op_conv_string, op_conv_string_l) && tp->ptype() == tp_string) {
        res = expr;
      } else if (vk::any_of_equal(root->type(), op_conv_array, op_conv_array_l) && tp->get_real_ptype() == tp_array) {
        res = expr;
      } else if (root->type() == op_force_mixed && tp->ptype() == tp_void) {
          expr->rl_type = val_none;
          res = VertexAdaptor<op_seq_rval>::create(expr, VertexAdaptor<op_null>::create());
      }
    }
    if (root->type() == op_conv_mixed) {
      res = expr;
    }
    if (root->type() == op_conv_drop_null){
      if (!tp->can_store_null()) {
        res = expr;
      }
    }
    if (root->type() == op_conv_drop_false){
      if (!tp->can_store_false()) {
        res = expr;
      }
    }
    if (res) {
      res->rl_type = root->rl_type;
      root = res;
    } else {
      break;
    }
  }
  return root;
}

// The function will try to convert the expression to a call to the
// __toString method if the expression has a class type and the class
// has such a method.
// If the conversion fails, an empty VertexPtr will be returned.
VertexPtr OptimizationPass::try_convert_expr_to_call_to_string_method(VertexPtr expr) {
  const auto *type = tinf::get_type(expr);
  if (type == nullptr) {
    return {};
  }

  const auto klass = type->class_type();
  if (!klass) {
    return {};
  }

  const auto *to_string_method = klass->get_instance_method(ClassData::NAME_OF_TO_STRING);
  kphp_error_act(to_string_method,
                 fmt_format("Converting to a string of a class {} that does not contain a __toString() method", klass->as_human_readable()),
                 return {});

  auto call_function = VertexAdaptor<op_func_call>::create(expr);
  call_function->set_string(std::string{to_string_method->local_name()});
  call_function->func_id = to_string_method->function;

  return call_function;
}

VertexPtr OptimizationPass::convert_strval_to_magic_tostring_method_call(VertexAdaptor<op_conv_string> conv) {
  const auto expr = conv->expr();

  if (const auto call = try_convert_expr_to_call_to_string_method(expr)) {
    return call;
  }

  return conv;
}

VertexPtr OptimizationPass::on_enter_vertex(VertexPtr root) {
  root = remove_extra_conversions(root);

  if (auto set_vertex = root.try_as<op_set>()) {
    explicit_cast_array_type(set_vertex->rhs(), tinf::get_type(set_vertex->lhs()), &current_function->explicit_const_var_ids);
    root = optimize_set_push_back(set_vertex);
  } else if (root->type() == op_string_build || root->type() == op_concat) {
    root = optimize_string_building(root);
  } else if (root->type() == op_postfix_inc) {
    root = optimize_postfix_inc(root);
  } else if (root->type() == op_postfix_dec) {
    root = optimize_postfix_dec(root);
  } else if (root->type() == op_index) {
    root = optimize_index(root.as<op_index>());
  } else if (auto param = root.try_as<op_foreach_param>()) {
    if (!param->x()->ref_flag) {
      auto temp_var = root.as<op_foreach_param>()->temp_var().as<op_var>();
      if (temp_var && temp_var->extra_type == op_ex_var_superlocal) {     // see CreateSwitchForeachVarsPass
        temp_var->var_id->needs_const_iterator_flag = true;
      }
    }
  } else if (auto func_param = root.try_as<op_func_param>()) {
    if (func_param->has_default_value() && func_param->default_value()) {
      explicit_cast_array_type(func_param->default_value(), tinf::get_type(func_param->var()), &current_function->explicit_header_const_var_ids);
    }
  } else if (auto func_call = root.try_as<op_func_call>()) {
    auto func = func_call->func_id;
    if (!func->is_extern()) {
      auto args = func_call->args();
      const auto &params = func->param_ids;
      const size_t elements = std::min(static_cast<size_t>(args.size()), params.size());
      for (size_t index = 0; index < elements; ++index) {
        explicit_cast_array_type(args[index], tinf::get_type(params[index]), &current_function->explicit_const_var_ids);
      }
    }
  } else if (auto op_array_vertex = root.try_as<op_array>()) {
    if (!var_init_expression_optimization_depth_) {
      for (auto &array_element : *op_array_vertex) {
        const auto *required_type = tinf::get_type(op_array_vertex)->lookup_at_any_key();
        if (vk::any_of_equal(array_element->type(), op_var, op_array)) {
          explicit_cast_array_type(array_element, required_type, &current_function->explicit_const_var_ids);
        } else if (auto array_key_value = array_element.try_as<op_double_arrow>()) {
          explicit_cast_array_type(array_key_value->value(), required_type, &current_function->explicit_const_var_ids);
        }
      }
    }
  } else if (auto op_return_vertex = root.try_as<op_return>()) {
    if (op_return_vertex->has_expr()) {
      explicit_cast_array_type(op_return_vertex->expr(), tinf::get_type(current_function, -1), &current_function->explicit_const_var_ids);
    }
  } else if (auto op_conv_string_vertex = root.try_as<op_conv_string>()) {
    root = convert_strval_to_magic_tostring_method_call(op_conv_string_vertex);
  }

  if (root->rl_type != val_none/* && root->rl_type != val_error*/) {
    // wtf?
    tinf::get_type(root);
  }
  return root;
}

bool OptimizationPass::user_recursion(VertexPtr root) {
  if (auto var_vertex = root.try_as<op_var>()) {
    VarPtr var = var_vertex->var_id;
    kphp_assert (var);
    if (var->init_val) {
      if (__sync_bool_compare_and_swap(&var->optimize_flag, false, true)) {
        ++var_init_expression_optimization_depth_;
        run_function_pass(var->init_val, this);
        kphp_assert(var_init_expression_optimization_depth_ > 0);
        --var_init_expression_optimization_depth_;
        if (!var->is_constant()) {
          explicit_cast_array_type(var->init_val, tinf::get_type(var));
        }
      }
    }
  }
  return false;
}

bool OptimizationPass::check_function(FunctionPtr function) const {
  return !function->is_extern();
}

void OptimizationPass::on_finish() {
  if (current_function->type == FunctionData::func_class_holder) {
    auto class_id = current_function->class_id;
    class_id->members.for_each([this](ClassMemberInstanceField &class_field) {
      if (class_field.var->init_val) {
        run_function_pass(class_field.var->init_val, this);

        if (can_init_value_be_removed(class_field.var->init_val, class_field.var)) {
          class_field.var->init_val = {};
          class_field.var->had_user_assigned_val = true;
        } else {
          explicit_cast_array_type(class_field.var->init_val, tinf::get_type(class_field.var));
        }
      }
    });

    class_id->members.for_each([this](ClassMemberStaticField &static_field) {
      if (static_field.var->init_val) {
        run_function_pass(static_field.var->init_val, this);
      }
    });
  }
}
