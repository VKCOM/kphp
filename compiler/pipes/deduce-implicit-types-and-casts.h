// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"
#include "compiler/threading/data-stream.h"

// see detailed comments is a .cpp file about what this pass does

class DeduceImplicitTypesAndCastsPass final : public FunctionPassBase {
  std::forward_list<VertexAdaptor<op_phpdoc_var>> phpdocs_for_vars;
  std::forward_list<VertexAdaptor<op_func_call>> generic_calls;
  std::forward_list<FunctionPtr> nested_lambdas;

  const TypeHint* get_found_phpdoc_for_var(const std::string& var_name) {
    for (const auto& phpdoc_var : phpdocs_for_vars) {
      if (phpdoc_var->var()->str_val == var_name) {
        return phpdoc_var->type_hint;
      }
    }
    return nullptr;
  }

public:
  std::string get_description() override {
    return "Deduce implicit types and casts";
  }

  bool check_function(FunctionPtr f) const override;
  void on_start() override;
  void on_finish() override;
  VertexPtr on_exit_vertex(VertexPtr root) override;

  int print_error_unexisting_function(const std::string& call_string);

  void patch_call_arg_on_func_call(VertexAdaptor<op_func_param> param, VertexPtr& call_arg, VertexAdaptor<op_func_call> call);

  void on_set_to_var(VertexAdaptor<op_var> lhs, VertexPtr& rhs);
  void on_set_to_instance_prop(VertexAdaptor<op_instance_prop> lhs, VertexPtr& rhs);
  void on_set_to_index(VertexPtr lhs, VertexPtr& rhs);
  void on_func_call(VertexAdaptor<op_func_call> call);
  void on_func_param(VertexAdaptor<op_func_param> v_param);
  void on_return(VertexAdaptor<op_return> v_return);
  void on_phpdoc_for_var(VertexAdaptor<op_phpdoc_var> v_phpdoc);
  void on_list(VertexAdaptor<op_list> v_list);
  void on_instance_prop(VertexAdaptor<op_instance_prop> v_prop);
  void on_clone(VertexAdaptor<op_clone> v_clone);
  void on_throw(VertexAdaptor<op_throw> v_throw);
  void on_lambda(VertexAdaptor<op_lambda> v_lambda);
};
