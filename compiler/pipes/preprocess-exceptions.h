// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class PreprocessExceptions final : public FunctionPassBase {
public:
  string get_description() override {
    return "Inject locations into the created exceptions";
  }

  VertexPtr on_exit_vertex(VertexPtr v) override;
};
