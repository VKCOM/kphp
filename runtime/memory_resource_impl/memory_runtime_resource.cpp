// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "kphp-core/memory_resource/memory_resource.h"

namespace memory_resource {

void MemoryStats::write_stats_to(stats_t *stats, const char *prefix) const noexcept {
  stats->add_gauge_stat(memory_limit, prefix, ".memory.limit");
  stats->add_gauge_stat(memory_used, prefix, ".memory.used");
  stats->add_gauge_stat(real_memory_used, prefix, ".memory.real_used");
  stats->add_gauge_stat(max_memory_used, prefix, ".memory.used_max");
  stats->add_gauge_stat(max_real_memory_used, prefix, ".memory.real_used_max");
  stats->add_gauge_stat(defragmentation_calls, prefix, ".memory.defragmentation_calls");
  stats->add_gauge_stat(huge_memory_pieces, prefix, ".memory.huge_memory_pieces");
  stats->add_gauge_stat(small_memory_pieces, prefix, ".memory.small_memory_pieces");
}

} // namespace memory_resource
