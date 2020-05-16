#pragma once

#include "compiler/function-pass.h"

class CalcValRefPass : public FunctionPassBase {
  bool is_allowed_for_getting_val_or_ref(Operation op, bool is_last, bool is_first);
public:
  string get_description() {
    return "Calc val ref";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && !function->is_extern();
  }


  VertexPtr on_enter_vertex(VertexPtr v);
};
