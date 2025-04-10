// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class GenTreePostprocessPass final : public FunctionPassBase {
  struct builtin_fun {
    Operation op;
    int args;
  };

  builtin_fun get_builtin_function(const std::string& name);

public:
  std::string get_description() override {
    return "Gen tree postprocess";
  }
  VertexPtr on_enter_vertex(VertexPtr root) override;
  VertexPtr on_exit_vertex(VertexPtr root) override;

  // converts the spread operator (...$a) to a call to the array_merge_spread function
  static VertexPtr convert_array_with_spread_operators(VertexAdaptor<op_array> array_vertex);
};
