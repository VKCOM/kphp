#pragma once

#include "compiler/function-pass.h"

class PreprocessBreakPass : public FunctionPassBase {
private:
  vector<VertexAdaptor<meta_op_cycle>> cycles;

  int current_label_id;

  int get_label_id(VertexAdaptor<meta_op_cycle> cycle, Operation op);

public:

  PreprocessBreakPass() :
    current_label_id(0) {
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local);

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local);
};
