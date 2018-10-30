#pragma once

#include "compiler/data/data_ptr.h"

class CFGData;

struct FunctionAndCFG {
  FunctionPtr function;
  CFGData *data;

  FunctionAndCFG() :
    function(),
    data(nullptr) {
  }

  FunctionAndCFG(FunctionPtr function, CFGData *data) :
    function(function),
    data(data) {
  }
};
