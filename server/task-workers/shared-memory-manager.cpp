// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <cstddef>
#include <new>
#include <random>
#include <sys/mman.h>

#include "server/task-workers/shared-memory-manager.h"

namespace task_workers {
void task_workers::SharedMemoryManager::init() {
  slice_size = slice_payload_size + slice_meta_info_size;
  total_slices_count = memory_size / slice_size;

  assert(slice_meta_info_size >= sizeof(SliceMetaInfo));
  assert(slice_payload_size % 1024 == 0);
  assert(memory_size % slice_size == 0);

  memory = static_cast<unsigned char *>(mmap(nullptr, memory_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
  for (size_t i = 0; i < total_slices_count; ++i) {
    unsigned char *slice_ptr = get_slice_ptr(i);
    new (slice_ptr + slice_payload_size) SliceMetaInfo{};
  }
}

void *SharedMemoryManager::allocate_slice() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(0, total_slices_count - 1);
  size_t random_slice_idx = dis(gen);
  size_t i = random_slice_idx;
  do {
    unsigned char *cur_slice_ptr = get_slice_ptr(i);
    SliceMetaInfo &cur_slice_meta_info = get_slice_meta_info(cur_slice_ptr);

    if (cur_slice_meta_info.acquire()) {
      return cur_slice_ptr;
    }

    ++i;
    if (i == total_slices_count) {
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

void SharedMemoryManager::set_memory_size(size_t memory_size_) {
  memory_size = memory_size_;
}

void SharedMemoryManager::set_slice_payload_size(size_t slice_payload_size_) {
  slice_payload_size = slice_payload_size_;
}

size_t SharedMemoryManager::get_slice_payload_size() const {
  return slice_payload_size;
}

size_t SharedMemoryManager::get_slice_meta_info_size() const {
  return slice_meta_info_size;
}

} // namespace task_workers
