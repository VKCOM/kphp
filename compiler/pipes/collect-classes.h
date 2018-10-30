#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

class CollectClassF {
public:
  void execute(FunctionPtr data, DataStream<FunctionPtr> &os);
};

