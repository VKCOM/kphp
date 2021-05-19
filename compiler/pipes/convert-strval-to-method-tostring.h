// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

// This pipe rewrites strval($x) expressions into $x->__toString() expressions.
// Conversion occurs if the variable has a class type and this class has a __toString() method.
class ConvertStrValToMagicMethodToStringPass final : public FunctionPassBase {
public:
  string get_description() final {
    return "Convert strval to calling the magic toString() method";
  }

  VertexPtr on_exit_vertex(VertexPtr root) override;

private:
  static VertexPtr try_convert_var_to_call_to_string_method(VertexAdaptor<op_var> var);
  static VertexPtr process_convert(VertexAdaptor<op_conv_string> conv);
  static void handle_print_r_call(VertexAdaptor<op_func_call> &call);
};
