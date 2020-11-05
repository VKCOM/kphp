// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"
#include "compiler/pipes/function-and-cfg.h"

class CloneStrangeConstParams final : public FunctionPassBase {
public:
  using ExecuteType = FunctionAndCFG;
  string get_description() override {
    return "Clone strange const params";
  }
  VertexPtr on_enter_vertex(VertexPtr root) override;
};
