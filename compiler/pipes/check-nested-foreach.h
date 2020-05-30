#pragma once

#include "compiler/function-pass.h"

class CheckNestedForeachPass : public FunctionPassBase {
  vector<VarPtr> foreach_vars{};
  vector<VarPtr> foreach_ref_vars{};
  vector<VarPtr> foreach_key_vars{};
  vector<VarPtr> errored_vars{};
public:

  string get_description() {
    return "Try to detect common errors: nested foreach";
  }

  bool check_function(FunctionPtr function) {
    return !function->is_extern();
  }


  VertexPtr on_enter_vertex(VertexPtr vertex);


  VertexPtr on_exit_vertex(VertexPtr vertex);
};
