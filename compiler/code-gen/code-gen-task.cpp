// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/code-gen-task.h"

ProfilerRaw &get_code_gen_profiler() {
  static CachedProfiler profiler{"Code Generation"};
  return *profiler;
}
