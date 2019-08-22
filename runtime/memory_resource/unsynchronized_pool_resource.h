#pragma once
#include <map>

#include "runtime/memory_resource/details/memory_chunk_list.h"
#include "runtime/memory_resource/details/universal_reallocate.h"
#include "runtime/memory_resource/monotonic_buffer_resource.h"
#include "runtime/memory_resource/resource_allocator.h"
#include "runtime/php_assert.h"

namespace memory_resource {

class unsynchronized_pool_resource : private monotonic_buffer_resource {
public:
  using monotonic_buffer_resource::try_expand;
  using monotonic_buffer_resource::get_memory_stats;

  void init(void *buffer, size_type buffer_size);

  void *allocate(size_type size) {
    void *mem = nullptr;
    const auto aligned_size = details::align_for_chunk(size);
    if (aligned_size < MAX_CHUNK_BLOCK_SIZE_) {
      const auto chunk_id = details::get_chunk_id(aligned_size);
      if ((mem = free_chunks_[chunk_id].get_mem())) {
        memory_debug("allocate %d, chunk found, allocated address %p\n", aligned_size, mem);
      } else {
        mem = get_from_pool(aligned_size);
        memory_debug("allocate %d, chunk not found, allocated address from pool %p\n", aligned_size, mem);
      }
    } else {
      php_assert(huge_pieces_);
      const auto piece_it = huge_pieces_->lower_bound(aligned_size);
      if (piece_it == huge_pieces_->end()) {
        mem = get_from_pool(aligned_size);
        memory_debug("allocate %d, huge chunk not found, allocated address from pool %p\n", aligned_size, mem);
      } else {
        const auto left = piece_it->first - aligned_size;
        mem = piece_it->second;
        memory_debug("allocate %d, huge chunk (%ud) found, map size = %zu, allocated address %p\n",
                     aligned_size, piece_it->first, huge_pieces_->size(), mem);
        huge_pieces_->erase(piece_it);
        if (left) {
          put_memory_back(static_cast<char *>(mem) + aligned_size, left);
        }
      }
    }

    register_allocation(mem, aligned_size);
    return mem;
  }

  void *allocate0(size_type size) {
    return memset(allocate(size), 0x00, size);
  }

  void *reallocate(void *mem, size_type new_size, size_type old_size) {
    const auto aligned_old_size = details::align_for_chunk(old_size);
    const auto aligned_new_size = details::align_for_chunk(new_size);
    return details::universal_reallocate(*this, mem, aligned_new_size, aligned_old_size);
  }

  void deallocate(void *mem, size_type size) {
    memory_debug("deallocate %d at %p\n", size, mem);
    const auto aligned_size = details::align_for_chunk(size);
    put_memory_back(mem, aligned_size);
    register_deallocation(aligned_size);
  }

private:
  void put_memory_back(void *mem, size_type size) {
    if (!monotonic_buffer_resource::put_memory_back(mem, size)) {
      if (size < MAX_CHUNK_BLOCK_SIZE_) {
        size_type chunk_id = details::get_chunk_id(size);
        free_chunks_[chunk_id].put_mem(mem);
      } else {
        php_assert(huge_pieces_);
        huge_pieces_->emplace(size, mem);
      }
    }
  }

  using map_allocator = resource_allocator<std::pair<const size_type, void *>, unsynchronized_pool_resource>;
  using map_type = std::multimap<size_type, void *, std::less<size_type>, map_allocator>;

  std::aligned_storage<sizeof(map_type), alignof(map_type)>::type huge_pieces_storage_;
  map_type *huge_pieces_{nullptr};

  static constexpr size_type MAX_CHUNK_BLOCK_SIZE_{16u * 1024u};
  std::array<details::unsynchronized_memory_chunk_list, details::get_chunk_id(MAX_CHUNK_BLOCK_SIZE_)> free_chunks_;
};

} // namespace memory_resource
