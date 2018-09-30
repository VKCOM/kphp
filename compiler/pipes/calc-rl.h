#pragma once

#include "compiler/bicycle.h"

class CalcRLF {
public:
  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
