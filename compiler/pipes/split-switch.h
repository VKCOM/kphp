#pragma once

#include "compiler/bicycle.h"

class SplitSwitchF {
public:
  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
