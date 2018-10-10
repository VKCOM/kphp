#pragma once

#include "compiler/bicycle.h"
#include "compiler/data_ptr.h"

class PrepareFunctionF {
public:
  void execute(FunctionPtr function, DataStream<FunctionPtr> &os);
};
