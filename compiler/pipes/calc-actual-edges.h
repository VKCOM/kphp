#pragma once

#include "compiler/pipes/function-and-edges.h"
#include "compiler/threading/data-stream.h"

class CalcActualCallsEdgesF {
public:
  void execute(FunctionPtr function, DataStream<FunctionAndEdges> &os);
};
