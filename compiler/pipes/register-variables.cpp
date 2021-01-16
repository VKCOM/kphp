// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/register-variables.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/var-data.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"
#include "compiler/utils/string-utils.h"

VarPtr RegisterVariablesPass::create_global_var(const string &name) {
  VarPtr var = G->get_global_var(name, VarData::var_global_t, VertexPtr());
  auto it = registred_vars.insert(make_pair(name, var));
  if (it.second == false) {
    VarPtr old_var = it.first->second;
    kphp_error(old_var == var, fmt_format("conflict in variable names [{}]", old_var->name));
  } else {
    kphp_assert(in_param_list == 0);
    current_function->global_var_ids.push_back(var);
  }
  return var;
}

VarPtr RegisterVariablesPass::create_local_var(const string &name, VarData::Type type, bool create_flag) {
  auto it = registred_vars.find(name);
  if (it != registred_vars.end()) {
    kphp_error (!create_flag, fmt_format("Redeclaration of local variable: {}", name));
    return it->second;
  }
  VarPtr var = G->create_local_var(current_function, name, type);
  kphp_error (registred_vars.insert(make_pair(name, var)).second, fmt_format("Redeclaration of local variable: {}", name));

  return var;
}

VarPtr RegisterVariablesPass::get_global_var(const string &name) {
  auto it = registred_vars.find(name);
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
  var_vertex->var_id = create_global_var(name);

  FunctionPtr function_where_global_keyword_occurred = var_vertex->location.get_function();
  if (function_where_global_keyword_occurred && function_where_global_keyword_occurred->is_main_function()) {
    var_vertex->var_id->marked_as_global = true;
  }
}

bool RegisterVariablesPass::is_const(VertexPtr v) {
  return v->const_type == cnst_const_val ||
         (v->type() == op_var && v.as<op_var>()->var_id->is_constant());
}

bool RegisterVariablesPass::is_global_var(VertexPtr v) {
  return v->type() == op_var && v.as<op_var>()->var_id->is_in_global_scope();
}

void RegisterVariablesPass::register_function_static_var(VertexAdaptor<op_var> var_vertex, VertexPtr default_value) {
  kphp_error(current_function->type == FunctionData::func_local, "keyword 'static' used in global function");

  string name = var_vertex->str_val;
  VarPtr var = create_local_var(name, VarData::var_static_t, true);
  kphp_assert(var->is_function_static_var());

  if (default_value) {
    if (!kphp_error(is_const(default_value), fmt_format("Default value of [{}] is not constant", name))) {
      var->init_val = default_value;
    }
  }
  var_vertex->var_id = var;
}

void RegisterVariablesPass::register_param_var(VertexAdaptor<op_var> var_vertex, VertexPtr default_value) {
  string name = var_vertex->str_val;
  VarPtr var = create_local_var(name, VarData::var_param_t, true);
  var->is_reference = var_vertex->ref_flag;
  var->marked_as_const = var_vertex->is_const;
  kphp_assert (var);
  if (default_value) {
    kphp_error_return(is_const(default_value) || current_function->is_extern(), fmt_format("Default value of [{}] is not constant", name));
    var->init_val = default_value;
  }
  var_vertex->var_id = var;
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
      ClassPtr used_klass = klass;
      kphp_assert(klass);
      while (klass && !klass->members.has_field(var_name)) {
        klass = klass->parent_class;
      }
      kphp_error_return(klass, fmt_format("static field not found: {}", name));
      auto field = klass->members.get_static_field(var_name);
      kphp_error_return(field, fmt_format("field {} is not static in klass {}", var_name, used_klass->name));
      kphp_error_return(klass == used_klass || !field->modifiers.is_private(),
                        fmt_format("Can't access private static field using derived class: {}", name));
      name = replace_backslashes(klass->name) + "$$" + var_name;
    }
    var = get_global_var(name);
  } else {
    VarData::Type var_type = var_vertex->extra_type == op_ex_var_superlocal_inplace
                             ? VarData::var_local_inplace_t
                             : VarData::var_local_t;
    var = get_local_var(name, var_type);
  }
  var_vertex->var_id = var;
  var->marked_as_global |= var_vertex->extra_type == op_ex_var_superglobal;
}

void RegisterVariablesPass::visit_global_vertex(VertexAdaptor<op_global> global) {
  for (auto var : global->args()) {
    kphp_error_act (
      var->type() == op_var,
      "unexpected expression in 'global'",
      continue
    );
    register_global_var(var.as<op_var>());
  }
}

