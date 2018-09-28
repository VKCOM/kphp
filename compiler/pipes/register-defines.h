#pragma once

#include "compiler/compiler-core.h"
#include "compiler/function-pass.h"

class RegisterDefinesPass : public FunctionPassBase {
private:
  AUTO_PROF (register_defines);
public:
  string get_description() {
    return "Register defines pass";
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *);
};
