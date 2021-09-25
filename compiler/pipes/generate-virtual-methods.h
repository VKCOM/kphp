// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class GenerateVirtualMethodsF final : public SyncPipeF<FunctionPtr> {
public:
  void on_finish(DataStream<FunctionPtr> &os) final;
};
