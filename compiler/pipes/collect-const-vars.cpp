#include "compiler/pipes/collect-const-vars.h"

#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"

int CollectConstVarsPass::get_dependency_level(VertexPtr vertex) {
  if (vertex->type() == op_var) {
    return vertex->get_var_id()->dependency_level;
  }
  int max_dependency_level = 0;
  for (const auto &child: *vertex) {
    max_dependency_level = std::max(max_dependency_level, get_dependency_level(child));
  }
  return max_dependency_level;
}
VertexPtr CollectConstVarsPass::on_exit_vertex(VertexPtr root, LocalT *) {
  VertexPtr res = root;

  if (root->type() == op_define_val) {
    VertexPtr expr = GenTree::get_actual_value(root);
    if (should_convert_to_const(expr)) {
      res = create_const_variable(expr, root->location);
    }
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
VertexPtr CollectConstVarsPass::on_enter_vertex(VertexPtr root, LocalT *local) {
  in_param_list_ += root->type() == op_func_param_list;

  local->need_recursion_flag = false;

  if (vk::any_of_equal(root->type(), op_defined, op_require, op_require_once)) {
    return root;
  }

  if (root->const_type == cnst_const_val) {
    if (root->type() == op_conv_regexp) {
      VertexPtr expr = GenTree::get_actual_value(root.as<op_conv_regexp>()->expr());
      if (vk::any_of_equal(expr->type(), op_string, op_concat, op_string_build)) {
        return create_const_variable(root, root->location);
      }
    }
    if (vk::none_of_equal(root->type(), op_string, op_array)) {
      if (should_convert_to_const(root)) {
        return create_const_variable(root, root->location);
      }
    }
    const_array_depth_ += root->type() == op_array;
  }
  local->need_recursion_flag = true;
  return root;
}

bool CollectConstVarsPass::should_convert_to_const(VertexPtr root) {
  return vk::any_of_equal(root->type(), op_string, op_array, op_concat, op_string_build, op_func_call);
}

VertexPtr CollectConstVarsPass::create_const_variable(VertexPtr root, Location loc) {
  std::string name;

  if (root->type() == op_string) {
    name = gen_const_string_name(root.as<op_string>()->str_val);
  } else if (root->type() == op_conv_regexp && root.as<op_conv_regexp>()->expr()->type() == op_string) {
    name = gen_const_regexp_name(root.as<op_conv_regexp>()->expr().as<op_string>()->str_val);
  } else if (is_array_suitable_for_hashing(root)) {
    name = gen_const_array_name(root.as<op_array>());
  } else {
    name = gen_unique_name("const_var");
  }

  auto var = VertexAdaptor<op_var>::create();
  var->str_val = name;
  var->extra_type = op_ex_var_const;
  var->location = loc;
  var->type_rule = root->type_rule;

  VarPtr var_id = G->get_global_var(name, VarData::var_const_t, root);
  if (root->type() != op_array) {
    var_id->dependency_level = 0;
  } else {
    int max_dep_level = 1;
    for (auto it : root.as<op_array>()->args()) {
      max_dep_level = std::max(max_dep_level, get_dependency_level(it) + 1);
    }

    var_id->dependency_level = max_dep_level;
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

  var->set_var_id(var_id);
  return var;
}

bool CollectConstVarsPass::need_recursion(VertexPtr, LocalT *local) {
  return local->need_recursion_flag;
}

bool CollectConstVarsPass::user_recursion(VertexPtr v, LocalT *, VisitVertex<CollectConstVarsPass> &visit) {
  if (v->type() == op_function) {
    if (current_function->type == FunctionData::func_class_holder) {
      ClassPtr c = current_function->class_id;
      c->members.for_each([&](ClassMemberStaticField &field) {
        if (field.init_val) {
          visit(field.init_val);
        }
      });
    }
  }
  return false;
}
