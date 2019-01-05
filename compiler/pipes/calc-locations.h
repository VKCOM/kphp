#pragma once

#include "compiler/function-pass.h"

class CalcLocationsPass : public FunctionPassBase {
public:
  string get_description() {
    return "Calc locations";
  }

  VertexPtr on_enter_vertex(VertexPtr v, FunctionPassBase::LocalT *);
};
