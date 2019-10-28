#pragma once

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/pipes/function-and-cfg.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class TypeInfererEndF final : public SyncPipeF<FunctionAndCFG> {
  using Base = SyncPipeF<FunctionAndCFG>;
public:
  TypeInfererEndF() = default;

  void on_finish(DataStream<FunctionAndCFG> &os) final {
    tinf::get_inferer()->check_restrictions();
    tinf::get_inferer()->finish();
    Base::on_finish(os);
  }
};
