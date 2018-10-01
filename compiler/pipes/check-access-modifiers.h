#pragma once

#include "compiler/function-pass.h"

class CheckAccessModifiers : public FunctionPassBase {
private:
  AUTO_PROF (check_access_modifiers);
  string namespace_name;
  string class_name;
public:
  CheckAccessModifiers() :
    namespace_name(""),
    class_name("") {
  }

  string get_description() {
    return "Check access modifiers";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern;
  }

  bool on_start(FunctionPtr function);

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *);
};
