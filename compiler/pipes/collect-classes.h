#pragma once

#include "compiler/bicycle.h"

class CollectClassF {
public:
  void on_finish(DataStream<FunctionPtr> &) {};

  void execute(FunctionPtr data, DataStream<FunctionPtr> &os);
};

