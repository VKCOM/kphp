#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

class CalcEmptyFunctions {
public:
  void execute(FunctionPtr f, DataStream<FunctionPtr> &os);
};
