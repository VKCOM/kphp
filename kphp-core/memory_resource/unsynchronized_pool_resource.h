// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>

#include "kphp-core/memory_resource/extra-memory-pool.h"
#include "kphp-core/memory_resource/details/memory_chunk_list.h"
#include "kphp-core/memory_resource/details/memory_chunk_tree.h"
#include "kphp-core/memory_resource/details/universal_reallocate.h"
#include "kphp-core/memory_resource/monotonic_buffer_resource.h"
#include "kphp-core/memory_resource/resource_allocator.h"
#include "kphp-core/functions/kphp-assert-core.h"

namespace memory_resource {

class unsynchronized_pool_resource : private monotonic_buffer_resource {
public:
  using monotonic_buffer_resource::try_expand;
  using monotonic_buffer_resource::get_memory_stats;
  using monotonic_buffer_resource::memory_begin;

  void init(void *buffer, size_t buffer_size, size_t oom_handling_buffer_size = 0) noexcept;
  void hard_reset() noexcept;
  void unfreeze_oom_handling_memory() noexcept;

  void *allocate(size_t size) noexcept {
    void *mem = nullptr;
    const auto aligned_size = details::align_for_chunk(size);
    if (aligned_size < MAX_CHUNK_BLOCK_SIZE_) {
      mem = try_allocate_small_piece(aligned_size);
      if (!mem) {
        mem = allocate_small_piece_from_fallback_resource(aligned_size);
      }
    } else {
      mem = allocate_huge_piece(aligned_size, true);
      if (!mem) {
        mem = perform_defragmentation_and_allocate_huge_piece(aligned_size);
      }
    }

    register_allocation(mem, aligned_size);
    return mem;
  }

  void *allocate0(size_t size) noexcept {
    auto *mem = allocate(size);
    if (likely(mem != nullptr)) {
      memset(mem, 0x00, size);
    }
    return mem;
  }

  void *reallocate(void *mem, size_t new_size, size_t old_size) noexcept {
    const auto aligned_old_size = details::align_for_chunk(old_size);
    const auto aligned_new_size = details::align_for_chunk(new_size);
    return details::universal_reallocate(*this, mem, aligned_new_size, aligned_old_size);
  }

  void deallocate(void *mem, size_t size) noexcept {
    memory_debug("deallocate %zu at %p\n", size, mem);
    const auto aligned_size = details::align_for_chunk(size);
    put_memory_back(mem, aligned_size);
    register_deallocation(aligned_size);
  }

  void perform_defragmentation() noexcept;

  bool is_enough_memory_for(size_t size) const noexcept {
    const auto aligned_size = details::align_for_chunk(size);
    // not using free_chunks_ here as the real size can be smaller
    return static_cast<size_t>(memory_end_ - memory_current_) >= aligned_size || huge_pieces_.has_memory_for(aligned_size);
  }

  extra_memory_pool *get_extra_memory_head() noexcept {
    return extra_memory_head_;
  }

  void add_extra_memory(extra_memory_pool *extra_memory) noexcept {
    extra_memory->next_in_chain = extra_memory_head_;
    extra_memory_head_ = extra_memory;
    put_memory_back(extra_memory->memory_begin(), extra_memory->get_pool_payload_size());
  }

private:
  void *try_allocate_small_piece(size_t aligned_size) noexcept {
    const auto chunk_id = details::get_chunk_id(aligned_size);
    auto *mem = free_chunks_[chunk_id].get_mem();
    if (mem) {
      --stats_.small_memory_pieces;
      memory_debug("allocate %zu, chunk found, allocated address %p\n", aligned_size, mem);
      return mem;
    }
    mem = get_from_pool(aligned_size, true);
    memory_debug("allocate %zu, chunk not found, allocated address from pool %p\n", aligned_size, mem);
    return mem;
  }

  void *allocate_huge_piece(size_t aligned_size, bool safe) noexcept {
    void *mem = nullptr;
    if (details::memory_chunk_tree::tree_node *piece = huge_pieces_.extract(aligned_size)) {
      --stats_.huge_memory_pieces;
      const size_t real_size = details::memory_chunk_tree::get_chunk_size(piece);
      mem = piece;
      if (const size_t left = real_size - aligned_size) {
        put_memory_back(static_cast<char *>(mem) + aligned_size, left);
      }
      memory_debug("allocate %zu, huge chunk (%zu) found, allocated address %p\n", aligned_size, real_size, mem);
      return piece;
    }
    mem = get_from_pool(aligned_size, safe);
    memory_debug("allocate %zu, huge chunk not found, allocated address from pool %p\n", aligned_size, mem);
    return mem;
  }

  void *allocate_small_piece_from_fallback_resource(size_t aligned_size) noexcept;
  void *perform_defragmentation_and_allocate_huge_piece(size_t aligned_size) noexcept;
  bool is_memory_from_extra_pool(void *mem, size_t size) const noexcept;

  void put_memory_back(void *mem, size_t size) noexcept {
    const bool from_extra_pool = (extra_memory_head_ != &extra_memory_tail_) && is_memory_from_extra_pool(mem, size);
    if (from_extra_pool || !monotonic_buffer_resource::put_memory_back(mem, size)) {
      if (size < MAX_CHUNK_BLOCK_SIZE_) {
        size_t chunk_id = details::get_chunk_id(size);
        free_chunks_[chunk_id].put_mem(mem);
        ++stats_.small_memory_pieces;
      } else {
        huge_pieces_.insert(mem, size);
        ++stats_.huge_memory_pieces;
      }
    }
  }

  details::memory_chunk_tree huge_pieces_;
  monotonic_buffer_resource fallback_resource_;
  size_t oom_handling_memory_size_{0};

  extra_memory_pool *extra_memory_head_{nullptr};
  extra_memory_pool extra_memory_tail_{sizeof(extra_memory_pool)};

  static constexpr size_t MAX_CHUNK_BLOCK_SIZE_{16u * 1024u};
  std::array<details::memory_chunk_list, details::get_chunk_id(MAX_CHUNK_BLOCK_SIZE_)> free_chunks_;
};

} // namespace memory_resource
