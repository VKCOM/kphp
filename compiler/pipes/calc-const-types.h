#pragma once

#include "compiler/function-pass.h"

/*** Calculate const_type for all nodes ***/
class CalcConstTypePass final : public FunctionPassBase {
  void calc_const_type_of_class_fields(ClassPtr klass);

public:

  void on_start() override;

  string get_description() override {
    return "Calc const types";
  }

  VertexPtr on_exit_vertex(VertexPtr v) override;
};
