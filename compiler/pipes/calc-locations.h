#pragma once

#include "compiler/function-pass.h"

class CalcLocationsPass final : public FunctionPassBase {
public:
  string get_description() override {
    return "Calc locations";
  }

  bool on_start(FunctionPtr function) override;

  VertexPtr on_enter_vertex(VertexPtr v) override;
};
