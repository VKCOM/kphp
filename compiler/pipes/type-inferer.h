// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/pipes/function-and-cfg.h"
#include "compiler/threading/data-stream.h"

class TypeInfererF {
public:
  // don't show on_finish() in profiler, as it does nothing and is just a zero line
  using need_on_finish_profiler = std::false_type;

  void execute(FunctionAndCFG input, DataStream<FunctionAndCFG>& os);

  void on_finish(DataStream<FunctionAndCFG>& os);
};
