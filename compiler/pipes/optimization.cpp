// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/optimization.h"

#include <sstream>

#include "common/algorithms/hashes.h"

#include "compiler/data/class-data.h"
#include "compiler/gentree.h"
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
      const auto *init_string = GenTree::get_constexpr_string(init_value);
      return init_string && init_string->empty();
    }
    case tp_array:
      return init_type->lookup_at(Key::any_key())->get_real_ptype() == tp_Unknown;
    default:
      return false;
  }
}

template<typename T>
VarPtr cast_const_array(VertexPtr &type_acceptor, const T &type_donor) {
  auto required_type = tinf::get_type(type_donor);
  auto existed_type = tinf::get_type(type_acceptor);
  if (existed_type->get_real_ptype() != tp_array ||
      type_acceptor->extra_type != op_ex_var_const ||
      are_equal_types(existed_type, required_type)) {
    return VarPtr{};
  }

  kphp_assert(vk::any_of_equal(required_type->get_real_ptype(), tp_array, tp_mixed));
  std::stringstream ss;
  ss << type_acceptor->get_string() << "$" << std::hex << vk::std_hash(type_out(required_type));
  std::string name = ss.str();
  bool is_new = true;
  VarPtr var_id  = G->get_global_var(name, VarData::var_const_t, type_acceptor, &is_new);
  if (is_new) {
    var_id->dependency_level = type_acceptor.as<op_var>()->var_id->dependency_level + 1;
    var_id->tinf_node.copy_type_from(required_type);
  }

  auto casted_var = VertexAdaptor<op_var>::create();
  casted_var->str_val = std::move(name);
  casted_var->extra_type = op_ex_var_const;
  casted_var->location = type_acceptor->location;

  casted_var->var_id = var_id;
  type_acceptor = casted_var;
  return var_id;
}
} // namespace

VertexPtr OptimizationPass::optimize_set_push_back(VertexAdaptor<op_set> set_op) {
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
    result = VertexAdaptor<op_set_value>::create(a, b, c);
  }
  result->location = set_op->get_location();
  result->extra_type = op_ex_internal_func;
  result->rl_type = set_op->rl_type;
  return result;
}
void OptimizationPass::collect_concat(VertexPtr root, vector<VertexPtr> *collected) {
  if (root->type() == op_string_build || root->type() == op_concat) {
    for (auto i : *root) {
      collect_concat(i, collected);
    }
  } else {
    collected->push_back(root);
  }
}
VertexPtr OptimizationPass::optimize_string_building(VertexPtr root) {
  vector<VertexPtr> collected;
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
VertexPtr OptimizationPass::optimize_index(VertexAdaptor<op_index> index) {
  if (!index->has_key()) {
    if (index->rl_type == val_l) {
      kphp_error (0, "Unsupported []");
    } else {
      kphp_error (0, "Cannot use [] for reading");
    }
  }
  return index;
}
VertexPtr OptimizationPass::remove_extra_conversions(VertexPtr root) {
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

VertexPtr OptimizationPass::on_enter_vertex(VertexPtr root) {
  root = remove_extra_conversions(root);

  if (root->type() == op_set) {
    root = optimize_set_push_back(root.as<op_set>());
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
  }

  if (root->rl_type != val_none/* && root->rl_type != val_error*/) {
    tinf::get_type(root);
  }
  return root;
}

VertexPtr OptimizationPass::on_exit_vertex(VertexPtr root) {
  if (auto param = root.try_as<op_func_param>()) {
    if (param->has_default_value() && param->default_value()) {
      if (auto var_id = cast_const_array(param->default_value(), param->var())) {
        current_function->explicit_header_const_var_ids.emplace(var_id);
      }
    }
  } else if (auto set_vertex = root.try_as<op_set>()) {
    if (auto var_id = cast_const_array(set_vertex->rhs(), set_vertex->lhs())) {
      current_function->explicit_const_var_ids.emplace(var_id);
    }
  } else if (auto func_call = root.try_as<op_func_call>()) {
    auto func = func_call->func_id;
    if (!func->has_variadic_param && !func->is_extern()) {
      auto args = func_call->args();
      const auto &params = func->param_ids;
      for (size_t index = 0; index < args.size(); ++index) {
        if (auto var_id = cast_const_array(args[index], params[index])) {
          current_function->explicit_const_var_ids.emplace(var_id);
        }
      }
    }
  }
  return root;
}

bool OptimizationPass::user_recursion(VertexPtr root) {
  if (auto var_vertex = root.try_as<op_var>()) {
    VarPtr var = var_vertex->var_id;
    kphp_assert (var);
    if (var->init_val) {
      if (try_optimize_var(var)) {
        run_function_pass(var->init_val, this);
        cast_const_array(var->init_val, var);
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
    class_id->members.for_each([](ClassMemberInstanceField &class_field) {
      if (class_field.var->init_val) {
        if (can_init_value_be_removed(class_field.var->init_val, class_field.var)) {
          class_field.var->init_val = {};
        } else {
          cast_const_array(class_field.var->init_val, class_field.var);
        }
      }
    });
  }
}
