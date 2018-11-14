#pragma once

#include "compiler/function-pass.h"
#include "compiler/data/define-data.h"

class EraseDefinesDeclarationsPass : public FunctionPassBase {
private:
  AUTO_PROF (erase_defines_declarations);
public:
  string get_description() {
    return "Erase defines declarations";
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *);

  inline bool need_recursion(VertexPtr v, LocalT *) {
    return can_define_be_inside_op(v->type());
  }
};
