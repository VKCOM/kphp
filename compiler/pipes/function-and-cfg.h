// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

struct CFGData {
  std::vector<VertexAdaptor<op_var>> uninited_vars;
  std::vector<std::vector<std::vector<VertexAdaptor<op_var>>>> todo_parts;
};

struct FunctionAndCFG {
  FunctionPtr function;
  CFGData data;

  FunctionAndCFG() = default;
  explicit FunctionAndCFG(FunctionPtr function, CFGData data) :
    function(std::move(function)),
    data(std::move(data)) {
  }
};
