#pragma once

#include "compiler/data/class-data.h"
#include "compiler/function-pass.h"

/*** Replace constant expressions with const variables ***/
class CollectConstVarsPass final : public FunctionPassBase {
private:
  int in_param_list_{0};
  int const_array_depth_{0};

  int get_dependency_level(VertexPtr vertex);
  bool should_convert_to_const(VertexPtr root);
  VertexPtr create_const_variable(VertexPtr root, Location loc);

public:
  string get_description() override {
    return "Collect constants";
  }

  VertexPtr on_enter_vertex(VertexPtr root) override;

  VertexPtr on_exit_vertex(VertexPtr root) override;

  bool user_recursion(VertexPtr v) override;

};
