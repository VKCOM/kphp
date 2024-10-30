// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class FixChainingAssignmentForArrayAccessPass final : public FunctionPassBase {
public:
  std::string get_description() override {
    return "Fix chaining assignment for ArrayAccess";
  }

  VertexPtr on_exit_vertex(VertexPtr root) final;
};
