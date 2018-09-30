#pragma once

#include "compiler/bicycle.h"

class CheckUBF {
public:
  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
