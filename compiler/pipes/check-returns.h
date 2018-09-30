#pragma once

#include "compiler/bicycle.h"
#include "compiler/pipes/function-and-cfg.h"

class CheckReturnsF {
public:
  void execute(FunctionAndCFG function_and_cfg, DataStream<FunctionAndCFG> &);
};
