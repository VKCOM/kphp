#pragma once

#include "compiler/bicycle.h"
#include "compiler/data_ptr.h"

class CollectRequiredF {
public:
  using ReadyFunctionPtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<ReadyFunctionPtr, SrcFilePtr, FunctionPtr>;

  void execute(FunctionPtr function, OStreamT &os);
};
