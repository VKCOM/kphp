#pragma once

#include "compiler/function-pass.h"

class GenTreePostprocessPass final : public FunctionPassBase {
  struct builtin_fun {
    Operation op;
    int args;
  };

  static bool is_superglobal(const string &s);
  builtin_fun get_builtin_function(const std::string &name);

public:
  string get_description() override {
    return "Gen tree postprocess";
  }
  VertexPtr on_enter_vertex(VertexPtr root) override;
  VertexPtr on_exit_vertex(VertexPtr root) override;
};
