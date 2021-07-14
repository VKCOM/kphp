// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

// Converts the spread operator (...$a) to a call to the array_merge_spread function.
class ConvertSpreadOperatorPass final : public FunctionPassBase {
public:
  string get_description() final {
    return "Convert spread operator";
  }

  VertexPtr on_exit_vertex(VertexPtr root) override;
};
