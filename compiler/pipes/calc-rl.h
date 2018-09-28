#pragma once

#include "compiler/bicycle.h"

class CalcRLF {
public:
  void on_finish(DataStream<FunctionPtr> &) {}

  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
