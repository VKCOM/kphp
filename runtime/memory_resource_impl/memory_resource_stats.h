#pragma once

#include "common/stats/provider.h"
#include "kphp-core/memory_resource/memory_resource.h"

void write_memory_stats_to(const memory_resource::MemoryStats & memoryStats, stats_t *stats, const char *prefix) noexcept;
