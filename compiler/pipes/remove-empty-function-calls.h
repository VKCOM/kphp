#pragma once

#include "compiler/function-pass.h"

class RemoveEmptyFunctionCalls final : public FunctionPassBase {
public:
  string get_description() override { return "Filter empty functions"; }

  VertexPtr on_enter_vertex(VertexPtr v) override;
  VertexPtr on_exit_vertex(VertexPtr v) override;
};
