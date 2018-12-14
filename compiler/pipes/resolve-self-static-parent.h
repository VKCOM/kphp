#pragma once

#include "compiler/function-pass.h"

class ResolveSelfStaticParentPass : public FunctionPassBase {
private:
  AUTO_PROF (resolve_self_static_parent);
public:
  string get_description() {
    return "Resolve self/static/parent";
  }

  VertexPtr on_enter_vertex(VertexPtr v, FunctionPassBase::LocalT *);
};
