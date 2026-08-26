// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class CalcMagicConstPass final : public FunctionPassBase {
public:
  string get_description() override {
    return "Calculate magic constants ";
  }

  VertexPtr on_enter_vertex(VertexPtr root) override;
};
