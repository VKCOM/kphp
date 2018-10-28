#pragma once

#include "compiler/pipes/function-and-cfg.h"
#include "compiler/threading/data-stream.h"

class TypeInfererF {
public:
  TypeInfererF();

  void execute(FunctionAndCFG input, DataStream<FunctionAndCFG> &os);

  void on_finish(DataStream<FunctionAndCFG> &os);
};

class TypeInfererEndF {
private:
  DataStream<FunctionAndCFG> tmp_stream;
public:
  TypeInfererEndF() {
    tmp_stream.set_sink(true);
  }

  void execute(FunctionAndCFG input, DataStream<FunctionAndCFG> &) {
    tmp_stream << input;
  }

  void on_finish(DataStream<FunctionAndCFG> &os);
};
