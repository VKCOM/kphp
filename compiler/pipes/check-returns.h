#pragma once

#include "compiler/pipes/function-and-cfg.h"
#include "compiler/threading/data-stream.h"

class CheckReturnsF {
public:
  void execute(FunctionAndCFG function_and_cfg, DataStream<FunctionAndCFG> &);
};
