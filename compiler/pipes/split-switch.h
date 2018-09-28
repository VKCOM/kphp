#pragma once

#include "compiler/bicycle.h"

class SplitSwitchF {
public:
  void on_finish(DataStream<FunctionPtr> &) {}

  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
