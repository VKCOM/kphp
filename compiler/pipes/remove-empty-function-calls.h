// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class RemoveEmptyFunctionCallsPass final : public FunctionPassBase {
public:
  std::string get_description() override { return "Filter empty functions"; }

  VertexPtr on_enter_vertex(VertexPtr v) override;
  VertexPtr on_exit_vertex(VertexPtr v) override;
};
