#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

class CollectRequiredAndClassesF {
public:
  using ReadyFunctionPtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<ReadyFunctionPtr, SrcFilePtr, FunctionPtr, ClassPtr>;

  void execute(FunctionPtr function, OStreamT &os);
};
