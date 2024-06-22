// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class ExtractAsyncPass final : public FunctionPassBase {
public:
  std::string get_description() override {
    return "Extract async";
  }

  bool check_function(FunctionPtr function) const override;

  VertexPtr on_exit_vertex(VertexPtr vertex) override;

  bool user_recursion(VertexPtr vertex) override {
    return vertex->type() == op_fork || vertex->type() == op_force_sync;
  }

private:
  void raise_cant_save_result_of_resumable_func(FunctionPtr func);
};
