#pragma once

#include "compiler/function-pass.h"

class CheckFunctionCallsPass : public FunctionPassBase {
private:
  AUTO_PROF (check_function_calls);
  void check_func_call(VertexPtr call);
public:
  string get_description() {
    return "Check function calls";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->root->type() == op_function;
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__((unused)));
};
