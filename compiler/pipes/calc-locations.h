#pragma once

#include "compiler/function-pass.h"

class CalcLocationsPass : public FunctionPassBase {
public:
  string get_description() {
    return "Calc locations";
  }

  bool on_start(FunctionPtr function);

  VertexPtr on_enter_vertex(VertexPtr v);
};
