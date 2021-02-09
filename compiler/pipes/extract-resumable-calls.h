// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class ExtractResumableCallsPass final : public FunctionPassBase {
private:
  static VertexPtr *skip_conv_and_sets(VertexPtr *replace) noexcept;
  static VertexPtr try_save_resumable_func_call_in_temp_var(VertexPtr vertex) noexcept;
  static VertexAdaptor<op_var> make_temp_resumable_var(const TypeData *type) noexcept;

public:
  string get_description() override {
    return "Extract easy resumable calls";
  }

  bool check_function(FunctionPtr function) const override;

  VertexPtr on_enter_vertex(VertexPtr vertex) override;

};
