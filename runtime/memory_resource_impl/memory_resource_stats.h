// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/stats/provider.h"
#include "runtime-common/runtime-core/memory-resource/memory_resource.h"

void write_memory_stats_to(const memory_resource::MemoryStats & memoryStats, stats_t *stats, const char *prefix) noexcept;
