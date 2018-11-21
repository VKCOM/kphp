#include "compiler/pipes/register-variables.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/var-data.h"
#include "compiler/gentree.h"

VarPtr RegisterVariablesPass::create_global_var(const string &name) {
  VarPtr var = G->get_global_var(name, VarData::var_global_t, VertexPtr());
  pair<map<string, VarPtr>::iterator, bool> it = registred_vars.insert(make_pair(name, var));
  if (it.second == false) {
    VarPtr old_var = it.first->second;
    kphp_error (
      old_var == var,
      format("conflict in variable names [%s]", old_var->name.c_str())
    );
  } else {
    if (in_param_list > 0) {
      current_function->header_global_var_ids.push_back(var);
    } else {
      current_function->global_var_ids.push_back(var);
    }
  }
  return var;
}
VarPtr RegisterVariablesPass::create_local_var(const string &name, VarData::Type type, bool create_flag) {
  map<string, VarPtr>::iterator it = registred_vars.find(name);
  if (it != registred_vars.end()) {
    kphp_error (!create_flag, "Redeclaration of local variable");
    return it->second;
  }
  VarPtr var = G->create_local_var(current_function, name, type);
  kphp_error (registred_vars.insert(make_pair(name, var)).second == true, "Redeclaration of local variable");

  return var;
}
VarPtr RegisterVariablesPass::get_global_var(const string &name) {
  map<string, VarPtr>::iterator it = registred_vars.find(name);
  if (it != registred_vars.end()) {
    return it->second;
  }
  return create_global_var(name);
}
VarPtr RegisterVariablesPass::get_local_var(const string &name, VarData::Type type) {
  return create_local_var(name, type, false);
}
void RegisterVariablesPass::register_global_var(VertexAdaptor<op_var> var_vertex) {
  string name = var_vertex->str_val;
  var_vertex->set_var_id(create_global_var(name));

  FunctionPtr function_where_global_keyword_occured = var_vertex->location.get_function();
  if (function_where_global_keyword_occured && function_where_global_keyword_occured->type() == FunctionData::func_global) {
    var_vertex->get_var_id()->marked_as_global = true;
  }
}
bool RegisterVariablesPass::is_const(VertexPtr v) {
  return v->const_type == cnst_const_val ||
         (v->type() == op_var && v->get_var_id()->is_constant()) ||
         v->type() == op_define_val;
}
bool RegisterVariablesPass::is_global_var(VertexPtr v) {
  return v->type() == op_var && v->get_var_id()->is_in_global_scope();
}
void RegisterVariablesPass::register_static_var(VertexAdaptor<op_var> var_vertex, VertexPtr default_value, OperationExtra extra_type) {
  kphp_error_return (!global_function_flag || extra_type == op_ex_static_private || extra_type == op_ex_static_public
                     || extra_type == op_ex_static_protected,
                     "Keyword 'static' used in global function");

  VarPtr var;
  string name;
  if (global_function_flag) {
    kphp_assert(!current_function->is_lambda());
    name = replace_backslashes(current_function->class_id->name) + "$$" + var_vertex->str_val;
    var = get_global_var(name);
    var->class_id = current_function->class_id;
    kphp_assert(var->is_class_static_var());
  } else {
    kphp_assert(extra_type == op_ex_none);
    name = var_vertex->str_val;
    var = create_local_var(name, VarData::var_static_t, true);
    kphp_assert(var->is_function_static_var());
  }
  if (default_value) {
    if (!kphp_error(is_const(default_value), format("Default value of [%s] is not constant", name.c_str()))) {
      var->init_val = default_value;
    }
  }
  var_vertex->set_var_id(var);
}
void RegisterVariablesPass::register_param_var(VertexAdaptor<op_var> var_vertex, VertexPtr default_value) {
  string name = var_vertex->str_val;
  VarPtr var = create_local_var(name, VarData::var_param_t, true);
  var->is_reference = var_vertex->ref_flag;
  kphp_assert (var);
  if (default_value) {
    if (!kphp_error (
      is_const(default_value) || is_global_var(default_value),
      format("Default value of [%s] is not constant", name.c_str()))) {
      var->init_val = default_value;
    }
  }
  var_vertex->set_var_id(var);
}
void RegisterVariablesPass::register_var(VertexAdaptor<op_var> var_vertex) {
  VarPtr var;
  string name = var_vertex->str_val;
  size_t pos$$ = name.find("$$");
  if (pos$$ != string::npos ||
      (vk::none_of_equal(var_vertex->extra_type, op_ex_var_superlocal, op_ex_var_superlocal_inplace) &&
       global_function_flag) ||
      var_vertex->extra_type == op_ex_var_superglobal) {
    if (pos$$ != string::npos) {
      string class_name = name.substr(0, pos$$);
      string var_name = name.substr(pos$$ + 2);
      ClassPtr klass = G->get_class(replace_characters(class_name, '$', '\\'));
      kphp_assert(klass);
      while (klass && !klass->members.has_field(var_name)) {
        klass = klass->parent_class;
      }
      if (kphp_error(klass, format("static field not found: %s", name.c_str()))) {
        return;
      }
      name = replace_backslashes(klass->name) + "$$" + var_name;
    }
    var = get_global_var(name);
  } else {
    VarData::Type var_type = var_vertex->extra_type == op_ex_var_superlocal_inplace
                             ? VarData::var_local_inplace_t
                             : VarData::var_local_t;
    var = get_local_var(name, var_type);
  }
  if (var_vertex->needs_const_iterator_flag) {
    var->needs_const_iterator_flag = true;
  }
  var_vertex->set_var_id(var);
  var->marked_as_global |= var_vertex->extra_type == op_ex_var_superglobal;
}
void RegisterVariablesPass::visit_global_vertex(VertexAdaptor<op_global> global) {
  for (auto var : global->args()) {
    kphp_error_act (
      var->type() == op_var,
      "unexpected expression in 'global'",
      continue
    );
    register_global_var(var);
  }
}
void RegisterVariablesPass::visit_static_vertex(VertexAdaptor<op_static> stat) {
  for (auto node : stat->args()) {
    VertexAdaptor<op_var> var;
    VertexPtr default_value;

    if (node->type() == op_var) {
      var = node;
    } else if (node->type() == op_set) {
      VertexAdaptor<op_set> set_expr = node;
      var = set_expr->lhs();
      kphp_error_act (
        var->type() == op_var,
        "unexpected expression in 'static'",
        continue
      );
      default_value = set_expr->rhs();
    } else {
      kphp_error_act (0, "unexpected expression in 'static'", continue);
    }

    register_static_var(var, default_value, stat->extra_type);
  }
}
void RegisterVariablesPass::visit_var(VertexAdaptor<op_var> var) {
  if (var->get_var_id()) {
    // автогенерённые через CREATE_VERTEX op_var типы, когда один VarData на несколько разных vertex'ов
    kphp_assert (var->get_var_id()->is_constant() ||
                 var->get_var_id()->type() == VarData::var_local_inplace_t);
    return;
  }
  register_var(var);
}
VertexPtr RegisterVariablesPass::on_enter_vertex(VertexPtr root, RegisterVariablesPass::LocalT *local) {
  kphp_assert (root);
  if (root->type() == op_global) {
    visit_global_vertex(root);
    local->need_recursion_flag = false;
    auto empty = VertexAdaptor<op_empty>::create();
    return empty;
  } else if (root->type() == op_static) {
    kphp_error(!stage::get_function()->root->inline_flag, "Inline functions don't support static variables");
    visit_static_vertex(root);
    local->need_recursion_flag = false;
    auto empty = VertexAdaptor<op_empty>::create();
    return empty;
  } else if (root->type() == op_var) {
    visit_var(root);
    local->need_recursion_flag = false;
  }
  return root;
}
