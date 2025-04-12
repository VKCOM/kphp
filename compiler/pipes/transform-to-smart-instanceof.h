// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>
#include <stack>
#include <string>

#include "compiler/data/data_ptr.h"
#include "compiler/function-pass.h"

class TransformToSmartInstanceofPass final : public FunctionPassBase {
public:
  std::string get_description() final {
    return "Transform To Smart Instanceof";
  }

  bool check_function(FunctionPtr f) const override {
    return !f->is_extern();
  }

  bool user_recursion(VertexPtr v) override;

  VertexPtr on_enter_vertex(VertexPtr v) override;

private:
  static VertexAdaptor<op_set> generate_tmp_var_with_instance_cast(VertexPtr instance_var, VertexPtr derived_name_vertex);
  static VertexAdaptor<op_instanceof> get_instanceof_from_if_condition(VertexPtr if_cond);
  static VertexAdaptor<op_instanceof> get_instanceof_from_if_not_condition(VertexPtr if_cond);
  void add_tmp_var_with_instance_cast(VertexAdaptor<op_var> instance_var, VertexPtr name_of_derived, VertexPtr &cmd);
  bool try_replace_if_not_instanceof_return(VertexAdaptor<op_if> v_if_not, VertexAdaptor<op_var> instance_var, VertexPtr name_of_derived);

  bool on_if_user_recursion(VertexAdaptor<op_if> v_if);
  bool on_catch_user_recursion(VertexAdaptor<op_catch> v_catch);
  bool on_lambda_user_recursion(VertexAdaptor<op_lambda> v_lambda);

  VertexPtr try_replace_switch_when_constexpr(VertexAdaptor<op_switch> v_switch);

  std::map<std::string, std::stack<std::string>> new_names_of_var;
};
