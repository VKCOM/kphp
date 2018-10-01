#pragma once

#include "compiler/function-pass.h"

class CheckNestedForeachPass : public FunctionPassBase {
  vector<VarPtr> foreach_vars;
  vector<VarPtr> forbidden_vars;
  int in_unset;
public:
  struct LocalT : public FunctionPassBase::LocalT {
    int to_remove;
    VarPtr to_forbid;
  };

  void init();

  string get_description() {
    return "Try to detect common errors: nested foreach";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern;
  }


  VertexPtr on_enter_vertex(VertexPtr vertex, LocalT *local);


  VertexPtr on_exit_vertex(VertexPtr vertex, LocalT *local);
};
