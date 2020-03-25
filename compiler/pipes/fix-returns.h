#pragma once

#include "compiler/function-pass.h"

class FixReturnsPass : public FunctionPassBase {
public:
  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local);

  bool need_recursion(VertexPtr root, LocalT *) {
    return root->type() != op_fork;
  }
};
