// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

class CollectRequiredAndClassesF {
public:
  // don't show it in profiler, as it duplicates CollectRequiredPass, that's immediately called from execute()
  // (this pipe is needed to have several outputs, which is impossible with Pass structure)
  using need_profiler = std::false_type;

  using ReadyFunctionPtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<ReadyFunctionPtr, SrcFilePtr, FunctionPtr, ClassPtr>;

  void execute(FunctionPtr function, OStreamT &os);
};
