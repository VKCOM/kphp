// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstddef>
#include <new>
#include <random>
#include <sys/mman.h>

#include "server/task-workers/shared-memory-manager.h"

namespace task_workers {
void task_workers::SharedMemoryManager::init() {
  memory = static_cast<unsigned char *>(mmap(nullptr, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
  for (size_t i = 0; i < TOTAL_SLICES_COUNT; ++i) {
    unsigned char *slice_ptr = get_slice_ptr(i);
    new (slice_ptr + SLICE_PAYLOAD_SIZE) SliceMetaInfo{};
  }
}

void *SharedMemoryManager::allocate_slice() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(0, TOTAL_SLICES_COUNT - 1);
  size_t random_slice_idx = dis(gen);
  size_t i = random_slice_idx;
  do {
    unsigned char *cur_slice_ptr = get_slice_ptr(i);
    SliceMetaInfo &cur_slice_meta_info = get_slice_meta_info(cur_slice_ptr);

    if (cur_slice_meta_info.acquire()) {
      return cur_slice_ptr;
    }

    ++i;
    if (i == TOTAL_SLICES_COUNT) {
      i = 0;
    }
  } while (i != random_slice_idx);

  return nullptr;
}

void SharedMemoryManager::deallocate_slice(void *slice) {
  auto *slice_ptr = static_cast<unsigned char *>(slice);
  SliceMetaInfo &slice_meta_info = get_slice_meta_info(slice_ptr);
  slice_meta_info.release();
}

} // namespace task_workers
