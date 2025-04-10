// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/pipes/function-and-cfg.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class CFGBeginF {
public:
  void execute(FunctionPtr function, DataStream<FunctionAndCFG>& os);
};

// This wrapper is used to get a readable name in the profiler.
class CFGBeginSync final : public SyncPipeF<FunctionAndCFG> {
public:
  void on_finish(DataStream<FunctionAndCFG>& os) final {
    SyncPipeF<FunctionAndCFG>::on_finish(os);
  }
};
