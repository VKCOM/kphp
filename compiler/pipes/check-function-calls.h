#pragma once

#include "compiler/function-pass.h"

class CheckFunctionCallsPass final : public FunctionPassBase {
private:
  void check_func_call(VertexPtr call);
public:
  string get_description() override {
    return "Check function calls";
  }

  VertexPtr on_enter_vertex(VertexPtr v) override;
  VertexPtr on_exit_vertex(VertexPtr v) override;
};
