#pragma once

#include "compiler/function-pass.h"

class ExtractAsyncPass : public FunctionPassBase {
public:
  string get_description() {
    return "Extract async";
  }

  bool check_function(FunctionPtr function);

  VertexPtr on_exit_vertex(VertexPtr vertex);

  bool need_recursion(VertexPtr vertex) {
    return vertex->type() != op_fork;
  }

private:
  void raise_cant_save_result_of_resumable_func(FunctionPtr func);
};
