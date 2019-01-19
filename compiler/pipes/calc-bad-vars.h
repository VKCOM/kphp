#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/pipes/calc-func-dep.h"
#include "compiler/threading/data-stream.h"

class CalcBadVarsF {
private:
  DataStream<std::pair<FunctionPtr, DepData>> tmp_stream;
public:

  CalcBadVarsF() {
    tmp_stream.set_sink(true);
  }

  void execute(std::pair<FunctionPtr, DepData> data, DataStream<FunctionPtr> &);

  void on_finish(DataStream<FunctionPtr> &os);
};
