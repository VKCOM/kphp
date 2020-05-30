#pragma once

#include "compiler/function-pass.h"

class FixReturnsPass final : public FunctionPassBase {
public:
  string get_description() override {
    return "Fix returns";
  }

  VertexPtr on_enter_vertex(VertexPtr root) override;

  bool user_recursion(VertexPtr root) override {
    return root->type() == op_fork;
  }
};
