#pragma once

#include "compiler/function-pass.h"

/*** Replace constant expressions with const variables ***/
class CollectConstVarsPass : public FunctionPassBase {
private:
  AUTO_PROF (collect_const_vars);
  int in_param_list;

  int get_dependency_level(VertexPtr vertex);
  bool should_convert_to_const(VertexPtr root);
  VertexPtr create_const_variable(VertexPtr root, Location loc);

public:
  struct LocalT : public FunctionPassBase::LocalT {
    bool need_recursion_flag;
  };

  CollectConstVarsPass() :
    in_param_list(0) {
  }

  string get_description() {
    return "Collect constants";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->type() != FunctionData::func_extern;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local);

  bool need_recursion(VertexPtr root __attribute__((unused)), LocalT *local);

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local __attribute__((unused)));
};
