// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-gen-task.h"

// this os exists to be passed to CodeGenerator constructor,
// but it won't be used actually, as CodeGenerator is not created in "just calc hashes" mode
static DataStream<WriterData *> dummy_os;

ProfilerRaw &get_code_gen_profiler() {
  static CachedProfiler profiler{"Code generation — calc hashes"};
  return *profiler;
}

CodeGenSchedulerTask::CodeGenSchedulerTask(DataStream<std::unique_ptr<CodeGenRootCmd>> &os, std::unique_ptr<CodeGenRootCmd> &&cmd)
  : W(true, dummy_os)
  , os(os)
  , cmd(std::move(cmd)) {}

void CodeGenSchedulerTask::execute() {
  AutoProfiler profler{get_code_gen_profiler()};
  stage::set_name("Code generation");
  
  cmd->compile(W);
  // if a command produced diff since the previous kphp launch, forward it next, it will be re-launched and saved
  if (W.was_diff_in_any_file()) {
    os << std::move(cmd);
  }
}
