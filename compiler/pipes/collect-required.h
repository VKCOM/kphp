#pragma once

#include "compiler/data_ptr.h"
#include "compiler/threading/data-stream.h"

class CollectRequiredF {
public:
  using ReadyFunctionPtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<ReadyFunctionPtr, SrcFilePtr, FunctionPtr>;

  void execute(FunctionPtr function, OStreamT &os);
};
