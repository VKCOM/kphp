// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-generator.h"
#include "compiler/scheduler/task.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"

ProfilerRaw &get_code_gen_profiler();

template<class T>
class CodeGenTask : public Task {
private:
  CodeGenerator W;
  T cmd;
public:
  CodeGenTask(const CodeGenerator &W, const T &cmd) :
    W(W),
    cmd(cmd) {
  }

  void execute() {
    AutoProfiler profler{get_code_gen_profiler()};
    stage::set_name("Async code generation");
    W << cmd;
  }
};

template<class T>
struct AsyncImpl {
  const T &cmd;
  AsyncImpl(const T &cmd) :
    cmd(cmd) {
  }
  void compile(CodeGenerator &W) const {
    register_async_task(new CodeGenTask<T>(W, cmd));
  }
};

template<class T>
AsyncImpl<T> Async(const T &cmd) {
  return AsyncImpl<T>(cmd);
}
