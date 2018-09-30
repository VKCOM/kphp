#pragma once

#include "compiler/bicycle.h"
#include "compiler/data_ptr.h"

class LoadFileF {
public:
  void execute(SrcFilePtr file, DataStream<SrcFilePtr> &os);
};
