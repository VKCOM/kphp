#include "runtime/memory_resource/synchronized_pool_resource.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <sys/mman.h>

#include "common/parallel/lock_accessible.h"
#include "common/wrappers/align.h"
#include "common/wrappers/likely.h"

#include "runtime/critical_section.h"
#include "runtime/inter_process_mutex.h"
#include "runtime/memory_resource/details/memory_chunk_list.h"
#include "runtime/memory_resource/details/memory_chunk_tree.h"
#include "runtime/memory_resource/details/universal_reallocate.h"
#include "runtime/memory_resource/monotonic_buffer_resource.h"
#include "runtime/php_assert.h"

namespace memory_resource {

class synchronized_pool_resource_impl : monotonic_buffer {
public:
  synchronized_pool_resource_impl(char *buffer, size_type buffer_size) {
    init(buffer, buffer_size);
  }

  void *allocate(size_type size, size_type reserved_before) {
    void *mem = nullptr;
    const auto aligned_size = details::align_for_chunk(size);

    if (aligned_size < MAX_CHUNK_BLOCK_SIZE_) {
      const auto chunk_id = details::get_chunk_id(aligned_size);
      mem = free_chunk_list_[chunk_id].get_mem();
    } else if (details::memory_chunk_node *mem_chunk = free_chunk_tree_->extract(aligned_size)) {
      const size_type chunk_size = details::memory_chunk_tree::get_chunk_size(mem_chunk);
      php_assert(chunk_size >= aligned_size);
      mem = mem_chunk;
      if (const size_type left_piece = chunk_size - aligned_size) {
        put_memory_back(mem, left_piece);
        mem = static_cast<char *>(mem) + left_piece;
      }
    }
    if (!mem) {
      mem = get_from_pool(aligned_size, reserved_before);
    } else {
      rollback_reserve(reserved_before);
    }
    std::lock_guard<inter_process_mutex> lock{memory_mutex_};
    register_allocation(mem, aligned_size);
    return mem;
  }

  void deallocate(void *mem, size_type size) {
    const auto aligned_size = details::align_for_chunk(size);
    put_memory_back(mem, aligned_size);
    std::lock_guard<inter_process_mutex> lock{memory_mutex_};
    register_deallocation(aligned_size);
  }

  size_type reserve(size_type size) {
    const size_type aligned_size = details::align_for_chunk(size);
    std::lock_guard<inter_process_mutex> lock{memory_mutex_};
    if (memory_end_ - memory_current_ < reserved_ + aligned_size) {
      return 0;
    }
    reserved_ += aligned_size;
    return aligned_size;
  }

  void rollback_reserve(size_type reserved_before) {
    std::lock_guard<inter_process_mutex> lock{memory_mutex_};
    php_assert(reserved_ >= reserved_before);
    reserved_ -= reserved_before;
  }

  MemoryStats get_memory_stats() {
    std::lock_guard<inter_process_mutex> lock{memory_mutex_};
    stats_.reserved = reserved_;
    return stats_;
  }

private:
  void *get_from_pool(size_type size, size_type reserved_before) {
    std::lock_guard<inter_process_mutex> lock{memory_mutex_};
    php_assert(reserved_ >= reserved_before);
    reserved_ -= reserved_before;
    if (unlikely(memory_end_ - memory_current_ < reserved_ + size)) {
      php_critical_error("not enough memory to continue");
      return nullptr;
    }
    void *mem = memory_current_;
    memory_current_ += size;
    return mem;
  }

  void put_memory_back(void *mem, size_type size) {
    {
      std::lock_guard<inter_process_mutex> lock{memory_mutex_};
      if (unlikely(!check_memory_piece(mem, size))) {
        critical_dump(mem, size);
        return;
      }

      if (static_cast<char *>(mem) + size == memory_current_) {
        memory_current_ = static_cast<char *>(mem);
        return;
      }
    }

    if (size < MAX_CHUNK_BLOCK_SIZE_) {
      size_type chunk_id = details::get_chunk_id(size);
      free_chunk_list_[chunk_id].put_mem(mem);
    } else {
      free_chunk_tree_->insert(mem, size);
    }
  }

