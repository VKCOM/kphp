// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/shared-data.h"

#include "common/wrappers/memory-utils.h"
#include <new>

WorkersStats::PackerRepr WorkersStats::pack() const noexcept {
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

void WorkersStats::unpack(PackerRepr stats) noexcept {
  total_workers = static_cast<uint16_t>(stats);
  stats >>= 16;
  ready_for_accept_workers = static_cast<uint16_t>(stats);
  stats >>= 16;
  waiting_workers = static_cast<uint16_t>(stats);
  stats >>= 16;
  running_workers = static_cast<uint16_t>(stats);
}

void SharedData::init() {
  auto *ptr = mmap_shared(sizeof(Storage));
  storage = new (ptr) Storage();
}
