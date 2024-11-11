// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class FixArrayAccessPass final : public FunctionPassBase {
public:
  std::string get_description() override {
    return "Fix functions related to ArrayAccess";
  }

  // `on_enter_vertex()` cannot be used because
  // `empty($obj[42])` node becomes a subnode itself in a transformation result
  VertexPtr on_exit_vertex(VertexPtr root) final;
};
