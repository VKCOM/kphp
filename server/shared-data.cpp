#include "server/shared-data.h"

#include <new>
#include "common/wrappers/memory-utils.h"

uint64_t WorkersStats::pack() const noexcept {
  uint64_t stats = 0;
  stats += running_workers;
  stats = stats << 16;
  stats += waiting_workers;
  stats = stats << 16;
  stats += ready_for_accept_workers;
  stats = stats << 16;
  stats += total_workers;
  return stats;
}

void WorkersStats::unpack(uint64_t stats) noexcept {
  running_workers = (stats) >> 16 * 3;
  waiting_workers = (stats << 16) >> 16 * 3;
  ready_for_accept_workers = (stats << 16 * 2) >> 16 * 3;
  total_workers = (stats << 16 * 3) >> 16 * 3;
}

void SharedData::init() {
  auto *ptr = mmap_shared(sizeof(Storage));
  storage = new (ptr) Storage();
}
