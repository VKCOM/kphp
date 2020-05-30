#pragma once

#include "compiler/function-pass.h"

class CalcValRefPass final : public FunctionPassBase {
  bool is_allowed_for_getting_val_or_ref(Operation op, bool is_last, bool is_first);
public:
  string get_description() override {
    return "Calc val ref";
  }

  VertexPtr on_enter_vertex(VertexPtr v) override;
};
