#pragma once

#include "compiler/bicycle.h"
#include "compiler/data_ptr.h"
#include "compiler/pipes/function-and-cfg.h"

class CFGBeginF {
public:
  void execute(FunctionPtr function, DataStream<FunctionAndCFG> &os);
};

/*** Control flow graph. End ***/
class CFGEndF {
public:
  void execute(FunctionAndCFG data, DataStream<FunctionPtr> &os);
};
