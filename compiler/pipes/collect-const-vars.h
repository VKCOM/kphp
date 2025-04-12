// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/class-data.h"
#include "compiler/function-pass.h"

/*** Replace constant expressions with const variables ***/
class CollectConstVarsPass final : public FunctionPassBase {
private:
  int in_param_list_{0};
  int composite_const_depth_{0};

  VertexPtr create_const_variable(VertexPtr root, Location loc);

public:
  std::string get_description() override {
    return "Collect constants";
  }

  VertexPtr on_enter_vertex(VertexPtr root) override;

  VertexPtr on_exit_vertex(VertexPtr root) override;

  bool user_recursion(VertexPtr v) override;

};
