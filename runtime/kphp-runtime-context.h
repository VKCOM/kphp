#pragma once

#include "kphp-core/kphp-core-context.h"
#include "kphp-core/kphp_core.h"

#include "common/smart_ptrs/singleton.h"

struct KphpRuntimeContext : KphpCoreContext {

  void init(void *mem, size_t script_mem_size, size_t oom_handling_mem_size);
  void free();

  string_buffer static_SB;
  string_buffer static_SB_spare;
};

extern KphpRuntimeContext kphpRuntimeContext;
