#pragma once

#include "compiler/function-pass.h"
#include "compiler/data/define-data.h"

class RegisterDefinesPass : public FunctionPassBase {
  AUTO_PROF(register_defines);

public:
  string get_description() {
    return "Register defines";
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *);

  inline bool need_recursion(VertexPtr v, LocalT *) {
    return can_define_be_inside_op(v->type());
  }
};
