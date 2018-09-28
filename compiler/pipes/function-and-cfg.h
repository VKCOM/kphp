#pragma once

#include "compiler/data_ptr.h"

class CFGCallback;

struct FunctionAndCFG {
  FunctionPtr function;
  CFGCallback *callback;

  FunctionAndCFG() :
    function(),
    callback(nullptr) {
  }

  FunctionAndCFG(FunctionPtr function, CFGCallback *callback) :
    function(function),
    callback(callback) {
  }
};
