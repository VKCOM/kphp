// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class ConvertStrValToMagicMethodToStringPass final : public FunctionPassBase {
public:
  string get_description() final {
    return "Convert strval to calling the magic toString() method";
  }

  VertexPtr on_exit_vertex(VertexPtr root) override;

private:
  static VertexPtr process_convert(VertexAdaptor<op_conv_string> conv);
  static VertexPtr convert_var_to_call_to_string_method(VertexAdaptor<op_var> var);
};
