#pragma once

#include "compiler/function-pass.h"

class CheckAccessModifiersPass : public FunctionPassBase {
private:
  string namespace_name;
  string class_name;
  ClassPtr class_id;
public:
  string get_description() {
    return "Check access modifiers";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern;
  }

  bool on_start(FunctionPtr function);

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *);
};
