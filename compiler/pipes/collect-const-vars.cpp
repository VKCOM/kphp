// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/collect-const-vars.h"

#include "compiler/data/src-file.h"
#include "compiler/vertex-util.h"
#include "compiler/data/var-data.h"
#include "compiler/compiler-core.h"
#include "compiler/name-gen.h"

int CollectConstVarsPass::get_dependency_level(VertexPtr vertex) {
  if (auto var = vertex.try_as<op_var>()) {
    return var->var_id->dependency_level;
  }
  int max_dependency_level = 0;
  for (const auto &child: *vertex) {
    max_dependency_level = std::max(max_dependency_level, get_dependency_level(child));
  }
  return max_dependency_level;
}
VertexPtr CollectConstVarsPass::on_exit_vertex(VertexPtr root) {
  VertexPtr res = root;
  if (auto as_def = res.try_as<op_define_val>()) {
    res = as_def->value();
  }
  if (root->const_type == cnst_const_val) {
    const_array_depth_ -= root->type() == op_array;
    if (should_convert_to_const(root)) {
      res = create_const_variable(root, root->location);
    }
  }

  in_param_list_ -= root->type() == op_func_param_list;
  return res;
}

static bool is_underlying_array_or_string_or_func_call(VertexPtr root) {
  auto real_type = root->type();
  if (auto as_define_val = root.try_as<op_define_val>())  {
    real_type = as_define_val->value()->type();
  }
  return vk::any_of_equal(real_type, op_array, op_string, op_func_call);
}

VertexPtr CollectConstVarsPass::on_enter_vertex(VertexPtr root) {
  in_param_list_ += root->type() == op_func_param_list;

  if (vk::any_of_equal(root->type(), op_defined)) {
    return root;
  }

  if (root->const_type == cnst_const_val) {
    if (root->type() == op_conv_regexp) {
      VertexPtr expr = VertexUtil::get_actual_value(root.as<op_conv_regexp>()->expr());
      if (vk::any_of_equal(expr->type(), op_string, op_concat, op_string_build)) {
        return create_const_variable(root, root->location);
      }
    }
    if (!is_underlying_array_or_string_or_func_call(root) && should_convert_to_const(root)) {
      return create_const_variable(root, root->location);
    }
    const_array_depth_ += root->type() == op_array;
  }
  return root;
}

bool CollectConstVarsPass::should_convert_to_const(VertexPtr root) {
  auto real_type = root->type();
  if (auto as_defined_val = root.try_as<op_define_val>()) {
    real_type = as_defined_val->value()->type();
  }
  auto inner = VertexUtil::get_value_if_inlined(root);
  if (auto as_func_call = inner.try_as<op_func_call>()) {
    const bool is_pure = as_func_call->func_id && as_func_call->func_id->is_pure;
    const bool is_inlined_const_object = as_func_call->func_id && root->type() == op_define_val && as_func_call->func_id->is_constructor();
    return is_pure || is_inlined_const_object;
  }
  return vk::any_of_equal(real_type, op_string, op_array, op_concat, op_string_build);
}

VertexPtr CollectConstVarsPass::create_const_variable(VertexPtr root, Location loc) {
  std::string name;

  VertexPtr real = root;
  if (real->type() == op_define_val) {
    real = VertexUtil::get_value_if_inlined(root);
  }

  if (real->type() == op_string) {
    name = gen_const_string_name(real.as<op_string>()->str_val);
  } else if (real->type() == op_conv_regexp && real.as<op_conv_regexp>()->expr()->type() == op_string) {
    name = gen_const_regexp_name(real.as<op_conv_regexp>()->expr().as<op_string>()->str_val);
  } else if (is_array_suitable_for_hashing(real)) {
    name = gen_const_array_name(real.as<op_array>());
  } else if (is_object_suitable_for_hashing(root)) {
    name = gen_const_object_name(root.as<op_define_val>());
  } else {
    name = gen_unique_name("const_var");
  }
  root = real;

  auto var = VertexAdaptor<op_var>::create();
  var->str_val = name;
  var->extra_type = op_ex_var_const;
  var->location = loc;

  VarPtr var_id = G->get_global_var(name, VarData::var_const_t, root);
  if (root->type() == op_array) {
    int max_dep_level = 1;
    for (auto it : root.as<op_array>()->args()) {
      max_dep_level = std::max(max_dep_level, get_dependency_level(it) + 1);
    }
    var_id->dependency_level = max_dep_level;
  } else if (auto as_func_call = root.try_as<op_func_call>(); as_func_call && as_func_call->func_id && as_func_call->func_id->is_constructor()) {
    int max_dep_level = 1;
    for (auto it : *as_func_call) {
      max_dep_level = std::max(max_dep_level, get_dependency_level(it) + 1);
    }
    var_id->dependency_level = max_dep_level;
  } else {
    var_id->dependency_level = 0;
  }

  if (const_array_depth_ > 0) {
    current_function->implicit_const_var_ids.insert(var_id);
  } else {
    if (in_param_list_ > 0) {
      current_function->explicit_header_const_var_ids.insert(var_id);
    } else {
      current_function->explicit_const_var_ids.insert(var_id);
    }
  }

  var->var_id = var_id;
  return var;
}

bool CollectConstVarsPass::user_recursion(VertexPtr v) {
  if (v->type() == op_function) {
    if (current_function->type == FunctionData::func_class_holder) {
      ClassPtr c = current_function->class_id;
      c->members.for_each([&](ClassMemberInstanceField &field) {
        if (field.var->init_val) {
          run_function_pass(field.var->init_val, this);
        }
      });
      c->members.for_each([&](ClassMemberStaticField &field) {
        if (field.var->init_val) {
          run_function_pass(field.var->init_val, this);
        }
      });
    }
  }
  return v->type() == op_defined;
}
