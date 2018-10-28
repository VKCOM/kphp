#pragma once

#include "compiler/data_ptr.h"
#include "compiler/threading/data-stream.h"

class CalcRLF {
public:
  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
