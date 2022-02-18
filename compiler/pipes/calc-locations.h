// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

#include "compiler/function-pass.h"

class CalcLocationsPass final : public FunctionPassBase {
public:
  std::string get_description() override {
    return "Calc locations";
  }

  void on_start() override;

  VertexPtr on_enter_vertex(VertexPtr v) override;
};
