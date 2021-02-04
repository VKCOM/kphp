// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <forward_list>

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

struct CFGData {
  std::forward_list<VertexAdaptor<op_var>> uninited_vars;
  std::forward_list<std::vector<std::vector<VertexAdaptor<op_var>>>> split_parts_list;
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
