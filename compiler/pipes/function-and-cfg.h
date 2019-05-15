#pragma once

#include "compiler/data/data_ptr.h"

class CFGData;

struct FunctionAndCFG {
  FunctionPtr function;
  CFGData *data;

  explicit FunctionAndCFG(FunctionPtr function = {}, CFGData *data = nullptr) :
    function(function),
    data(data) {
  }
};
