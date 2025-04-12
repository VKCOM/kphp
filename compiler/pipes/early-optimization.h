// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

// EarlyOptimizationF implements an optimizations pass that happens before the type inference.
// It only uses the syntax to replace one operation with another, preferring the one
// that would execute faster or allocate less heap memory.
class EarlyOptimizationF {
public:
  void execute(FunctionPtr f, DataStream<FunctionPtr> &os);
};
