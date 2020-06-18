#include <cassert>

#include "runtime/memory_resource/memory_resource.h"

namespace {

void write_stat(stats_t *stats, const char *prefix, const char *suffix, size_t value) noexcept {
  char buffer[256]{0};
  const int len = snprintf(buffer, sizeof(buffer) - 1, "%s.%s", prefix, suffix);
  assert(len > 0 && sizeof(buffer) >= static_cast<size_t>(len + 1));
  add_histogram_stat_long(stats, buffer, static_cast<int64_t>(value));
};

} // namespace

namespace memory_resource {

void MemoryStats::write_stats_to(stats_t *stats, const char *prefix) const noexcept {
  write_stat(stats, prefix, "memory.limit", memory_limit);
  write_stat(stats, prefix, "memory.used", memory_used);
  write_stat(stats, prefix, "memory.used_max", max_memory_used);
  write_stat(stats, prefix, "memory.real_used", real_memory_used);
  write_stat(stats, prefix, "memory.real_used_max", max_real_memory_used);
  write_stat(stats, prefix, "memory.defragmentation_calls", defragmentation_calls);
  write_stat(stats, prefix, "memory.huge_memory_pieces", huge_memory_pieces);
  write_stat(stats, prefix, "memory.small_memory_pieces", small_memory_pieces);
}

} // namespace memory_resource
