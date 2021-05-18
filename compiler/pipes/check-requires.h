// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/pipes/sort-and-inherit-classes.h"
#include "compiler/threading/data-stream.h"
#include "compiler/pipes/sync.h"

class CheckRequires final: public SyncPipeF<FunctionPtr, FunctionPtr> {
  using Base = SyncPipeF<FunctionPtr, FunctionPtr>;
public:
  bool forward_to_next_pipe(const FunctionPtr &f) final;

  void execute(FunctionPtr function, DataStream<FunctionPtr> &unused_os) final;

  void on_finish(DataStream<FunctionPtr> &os) final {
    stage::die_if_global_errors();
    SortAndInheritClassesF::check_on_finish(os);
    Base::on_finish(os);
  }
};
