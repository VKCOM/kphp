#pragma once

#include "compiler/function-pass.h"

class CollectDefinesPass : public FunctionPassBase {
private:
  AUTO_PROF (collect_defines);
public:
  string get_description() {
    return "Collect defines";
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *);
};
