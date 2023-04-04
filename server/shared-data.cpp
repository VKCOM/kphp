#include "server/shared-data.h"

#include <new>
#include "common/wrappers/memory-utils.h"

void SharedData::init() {
  auto *ptr = mmap_shared(sizeof(Storage));
  storage = new (ptr) Storage();
}
