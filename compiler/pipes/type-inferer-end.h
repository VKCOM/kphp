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

  bool forward_to_next_pipe(const FunctionAndCFG &f_and_cfg) final {
    return f_and_cfg.function->type != FunctionData::func_class_holder || f_and_cfg.function->class_id->really_used;
  }

  void on_finish(DataStream<FunctionAndCFG> &os) final {
    tinf::get_inferer()->check_restrictions();
    tinf::get_inferer()->finish();
    Base::on_finish(os);
  }
};
