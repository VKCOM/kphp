// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

#include "common/smart_ptrs/singleton.h"

struct KphpRuntimeContext : KphpCoreContext {

  void init(void *mem, size_t script_mem_size, size_t oom_handling_mem_size);
  void free();

  string_buffer static_SB;
  string_buffer static_SB_spare;
};

extern KphpRuntimeContext kphp_runtime_context;
extern RuntimeAllocator runtime_allocator;
