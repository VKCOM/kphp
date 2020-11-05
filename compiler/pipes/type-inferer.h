// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/pipes/function-and-cfg.h"
#include "compiler/threading/data-stream.h"

class TypeInfererF {
public:
  void execute(FunctionAndCFG input, DataStream<FunctionAndCFG> &os);

  void on_finish(DataStream<FunctionAndCFG> &os);
};

