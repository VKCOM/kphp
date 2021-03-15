// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/memory_resource/memory_resource.h"

namespace memory_resource {

void MemoryStats::write_stats_to(stats_t *stats, const char *prefix) const noexcept {
  add_gauge_stat(stats, memory_limit, prefix, ".memory.limit");
  add_gauge_stat(stats, memory_used, prefix, ".memory.used");
  add_gauge_stat(stats, max_memory_used, prefix, ".memory.used_max");
  add_gauge_stat(stats, real_memory_used, prefix, ".memory.real_used");
  add_gauge_stat(stats, max_real_memory_used, prefix, ".memory.real_used_max");
  add_gauge_stat(stats, defragmentation_calls, prefix, ".memory.defragmentation_calls");
  add_gauge_stat(stats, huge_memory_pieces, prefix, ".memory.huge_memory_pieces");
  add_gauge_stat(stats, small_memory_pieces, prefix, ".memory.small_memory_pieces");
}

} // namespace memory_resource
