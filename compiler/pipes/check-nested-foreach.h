// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class CheckNestedForeachPass final : public FunctionPassBase {
public:
  std::string get_description() final {
    return "Try to detect common errors: nested foreach";
  }

  bool check_function(FunctionPtr function) const final {
    return !function->is_extern();
  }

  VertexPtr on_enter_vertex(VertexPtr vertex) final;
  VertexPtr on_exit_vertex(VertexPtr vertex) final;

private:
  std::vector<VarPtr> foreach_iterable_non_local_vars_;
  std::vector<VarPtr> foreach_vars_;
  std::vector<VarPtr> foreach_ref_vars_;
  std::vector<VarPtr> foreach_key_vars_;
  std::vector<VarPtr> errored_vars_;
};
