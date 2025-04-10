// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"
#include "compiler/threading/locks.h"

// see detailed comments is a .cpp file about what this pass does

class InstantiateGenericsAndLambdasF {
  using Base = SyncPipeF<FunctionPtr>;

  using InstantiatedGenericPtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<FunctionPtr, InstantiatedGenericPtr>;

public:
  // don't show it in profiler, as it duplicates InstantiateGenericsAndLambdasPass, that's immediately called from execute()
  // (this pipe is needed to have several outputs, which is impossible with Pass structure)
  using need_profiler = std::false_type;

  void execute(FunctionPtr function, OStreamT& os);
};
