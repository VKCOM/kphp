// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class CheckFunctionCallsPass final : public FunctionPassBase {
private:
  void check_func_call(VertexPtr &call);
  VertexAdaptor<op_func_call> process_named_args(FunctionPtr func, VertexAdaptor<op_func_call> call) const;

public:
  string get_description() override {
    return "Check function calls";
  }

  VertexPtr on_enter_vertex(VertexPtr v) override;
  VertexPtr on_exit_vertex(VertexPtr v) override;
};
