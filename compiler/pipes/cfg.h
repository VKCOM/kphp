#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/pipes/function-and-cfg.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class CFGBeginF {
public:
  void execute(FunctionPtr function, DataStream<FunctionAndCFG> &os);
};

// используем эту обертку для того, чтобы было понятное имя в профайлере
class CFGBeginSync final : public SyncPipeF<FunctionAndCFG> {
public:
  void on_finish(DataStream<FunctionAndCFG> &os) final {
    SyncPipeF<FunctionAndCFG>::on_finish(os);
  }
};
