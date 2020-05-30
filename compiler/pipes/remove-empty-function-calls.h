#pragma once

#include "compiler/function-pass.h"

class RemoveEmptyFunctionCalls final : public FunctionPassBase {
public:
  string get_description() override { return "Filter empty functions"; }

  VertexPtr on_exit_vertex(VertexPtr v) override;
};