  inter_process_mutex memory_mutex_;
  size_type reserved_{0};

  static constexpr size_type MAX_CHUNK_BLOCK_SIZE_{16u * 1024u};
  std::array<details::synchronized_memory_chunk_list, details::get_chunk_id(MAX_CHUNK_BLOCK_SIZE_)> free_chunk_list_;
  vk::lock_accessible<details::memory_chunk_tree, inter_process_mutex> free_chunk_tree_;
};

void synchronized_pool_resource::init(size_type pool_size) {
  php_assert(!impl_);
  php_assert(!shared_memory_);
  php_assert(!pool_size_);

  pool_size_ = pool_size;
  shared_memory_ = mmap(nullptr, pool_size_, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  inplace_init();
}

void synchronized_pool_resource::inplace_init() {
  size_t left_space = pool_size_;
  void *impl_place = shared_memory_;
  if (!vk::align(alignof(synchronized_pool_resource_impl), sizeof(synchronized_pool_resource_impl), impl_place, left_space)) {
    php_critical_error("not enough memory to continue");
  }
  left_space -= sizeof(synchronized_pool_resource_impl);
  php_assert(pool_size_ > left_space);

  if (impl_) {
    // in case of hard_reset it should be same place
    php_assert(impl_ == impl_place);
  }
  char *buffer = static_cast<char *>(shared_memory_) + (pool_size_ - left_space);
  impl_ = new(impl_place) synchronized_pool_resource_impl{buffer, static_cast<size_type>(left_space)};
}

void synchronized_pool_resource::free() {
  php_assert(impl_);
  php_assert(shared_memory_);
  php_assert(pool_size_);
  impl_->~synchronized_pool_resource_impl();
  munmap(shared_memory_, pool_size_);
  shared_memory_ = nullptr;
  impl_ = nullptr;
  pool_size_ = 0;
}

void synchronized_pool_resource::hard_reset() {
  php_assert(impl_);
  php_assert(shared_memory_);
  php_assert(pool_size_);
  impl_->~synchronized_pool_resource_impl();
  inplace_init();
}

void *synchronized_pool_resource::allocate(size_type size) {
  php_assert(impl_);
  php_assert(reserved_ >= size);
  php_assert(!allocations_forbidden_);
  dl::CriticalSectionGuard critical_section;
  void *result = impl_->allocate(size, reserved_);
  reserved_ = 0;
  return result;
}

void synchronized_pool_resource::deallocate(void *mem, size_type size) {
  php_assert(impl_);
  dl::CriticalSectionGuard critical_section;
  impl_->deallocate(mem, size);
}

void *synchronized_pool_resource::reallocate(void *, size_type, size_type) {
  php_critical_error("synchronized_pool_resource::reallocate is not supported");
  return nullptr;
}

bool synchronized_pool_resource::reserve(size_type size) {
  php_assert(impl_);
  php_assert(!reserved_);
  php_assert(!allocations_forbidden_);
  dl::CriticalSectionGuard critical_section;
  if ((reserved_ = impl_->reserve(size))) {
    return true;
  }
  return false;
}

void synchronized_pool_resource::rollback_reserve() {
  php_assert(impl_);
  php_assert(reserved_);
  dl::CriticalSectionGuard critical_section;
  impl_->rollback_reserve(reserved_);
  reserved_ = 0;
}

void synchronized_pool_resource::forbid_allocations() {
  php_assert(!allocations_forbidden_);
  allocations_forbidden_ = true;
}

void synchronized_pool_resource::allow_allocations() {
  php_assert(allocations_forbidden_);
  allocations_forbidden_ = false;
}

MemoryStats synchronized_pool_resource::get_memory_stats() {
  php_assert(impl_);
  dl::CriticalSectionGuard critical_section;
  return impl_->get_memory_stats();
}

} // namespace memory_resource
