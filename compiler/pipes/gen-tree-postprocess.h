#pragma once

#include "compiler/function-pass.h"

class GenTreePostprocessPass : public FunctionPassBase {
  struct builtin_fun {
    Operation op;
    int args;
  };

  static bool is_superglobal(const string &s);
  builtin_fun get_builtin_function(const std::string &name);

public:
  VertexPtr on_enter_vertex(VertexPtr root, LocalT *);
  VertexPtr on_exit_vertex(VertexPtr root, LocalT *);
};
