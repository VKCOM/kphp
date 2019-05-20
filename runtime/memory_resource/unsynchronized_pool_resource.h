#pragma once

#include "runtime/memory_resource/monotonic_buffer_resource.h"

#include "runtime/php_assert.h"

#include <map>

namespace memory_resource {
template<class T, class MemoryResource>
class resource_allocator {
public:
  using value_type = T;

  template<class, class>
  friend
  class resource_allocator;

  explicit resource_allocator(MemoryResource &memory_resource) noexcept:
    memory_resource_(memory_resource) {
  }

  template<class U>
  explicit resource_allocator(const resource_allocator<U, MemoryResource> &other) noexcept:
    memory_resource_(other.memory_resource_) {
  }

  value_type *allocate(size_type size, void const * = nullptr) {
    php_assert(size == 1 && sizeof(T) < memory_resource_.max_chunk_block_size());
    auto result = static_cast <value_type *> (memory_resource_.allocate(sizeof(T)));
    if (unlikely(!result)) {
      php_critical_error("not enough memory to continue");
    }
    return result;
  }

  void deallocate(value_type *mem, size_type size) {
    php_assert(size == 1 && sizeof(T) < memory_resource_.max_chunk_block_size());
    memory_resource_.deallocate(mem, sizeof(T));
  }

private:
  MemoryResource &memory_resource_;
};

struct ChunkListNode {
  explicit ChunkListNode(ChunkListNode *next = nullptr) :
    next_(next) {
  }

  void *get_mem() {
    void *result = next_;
    if (next_) {
      next_ = next_->next_;
    }
    return result;
  }

  void put_mem(void *block) {
    next_ = new(block) ChunkListNode{next_};
  }

private:
  ChunkListNode *next_{nullptr};
};

class unsynchronized_pool_resource : private monotonic_buffer_resource {
public:
  using monotonic_buffer_resource::try_expand;
  using monotonic_buffer_resource::get_memory_stats;

  static constexpr size_type max_chunk_block_size() noexcept {
    return MAX_CHUNK_BLOCK_SIZE_;
  }

  void init(void *buffer, size_type buffer_size);

  void *allocate(size_type size) {
    void *mem = nullptr;
    const auto aligned_size = align_for_chunk(size);
    if (aligned_size < MAX_CHUNK_BLOCK_SIZE_) {
      const auto chunk_id = get_chunk_id(aligned_size);
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

  void *reallocate(void *mem, size_type new_size, size_type old_size) {
    const auto aligned_old_size = align_for_chunk(old_size);
    const auto aligned_new_size = align_for_chunk(new_size);
    return universal_reallocate(*this, mem, aligned_new_size, aligned_old_size);
  }

  void deallocate(void *mem, size_type size) {
    memory_debug("deallocate %d at %p\n", size, mem);
    const auto aligned_size = align_for_chunk(size);
    if (put_memory_back(mem, aligned_size)) {
      stats_.real_memory_used -= aligned_size;
    }
    stats_.memory_used -= aligned_size;
  }

private:
  static constexpr size_type align_for_chunk(size_type size) {
    return (size + 7) & -8;
  }

  static constexpr size_type get_chunk_id(size_type aligned_size) {
    return aligned_size >> 3;
  }

  bool put_memory_back(void *mem, size_type size) {
    if (monotonic_buffer_resource::put_memory_back(mem, size)) {
      return true;
    }

    if (size < MAX_CHUNK_BLOCK_SIZE_) {
      size_type chunk_id = get_chunk_id(size);
      free_chunks_[chunk_id].put_mem(mem);
    } else {
      php_assert(huge_pieces_);
      huge_pieces_->emplace(size, mem);
    }
    return false;
  }

  using map_allocator = resource_allocator<std::pair<const size_type, void *>, unsynchronized_pool_resource>;
  using map_type = std::multimap<size_type, void *, std::less<size_type>, map_allocator>;

  std::aligned_storage<sizeof(map_type), alignof(map_type)>::type huge_pieces_storage_;
  map_type *huge_pieces_{nullptr};

  static constexpr size_type MAX_CHUNK_BLOCK_SIZE_{16384};
  static constexpr size_t CHUNKS_COUNT_{MAX_CHUNK_BLOCK_SIZE_ >> 3};
  std::array<ChunkListNode, CHUNKS_COUNT_> free_chunks_;
};
} // namespace memory_resource
