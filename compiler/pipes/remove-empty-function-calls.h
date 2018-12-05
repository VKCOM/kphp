#pragma once

#include "compiler/function-pass.h"

class RemoveEmptyFunctionCalls : public FunctionPassBase {
public:
  string get_description() { return "Filter empty functions"; }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *);
  VertexPtr on_exit_vertex(VertexPtr v, LocalT *);
};
