#pragma once

#include "compiler/function-pass.h"

class ExtractAsyncPass final : public FunctionPassBase {
public:
  string get_description() override {
    return "Extract async";
  }

  bool check_function(FunctionPtr function) const override;

  VertexPtr on_exit_vertex(VertexPtr vertex) override;

  bool user_recursion(VertexPtr vertex) override {
    return vertex->type() == op_fork;
  }

private:
  void raise_cant_save_result_of_resumable_func(FunctionPtr func);
};
