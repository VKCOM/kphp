#pragma once

#include "compiler/function-pass.h"

class PreprocessEq3Pass : public FunctionPassBase {
private:
  VertexPtr convert_eq3_null_to_isset(VertexPtr eq_op, VertexPtr not_null);
public:
  string get_description() {
    return "Preprocess eq3";
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *);
};
