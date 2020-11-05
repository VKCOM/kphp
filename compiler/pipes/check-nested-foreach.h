// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class CheckNestedForeachPass final : public FunctionPassBase {
  vector<VarPtr> foreach_vars{};
  vector<VarPtr> foreach_ref_vars{};
  vector<VarPtr> foreach_key_vars{};
  vector<VarPtr> errored_vars{};
public:

  string get_description() override {
    return "Try to detect common errors: nested foreach";
  }

  bool check_function(FunctionPtr function) const override {
    return !function->is_extern();
  }


  VertexPtr on_enter_vertex(VertexPtr vertex) override;


  VertexPtr on_exit_vertex(VertexPtr vertex) override;
};
