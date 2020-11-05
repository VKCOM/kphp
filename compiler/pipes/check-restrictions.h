// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/inferring/public.h"
#include "compiler/pipes/function-and-cfg.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class CheckRestrictionsF final : public SyncPipeF<FunctionAndCFG> {
  using Base = SyncPipeF<FunctionAndCFG>;
public:
  CheckRestrictionsF() = default;

  void on_finish(DataStream<FunctionAndCFG> &os) final {
    tinf::get_inferer()->check_restrictions();
    tinf::get_inferer()->finish();
    Base::on_finish(os);
  }
};
