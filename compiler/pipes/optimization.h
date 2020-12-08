// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/compiler-core.h"
#include "compiler/function-pass.h"

class OptimizationPass final : public FunctionPassBase {
private:
  VertexPtr optimize_set_push_back(VertexAdaptor<op_set> set_op);
  void collect_concat(VertexPtr root, vector<VertexPtr> *collected);
  VertexPtr optimize_string_building(VertexPtr root);
  VertexPtr optimize_postfix_inc(VertexPtr root);
  VertexPtr optimize_postfix_dec(VertexPtr root);
  VertexPtr optimize_index(VertexAdaptor<op_index> index);
  VertexPtr remove_extra_conversions(VertexPtr root);

public:
  string get_description() override {
    return "Optimization";
  }

  bool check_function(FunctionPtr function) const override;

  VertexPtr on_enter_vertex(VertexPtr root) override;
  VertexPtr on_exit_vertex(VertexPtr root) override;

  bool user_recursion(VertexPtr root) override;

  void on_finish() override;
};
