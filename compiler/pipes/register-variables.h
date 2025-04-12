// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>
#include <string>

#include "compiler/data/var-data.h"
#include "compiler/function-pass.h"

/**
 * 1. Function parametres (with default values)
 * 2. Global variables
 * 3. Static local variables (with default values)
 * 4. Local variables
 * 5. Class static fields
 */
class RegisterVariablesPass final : public FunctionPassBase {
private:
  std::map<std::string, VarPtr> registred_vars;
  std::forward_list<VertexAdaptor<op_phpdoc_var>> phpdoc_vars;
  bool global_function_flag{false};
  int in_param_list{0};

  VarPtr create_global_var(const std::string &name);
  VarPtr create_local_var(const std::string &name, VarData::Type type, bool create_flag);
  VarPtr get_global_var(const std::string &name);
  VarPtr get_local_var(const std::string &name, VarData::Type type = VarData::var_local_t);
  void register_global_var(VertexAdaptor<op_var> var_vertex);
  bool is_const(VertexPtr v);
  bool is_global_var(VertexPtr v);
  void register_function_static_var(VertexAdaptor<op_var> var_vertex, VertexPtr default_value);
  void register_param_var(VertexAdaptor<op_var> var_vertex, VertexPtr default_value);
  void register_var(VertexAdaptor<op_var> var_vertex);
  void visit_global_vertex(VertexAdaptor<op_global> global);
  void visit_static_vertex(VertexAdaptor<op_static> stat);
  void visit_var(VertexAdaptor<op_var> var);
  void visit_phpdoc_var(VertexAdaptor<op_phpdoc_var> phpdoc_var);

  void visit_func_param_list(VertexAdaptor<op_func_param_list> list);


public:

  std::string get_description() override {
    return "Register variables";
  }

  void on_start() override {
    global_function_flag = current_function->is_main_function() || current_function->type == FunctionData::func_switch;
  }

  void on_finish() override;

  VertexPtr on_enter_vertex(VertexPtr root) override;
  VertexPtr on_exit_vertex(VertexPtr root) override;

  bool user_recursion(VertexPtr v) override;

};
