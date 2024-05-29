#include "runtime/memory_resource_impl/memory_resource_stats.h"

void write_memory_stats_to(const memory_resource::MemoryStats & memoryStats, stats_t *stats, const char *prefix) noexcept {
  stats->add_gauge_stat(memoryStats.memory_limit, prefix, ".memory.limit");
  stats->add_gauge_stat(memoryStats.memory_used, prefix, ".memory.used");
  stats->add_gauge_stat(memoryStats.real_memory_used, prefix, ".memory.real_used");
  stats->add_gauge_stat(memoryStats.max_memory_used, prefix, ".memory.used_max");
  stats->add_gauge_stat(memoryStats.max_real_memory_used, prefix, ".memory.real_used_max");
  stats->add_gauge_stat(memoryStats.defragmentation_calls, prefix, ".memory.defragmentation_calls");
  stats->add_gauge_stat(memoryStats.huge_memory_pieces, prefix, ".memory.huge_memory_pieces");
  stats->add_gauge_stat(memoryStats.small_memory_pieces, prefix, ".memory.small_memory_pieces");
}