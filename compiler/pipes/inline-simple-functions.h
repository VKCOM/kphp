// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class InlineSimpleFunctions final : public FunctionPassBase {
private:
  bool inline_is_possible_{true};
  int n_simple_operations_{0};
  bool in_param_list_{false};

  void on_simple_operation() noexcept;

public:
  std::string get_description() final { return "Inline simple functions"; }

  VertexPtr on_enter_vertex(VertexPtr root) final;
  VertexPtr on_exit_vertex(VertexPtr root) final;
  bool user_recursion(VertexPtr) final;
  bool check_function(FunctionPtr function) const final;
  void on_start() final;
  void on_finish() final;
};
