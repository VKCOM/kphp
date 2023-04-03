#include "server/data-sharing.h"

#include "common/wrappers/memory-utils.h"
#include "server/web-server-stats.h"

void DataSharing::init() {
  auto *ptr = mmap_shared(sizeof(Storage));
  storage = new (ptr) Storage();
}
