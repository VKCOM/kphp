#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

/*** Replace __FUNCTION__ ***/
class PreprocessFunctionF {
  using InstanceOfFunctionTemplatePtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<FunctionPtr, InstanceOfFunctionTemplatePtr>;
public:
  void execute(FunctionPtr function, OStreamT &os);
};
