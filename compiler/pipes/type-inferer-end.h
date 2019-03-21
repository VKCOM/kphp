#pragma once

#include "compiler/pipes/function-and-cfg.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class TypeInfererEndF final : public SyncPipeF<FunctionAndCFG> {
  using Base = SyncPipeF<FunctionAndCFG>;
public:
  TypeInfererEndF() = default;

  void on_finish(DataStream<FunctionAndCFG> &os) final;
};