void RegisterVariablesPass::visit_static_vertex(VertexAdaptor<op_static> stat) {
  for (auto node : stat->args()) {
    VertexPtr default_value;

    auto var = node.try_as<op_var>();
    if (!var) {
      if (auto set_expr = node.try_as<op_set>()) {
        kphp_error_act (
          set_expr->lhs()->type() == op_var,
          "unexpected expression in 'static'",
          continue
        );
        var = set_expr->lhs().as<op_var>();
        default_value = set_expr->rhs();
      } else {
        kphp_error_act (0, "unexpected expression in 'static'", continue);
      }
    }

    register_function_static_var(var, default_value);
  }
}

void RegisterVariablesPass::visit_var(VertexAdaptor<op_var> var) {
  if (var->var_id) {
    // autogenerated via CREATE_VERTEX op_var types that have one VarData per multiple vertices
    kphp_assert (var->var_id->is_constant() || vk::any_of_equal(var->var_id->type(), VarData::var_local_inplace_t, VarData::var_local_t));
    return;
  }
  register_var(var);
}


VertexPtr RegisterVariablesPass::on_enter_vertex(VertexPtr root) {
  kphp_assert (root);
  if (root->type() == op_global) {
    visit_global_vertex(root.as<op_global>());
    return VertexAdaptor<op_empty>::create();
  } else if (root->type() == op_static) {
    visit_static_vertex(root.as<op_static>());
    return VertexAdaptor<op_empty>::create();
  } else if (root->type() == op_var) {
    visit_var(root.as<op_var>());
  } else if (auto prop = root.try_as<op_instance_prop>()) {
    // if it's null, then the unknown field access already errored in the resolve_class_of_arrow_access()
    if (auto klass = resolve_class_of_arrow_access(current_function, root)) {
      if (auto m = klass->get_instance_field(root->get_string())) {
        prop->var_id = m->var;
      } else {
        kphp_error(0, fmt_format("Invalid property access ...->{}: does not exist in class `{}`", root->get_string(), klass->get_name()));
      }
    }
  }

  return root;
}

VertexPtr RegisterVariablesPass::on_exit_vertex(VertexPtr root) {
  if (auto foreach = root.try_as<op_foreach_param>()) {
    if (foreach->x()->ref_flag) {
      foreach->x()->var_id->is_foreach_reference = true;
    }
  }
  return root;
}

void RegisterVariablesPass::visit_func_param_list(VertexAdaptor<op_func_param_list> list) {
  for (auto i : list->params()) {
    auto param = i.as<op_func_param>();
    if (param->has_default_value() && param->default_value()) {
      run_function_pass(param->default_value(), this);
      kphp_error(!param->var()->ref_flag || current_function->is_extern(), "Reference argument can not have a default value");
      register_param_var(param->var(), param->default_value());
    } else {
      register_param_var(param->var(), {});
    }
  }
}

void RegisterVariablesPass::visit_phpdoc_var(VertexAdaptor<op_phpdoc_var> phpdoc_var) {
  const std::string &var_name = phpdoc_var->var()->get_string();

  auto try_bind_to_one_of = [&var_name, &phpdoc_var](const std::vector<VarPtr> &lookup_vars_list) -> bool {
    auto var_it = std::find_if(lookup_vars_list.begin(), lookup_vars_list.end(),
                               [&var_name](VarPtr var) { return var->name == var_name; });
    if (var_it == lookup_vars_list.end()) {
      return false;
    }

    phpdoc_var->var()->var_id = *var_it;
    return true;
  };

  if (try_bind_to_one_of(current_function->local_var_ids) ||
      try_bind_to_one_of(current_function->global_var_ids) ||
      try_bind_to_one_of(current_function->static_var_ids)) {
    return;
  }
  if (try_bind_to_one_of(current_function->param_ids)) {
    kphp_error(0, "Do not use @var for arguments, use @param or type hint");
    return;
  }
  kphp_error(0, fmt_format("Unknown var ${} in phpdoc", var_name));
}

bool RegisterVariablesPass::user_recursion(VertexPtr v) {
  if (v->type() == op_func_param_list) {
    in_param_list++;
    visit_func_param_list(v.as<op_func_param_list>());
    in_param_list--;
    return true;
  } else if (v->type() == op_phpdoc_var) {
    phpdoc_vars.emplace_front(v.as<op_phpdoc_var>()); // we'll analyze them on function finish
    return true;
  }
  return false;
}

void RegisterVariablesPass::on_finish() {
  // analyze @var phpdocs — only after all variables have been detected by code and registered
  // that's because only now we know whether a var inside is local/static/global — to bind correctly to var_id
  for (auto v : phpdoc_vars) {
    stage::set_location(v->location);
    visit_phpdoc_var(v);
  }
}
