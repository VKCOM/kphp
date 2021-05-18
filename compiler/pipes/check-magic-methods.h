// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class CheckMagicMethods final : public FunctionPassBase {
public:
  string get_description() final {
    return "Check magic methods";
  }

  VertexPtr on_exit_vertex(VertexPtr root) override;

private:
  static VertexPtr process_func(VertexAdaptor<op_function> fun);
};
