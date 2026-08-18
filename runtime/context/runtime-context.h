// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/allocator/global-memory-allocator.h"
#include "runtime-common/core/runtime-core.h"

extern RuntimeContext kphp_runtime_context;
extern GlobalMemoryAllocator global_memory_allocator;
extern RuntimeAllocator runtime_allocator;
