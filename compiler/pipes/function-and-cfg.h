#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"

struct CFGData {
  std::vector<VertexAdaptor<op_var>> uninited_vars;
  std::vector<VarPtr> todo_var;
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
