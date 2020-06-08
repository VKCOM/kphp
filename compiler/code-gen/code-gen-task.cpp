#include "compiler/code-gen/code-gen-task.h"

ProfilerRaw &get_code_gen_profiler() {
  static CachedProfiler profiler{"Code Generation"};
  return *profiler;
}
