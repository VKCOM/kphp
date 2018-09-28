#pragma once

#include "compiler/bicycle.h"
#include "compiler/pipes/function-and-cfg.h"

class CheckReturnsF {
public:
  void on_finish(DataStream<FunctionAndCFG> &) {}

  void execute(FunctionAndCFG function_and_cfg, DataStream<FunctionAndCFG> &);
};
