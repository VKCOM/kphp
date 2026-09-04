// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

// This pipe forbids suspicious conversions (op_conv*) after the type inference.
class CheckConversionsPass final : public FunctionPassBase {
  static const std::multimap<Operation, PrimitiveType> forbidden_conversions;

public:
  std::string get_description() override {
    return "CheckConversions";
  }

  VertexPtr on_enter_vertex(VertexPtr vertex) override;
};
