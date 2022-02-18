// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class CalcValRefPass final : public FunctionPassBase {
  bool is_allowed_for_getting_val_or_ref(Operation op, bool is_last, bool is_first);
public:
  std::string get_description() override {
    return "Calc val ref";
  }

  VertexPtr on_enter_vertex(VertexPtr v) override;
};
