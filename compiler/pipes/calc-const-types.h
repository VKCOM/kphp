#pragma once

#include "compiler/function-pass.h"

/*** Calculate const_type for all nodes ***/
class CalcConstTypePass : public FunctionPassBase {
  void calc_const_type_of_class_fields(ClassPtr klass);

public:

  bool on_start(FunctionPtr function);

  string get_description() {
    return "Calc const types";
  }

  VertexPtr on_exit_vertex(VertexPtr v);
};
