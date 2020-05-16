#pragma once

#include "compiler/data/define-data.h"
#include "compiler/function-pass.h"

class EraseDefinesDeclarationsPass : public FunctionPassBase {
public:
  string get_description() {
    return "Erase defines declarations";
  }

  VertexPtr on_exit_vertex(VertexPtr root);

  inline bool need_recursion(VertexPtr v) {
    return can_define_be_inside_op(v->type());
  }
};
