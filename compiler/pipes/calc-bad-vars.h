#pragma once

#include "compiler/data_ptr.h"
#include "compiler/threading/data-stream.h"

struct DepData;

class CalcBadVarsF {
private:
  DataStream<pair<FunctionPtr, DepData *>> tmp_stream;
public:
  CalcBadVarsF() {
    tmp_stream.set_sink(true);
  }

  void execute(FunctionPtr function, DataStream<FunctionPtr> &);


  void on_finish(DataStream<FunctionPtr> &os);
};
