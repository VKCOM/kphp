#pragma once

#include <map>

#include "compiler/data/data_ptr.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/locks.h"

class GenerateVirtualMethods final : public SyncPipeF<FunctionPtr> {
  using Base = SyncPipeF<FunctionPtr>;

  Lockable mutex;
  std::map<ClassPtr, std::vector<ClassPtr>> lambdas_interfaces;
public:

  void execute(FunctionPtr function, DataStream<FunctionPtr> &unused_os) final;
  void on_finish(DataStream<FunctionPtr> &os) final;
};
