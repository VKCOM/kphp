#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

class LoadFileF {
public:
  void execute(SrcFilePtr file, DataStream<SrcFilePtr> &os);
};
