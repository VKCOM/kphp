#include "compiler/code-gen/code-gen-task.h"

ProfilerRaw &get_code_gen_profiler() {
  static CachedProfiler profiler{"Async Code Generation"};
  return *profiler;
}
