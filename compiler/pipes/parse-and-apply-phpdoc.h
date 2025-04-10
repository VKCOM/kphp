// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/data/function-data.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class ParseAndApplyPhpdocF final : public SyncPipeF<FunctionPtr> {
  using need_profiler = std::true_type;

  using Base = SyncPipeF<FunctionPtr>;

public:
  bool forward_to_next_pipe(const FunctionPtr& f) override {
    return !f->is_lambda() && !f->is_generic();
  }

  void execute(FunctionPtr function, DataStream<FunctionPtr>& unused_os) final;
  void on_finish(DataStream<FunctionPtr>& os) final;
};
