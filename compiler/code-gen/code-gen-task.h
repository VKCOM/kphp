// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-generator.h"
#include "compiler/scheduler/task.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"

ProfilerRaw &get_code_gen_profiler();

struct CodeGenRootCmd;

class CodeGenSchedulerTask : public Task {
  CodeGenerator W;
  DataStream<std::unique_ptr<CodeGenRootCmd>> &os;
  std::unique_ptr<CodeGenRootCmd> cmd;

public:
  CodeGenSchedulerTask(DataStream<std::unique_ptr<CodeGenRootCmd>> &os, std::unique_ptr<CodeGenRootCmd> &&cmd);

  void execute() final;
};

inline void code_gen_start_root_task(DataStream<std::unique_ptr<CodeGenRootCmd>> &os, std::unique_ptr<CodeGenRootCmd> &&cmd) {
  register_async_task(new CodeGenSchedulerTask(os, std::move(cmd)));
}
