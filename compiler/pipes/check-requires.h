#pragma once

#include "compiler/pipes/sort-and-inherit-classes.h"
#include "compiler/threading/data-stream.h"
#include "compiler/pipes/sync.h"

class CheckRequires final: public SyncPipeF<FunctionPtr, FunctionPtr> {
  using Base = SyncPipeF<FunctionPtr, FunctionPtr>;
public:
  void on_finish(DataStream<FunctionPtr> &os) final {
    SortAndInheritClassesF::check_on_finish(os);
    Base::on_finish(os);
  }
};
