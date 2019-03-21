#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/pipes/calc-func-dep.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class CalcBadVarsF final : public SyncPipeF<std::pair<FunctionPtr, DepData>, FunctionPtr> {
public:
  void on_finish(DataStream<FunctionPtr> &os) final;
};
