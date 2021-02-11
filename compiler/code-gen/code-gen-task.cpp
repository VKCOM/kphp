// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-gen-task.h"

ProfilerRaw &get_code_gen_profiler() {
  static CachedProfiler profiler{"Code Generation"};
  return *profiler;
}

void CodeGenSchedulerTask::execute() {
  AutoProfiler profler{get_code_gen_profiler()};
  stage::set_name("Code generation");
  // uncomment this to launch codegen twice and ensure there is no diff (codegeneration is idempotent)
  // cmd->compile(W);
  cmd->compile(W);
}
