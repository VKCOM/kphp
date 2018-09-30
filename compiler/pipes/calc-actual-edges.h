#pragma once

#include "compiler/bicycle.h"
#include "compiler/pipes/function-and-edges.h"

class CalcActualCallsEdgesF {
public:
  void execute(FunctionPtr function, DataStream<FunctionAndEdges> &os);
};
