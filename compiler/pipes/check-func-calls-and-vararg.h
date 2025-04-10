// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/function-pass.h"

class CheckFuncCallsAndVarargPass final : public FunctionPassBase {
  VertexAdaptor<op_func_call> process_varargs(VertexAdaptor<op_func_call> call, FunctionPtr f_called);

  VertexPtr maybe_autofill_missing_call_arg(VertexAdaptor<op_func_call> call, FunctionPtr f_called, VertexAdaptor<op_func_param> param);
  VertexPtr create_CompileTimeLocation_call_arg(const Location& call_location);

public:
  std::string get_description() override {
    return "Check func calls and vararg";
  }

  bool check_function(FunctionPtr function) const final {
    return !function->is_extern();
  }

  VertexPtr on_enter_vertex(VertexPtr root) final;

  VertexPtr on_func_call(VertexAdaptor<op_func_call> call);
  VertexPtr on_fork(VertexAdaptor<op_fork> v_fork);
};
