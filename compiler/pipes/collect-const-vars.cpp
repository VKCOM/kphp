#include "compiler/pipes/collect-const-vars.h"

#include "compiler/gentree.h"
#include "compiler/data/src-file.h"

int CollectConstVarsPass::get_dependency_level(VertexPtr vertex) {
  if (vertex->type() == op_var) {
    VertexAdaptor<op_var> var_adaptor = vertex.as<op_var>();
    return var_adaptor->get_var_id()->dependency_level;
  }

  if (vertex->type() == op_double_arrow) {
    int dep_key = get_dependency_level(vertex.as<op_double_arrow>()->key());
    int dep_value = get_dependency_level(vertex.as<op_double_arrow>()->value());

    return std::max(dep_key, dep_value);
  }

  return 0;
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
    if (should_convert_to_const(root)) {
      res = create_const_variable(root, root->location);
    }
  }

  in_param_list -= root->type() == op_func_param_list;

  return res;
}
VertexPtr CollectConstVarsPass::on_enter_vertex(VertexPtr root, LocalT *local) {
  in_param_list += root->type() == op_func_param_list;

  local->need_recursion_flag = false;

  if (root->type() == op_defined || root->type() == op_require || root->type() == op_require_once) {
    return root;
  }

  if (root->const_type == cnst_const_val) {
    if (root->type() == op_conv_regexp) {
      VertexPtr expr = GenTree::get_actual_value(root.as<op_conv_regexp>()->expr());

      bool conv_to_const =
        expr->type() == op_string ||
        expr->type() == op_concat ||
        expr->type() == op_string_build ||
        should_convert_to_const(root);

      if (conv_to_const) {
        return create_const_variable(root, root->location);
      }
    }
    if (root->type() != op_string && root->type() != op_array) {
      if (should_convert_to_const(root)) {
        return create_const_variable(root, root->location);
      }
    }
  }
  local->need_recursion_flag = true;

  return root;
}
bool CollectConstVarsPass::should_convert_to_const(VertexPtr root) {
  return root->type() == op_string || root->type() == op_array ||
         root->type() == op_concat || root->type() == op_string_build ||
         root->type() == op_func_call;
}
VertexPtr CollectConstVarsPass::create_const_variable(VertexPtr root, Location loc) {
  string name;
  bool global_init_flag = true;

  if (root->type() == op_string) {
    name = gen_const_string_name(root.as<op_string>()->str_val);
  } else if (root->type() == op_conv_regexp && root.as<op_conv_regexp>()->expr()->type() == op_string) {
    name = gen_const_regexp_name(root.as<op_conv_regexp>()->expr().as<op_string>()->str_val);
  } else if (is_array_suitable_for_hashing(root)) {
    name = gen_const_array_name(root.as<op_array>());
  } else {
    global_init_flag = false;
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

  var_id->global_init_flag = global_init_flag;

  FunctionPtr where_f = current_function->type == FunctionData::func_class_holder ? current_function->file_id->main_function : current_function;
  if (in_param_list > 0) {
    where_f->header_const_var_ids.insert(var_id);
  } else {
    where_f->const_var_ids.insert(var_id);
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
