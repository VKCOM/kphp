#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

class PreprocessFunctionF {
  using InstanceOfFunctionTemplatePtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<FunctionPtr, InstanceOfFunctionTemplatePtr>;
public:
  void execute(FunctionPtr function, OStreamT &os);
};
