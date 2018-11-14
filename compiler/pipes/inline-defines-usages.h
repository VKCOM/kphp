#pragma once

#include "compiler/compiler-core.h"
#include "compiler/function-pass.h"

class InlineDefinesUsagesPass : public FunctionPassBase {
private:
  AUTO_PROF (inline_defines_usages);
public:
  string get_description() {
    return "Inline defines pass";
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *);
};
