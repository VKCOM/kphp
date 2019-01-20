#pragma once

#include "compiler/function-pass.h"
#include "compiler/pipes/function-and-cfg.h"

class CheckReturnsPass : public FunctionPassBase {
private:
  bool have_void;
  bool have_not_void;
  bool warn_fired;
  bool error_fired;
public:

  using ExecuteType = FunctionAndCFG;

  string get_description() {
    return "Check returns";
  }

  void init();

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local __attribute__((unused)));

  nullptr_t on_finish();
};
