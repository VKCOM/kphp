#pragma once

#include "compiler/function-pass.h"

class PreprocessEq3Pass : public FunctionPassBase {
private:
  AUTO_PROF (preprocess_eq3);
public:
  string get_description() {
    return "Preprocess eq3";
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *);
};
