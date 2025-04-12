// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class ExtractResumableCallsPass final : public FunctionPassBase {
private:
  static VertexPtr *skip_conv_and_sets(VertexPtr *replace) noexcept;
  static bool is_resumable_expr(VertexPtr vertex) noexcept;
  static VertexPtr *extract_resumable_expr(VertexPtr vertex) noexcept;
  static std::pair<VertexAdaptor<op_move>, VertexAdaptor<op_set>> make_temp_resumable_var(VertexPtr init) noexcept;

  static VertexPtr replace_set_ternary(VertexAdaptor<op_set_modify> set_vertex, VertexAdaptor<op_ternary> rhs_ternary) noexcept;
  static VertexPtr replace_set_logical_operation(VertexAdaptor<op_set_modify> set_vertex, VertexAdaptor<meta_op_binary> operation) noexcept;
  static VertexPtr replace_resumable_expr_with_temp_var(VertexPtr *resumable_expr, VertexPtr expr_user) noexcept;

public:
  std::string get_description() override {
    return "Extract easy resumable calls";
  }

  bool check_function(FunctionPtr function) const override;

  VertexPtr on_enter_vertex(VertexPtr vertex) override;

};
