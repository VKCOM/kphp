#pragma once

#include "compiler/data_ptr.h"
#include "compiler/pipes/function-and-cfg.h"
#include "compiler/threading/data-stream.h"

class CFGBeginF {
public:
  void execute(FunctionPtr function, DataStream<FunctionAndCFG> &os);
};

/*** Control flow graph. End ***/
class CFGEndF {
public:
  void execute(FunctionAndCFG data, DataStream<FunctionPtr> &os);
};
