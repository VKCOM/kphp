#pragma once

#include "compiler/data/class-data.h"
#include "compiler/function-pass.h"

/*** Replace constant expressions with const variables ***/
class CollectConstVarsPass : public FunctionPassBase {
private:
  int in_param_list_{0};
  int const_array_depth_{0};

  int get_dependency_level(VertexPtr vertex);
  bool should_convert_to_const(VertexPtr root);
  VertexPtr create_const_variable(VertexPtr root, Location loc);

public:
  string get_description() {
    return "Collect constants";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && !function->is_extern();
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local);

  bool need_recursion(VertexPtr root __attribute__((unused)), LocalT *local);

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local __attribute__((unused)));

  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitVertex<CollectConstVarsPass> &visit);

};
